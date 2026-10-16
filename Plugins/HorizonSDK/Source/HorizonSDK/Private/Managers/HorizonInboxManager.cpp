#include "Managers/HorizonInboxManager.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Dom/JsonObject.h"

void UHorizonInboxManager::Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth,UHorizonPlayerValuesManager* Values)
{
    HttpClient=Http; AuthManager=Auth; PlayerValues=Values;
    if (Auth) { Auth->OnUserSignedIn.AddUniqueDynamic(this,&UHorizonInboxManager::ResetIdentity); Auth->OnUserSignedOut.AddUniqueDynamic(this,&UHorizonInboxManager::ResetIdentity); }
}
void UHorizonInboxManager::SyncIdentity()
{
    const FString Next=AuthManager && HttpClient ? AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken()+TEXT("|")+HttpClient->GetApiKey()+TEXT("|")+HttpClient->GetActiveHost():FString();
    if (Next!=Identity) { ResetIdentity(); Identity=Next; }
}
void UHorizonInboxManager::ResetIdentity()
{ Identity.Empty(); ++Epoch; Current={}; MaxRevision=MaxBroadcastRevision=MaxServerTime=-1; LastErrorCode.Empty(); }
void UHorizonInboxManager::ClearCache() { SyncIdentity(); ++Epoch; Current={}; }
FHorizonInboxSnapshot UHorizonInboxManager::GetCurrentInbox() { SyncIdentity(); return Current; }
bool UHorizonInboxManager::IsCurrent(const FString& SentIdentity,int64 SentEpoch) { SyncIdentity(); return Identity==SentIdentity && Epoch==SentEpoch; }
bool UHorizonInboxManager::Begin(const FString& UserId)
{
    SyncIdentity(); FGuid Id;
    if (!AuthManager || !HttpClient || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || AuthManager->GetCurrentUser().AccessToken!=HttpClient->GetSessionToken()) LastErrorCode=TEXT("SESSION_REQUIRED");
    else if (AuthManager->GetCurrentUser().UserId!=UserId) LastErrorCode=TEXT("SESSION_FORBIDDEN");
    else if (!FGuid::ParseExact(UserId,EGuidFormats::DigitsWithHyphens,Id)) LastErrorCode=TEXT("INVALID_REQUEST");
    else LastErrorCode.Empty();
    return LastErrorCode.IsEmpty();
}
int64 UHorizonInboxManager::TimeMillis(const FString& Value)
{ FDateTime Time; return FDateTime::ParseIso8601(*Value,Time)?Time.ToUnixTimestamp()*1000+Time.GetMillisecond():-1; }
bool UHorizonInboxManager::ValidSince(const FString& Value)
{
    if (Value.Len()>128) return false;
    for (TCHAR C:Value) if (!(C>='a' && C<='z') && !(C>='A' && C<='Z') && !(C>='0' && C<='9') && C!=':' && C!='_' && C!='-') return false;
    return true;
}
void UHorizonInboxManager::Fail(const FString& Code,FOnInboxComplete Complete,const FString& Message)
{ LastErrorCode=Code; Complete.ExecuteIfBound(false,{},Code,Message); }
bool UHorizonInboxManager::CanAccept(const FHorizonInboxSnapshot& Snapshot,const FString& UserId) const
{
    return Snapshot.bPresent && Snapshot.UserId==UserId && Snapshot.Revision>=MaxRevision && Snapshot.BroadcastRevision>=MaxBroadcastRevision && TimeMillis(Snapshot.ServerTime)>=MaxServerTime;
}
void UHorizonInboxManager::Adopt(FHorizonInboxSnapshot Snapshot)
{
    MaxRevision=Snapshot.Revision; MaxBroadcastRevision=Snapshot.BroadcastRevision; MaxServerTime=TimeMillis(Snapshot.ServerTime);
    Current=MoveTemp(Snapshot); LastErrorCode.Empty();
}
void UHorizonInboxManager::GetInbox(const FString& UserId,FOnInboxComplete Complete,const FString& Since)
{
    if (!Begin(UserId)) { Fail(LastErrorCode,Complete); return; }
    if (!ValidSince(Since)) { Fail(TEXT("INVALID_REQUEST"),Complete); return; }
    const FString StartedIdentity=Identity; const int64 StartedEpoch=Epoch;
    if (bReading)
    {
        if (QueuedRead.Callbacks.Num() && (QueuedRead.Identity!=Identity || QueuedRead.Epoch!=Epoch))
        { auto Old=MoveTemp(QueuedRead.Callbacks); QueuedRead={}; for (const auto& Callback:Old) Callback.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Session or cache changed")); }
        if (!IsCurrent(StartedIdentity,StartedEpoch)) { Fail(TEXT("STALE_RESPONSE"),Complete); return; }
        if (QueuedRead.Callbacks.Num()>=64) { Fail(TEXT("REQUEST_IN_PROGRESS"),Complete); return; }
        if (QueuedRead.Callbacks.IsEmpty()) { QueuedRead.User=UserId; QueuedRead.Since=Since; QueuedRead.Identity=Identity; QueuedRead.Epoch=Epoch; }
        else if (QueuedRead.Since!=Since) QueuedRead.Since.Empty();
        QueuedRead.Callbacks.Add(Complete); return;
    }
    FReadGroup Group; Group.User=UserId; Group.Since=Since; Group.Identity=Identity; Group.Epoch=Epoch; Group.Callbacks.Add(Complete); StartRead(MoveTemp(Group));
}
void UHorizonInboxManager::StartRead(FReadGroup Group,bool FullRetry)
{
    ActiveRead=MoveTemp(Group); bReading=true;
    FString Route=TEXT("/api/v1/app/inbox?userId=")+FGenericPlatformHttp::UrlEncode(ActiveRead.User);
    if (!ActiveRead.Since.IsEmpty()) Route+=TEXT("&since=")+FGenericPlatformHttp::UrlEncode(ActiveRead.Since);
    TWeakObjectPtr<UHorizonInboxManager> Weak(this);
    HttpClient->Get(Route,true,FOnHttpResponse::CreateLambda([Weak,FullRetry](const FHorizonNetworkResponse& Response) { if (auto* Self=Weak.Get()) Self->ReceiveRead(Response,FullRetry); }));
}
void UHorizonInboxManager::ReceiveRead(const FHorizonNetworkResponse& Response,bool FullRetry)
{
    if (!IsCurrent(ActiveRead.Identity,ActiveRead.Epoch)) { FinishRead(false,{},TEXT("STALE_RESPONSE"),TEXT("Session or cache changed")); return; }
    if (!Response.bSuccess) { if (Response.StatusCode==401 || Response.StatusCode==403) ClearCache(); FinishRead(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
    FHorizonInboxSnapshot Snapshot;
    if (!FHorizonInboxSnapshot::TryFromJson(Response.JsonData,Snapshot)) { FinishRead(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid inbox snapshot")); return; }
    if (!CanAccept(Snapshot,ActiveRead.User)) { FinishRead(false,{},TEXT("STALE_RESPONSE"),TEXT("Older inbox snapshot")); return; }
    if (Snapshot.bUnchanged)
    {
        if (!Current.bPresent || Current.Cursor!=Snapshot.Cursor || Current.Revision!=Snapshot.Revision || Current.BroadcastRevision!=Snapshot.BroadcastRevision)
        {
            if (FullRetry) { FinishRead(false,{},TEXT("INVALID_RESPONSE"),TEXT("Unchanged reply has no matching materialized snapshot")); return; }
            auto Group=MoveTemp(ActiveRead); Group.Since.Empty(); StartRead(MoveTemp(Group),true); return;
        }
        Snapshot.Entries=Current.Entries;
    }
    Adopt(Snapshot); FinishRead(true,Snapshot,TEXT(""),TEXT(""));
}
void UHorizonInboxManager::FinishRead(bool Success,const FHorizonInboxSnapshot& Snapshot,const FString& Code,const FString& Message)
{
    auto Callbacks=MoveTemp(ActiveRead.Callbacks); auto Next=MoveTemp(QueuedRead); ActiveRead={}; QueuedRead={}; bReading=false; LastErrorCode=Code;
    // Start at most one queued HTTP request before invoking re-entrant caller callbacks.
    if (!Next.Callbacks.IsEmpty())
    {
        if (IsCurrent(Next.Identity,Next.Epoch)) StartRead(MoveTemp(Next));
        else for (const auto& Callback:Next.Callbacks) Callback.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Session or cache changed"));
    }
    for (const auto& Callback:Callbacks) Callback.ExecuteIfBound(Success,Snapshot,Code,Message);
}
void UHorizonInboxManager::Mutate(const FString& UserId,const FString& EntryId,const FString& Action,FOnInboxComplete Complete)
{
    if (!Begin(UserId)) { Fail(LastErrorCode,Complete); return; }
    if (!FHorizonInboxEntry::IsEntryId(EntryId)) { Fail(TEXT("INVALID_REQUEST"),Complete); return; }
    if (bMutating) { Fail(TEXT("REQUEST_IN_PROGRESS"),Complete); return; }
    bMutating=true; ++Epoch; const FString SentIdentity=Identity; const int64 SentEpoch=Epoch;
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"),UserId); Body->SetStringField(TEXT("entryId"),EntryId);
    TWeakObjectPtr<UHorizonInboxManager> Weak(this);
    HttpClient->PostJson(Body,TEXT("/api/v1/app/inbox/")+Action,true,FOnHttpResponse::CreateLambda([Weak,UserId,SentIdentity,SentEpoch,Complete](const FHorizonNetworkResponse& Response)
    {
        auto* Self=Weak.Get(); if (!Self) return; Self->bMutating=false;
        if (!Self->IsCurrent(SentIdentity,SentEpoch)) { Self->Fail(TEXT("STALE_RESPONSE"),Complete); return; }
        if (!Response.bSuccess) { if (Response.StatusCode==401 || Response.StatusCode==403) Self->ClearCache(); Self->Fail(Response.GetErrorCodeString(),Complete,Response.ErrorMessage); return; }
        FHorizonInboxSnapshot Snapshot;
        if (!FHorizonInboxSnapshot::TryFromJson(Response.JsonData,Snapshot) || Snapshot.bUnchanged) { Self->Fail(TEXT("INVALID_RESPONSE"),Complete); return; }
        if (!Self->CanAccept(Snapshot,UserId)) { Self->Fail(TEXT("STALE_RESPONSE"),Complete); return; }
        Self->Adopt(Snapshot); Complete.ExecuteIfBound(true,Snapshot,TEXT(""),TEXT(""));
    }));
}
void UHorizonInboxManager::MarkRead(const FString& UserId,const FString& EntryId,FOnInboxComplete Complete) { Mutate(UserId,EntryId,TEXT("read"),Complete); }
void UHorizonInboxManager::Remove(const FString& UserId,const FString& EntryId,FOnInboxComplete Complete) { Mutate(UserId,EntryId,TEXT("remove"),Complete); }
void UHorizonInboxManager::Claim(const FString& UserId,const FString& EntryId,FOnInboxClaimComplete Complete)
{
    if (!Begin(UserId)) { Complete.ExecuteIfBound(false,{},LastErrorCode,LastErrorCode); return; }
    if (!PlayerValues || !FHorizonInboxEntry::IsEntryId(EntryId) || bMutating) { LastErrorCode=bMutating?TEXT("REQUEST_IN_PROGRESS"):TEXT("INVALID_REQUEST"); Complete.ExecuteIfBound(false,{},LastErrorCode,LastErrorCode); return; }
    bMutating=true; ++Epoch; const FString SentIdentity=Identity; const int64 SentEpoch=Epoch,ValuesEpoch=PlayerValues->GetCacheEpoch();
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"),UserId); Body->SetStringField(TEXT("entryId"),EntryId);
    TWeakObjectPtr<UHorizonInboxManager> Weak(this);
    HttpClient->PostJson(Body,TEXT("/api/v1/app/inbox/claim"),true,FOnHttpResponse::CreateLambda([Weak,UserId,EntryId,SentIdentity,SentEpoch,ValuesEpoch,Complete](const FHorizonNetworkResponse& Response)
    {
        auto* Self=Weak.Get(); if (!Self) return; Self->bMutating=false;
        auto FailClaim=[&](const FString& Code) { Self->LastErrorCode=Code; Complete.ExecuteIfBound(false,{},Code,Response.ErrorMessage); };
        if (!Self->IsCurrent(SentIdentity,SentEpoch) || !Self->PlayerValues || Self->PlayerValues->GetCacheEpoch()!=ValuesEpoch) { FailClaim(TEXT("STALE_RESPONSE")); return; }
        if (!Response.bSuccess) { if (Response.StatusCode==401 || Response.StatusCode==403) Self->ClearCache(); FailClaim(Response.GetErrorCodeString()); return; }
        FHorizonInboxClaim Result;
        if (!FHorizonInboxClaim::TryFromJson(Response.JsonData,Result) || Result.EntryId!=EntryId) { FailClaim(TEXT("INVALID_RESPONSE")); return; }
        if (!Self->CanAccept(Result.Inbox,UserId) || !Self->PlayerValues->AcceptServerState(UserId,Result.PlayerValues)) { FailClaim(TEXT("STALE_RESPONSE")); return; }
        Self->Adopt(Result.Inbox); Complete.ExecuteIfBound(true,Result,TEXT(""),TEXT(""));
    }));
}
void UHorizonInboxManager::RequestInbox(const FString& UserId,const FString& Since)
{ GetInbox(UserId,FOnInboxComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonInboxSnapshot& Value,const FString& Code,const FString& Message) { OnInboxResult.Broadcast(Ok,Value,Code,Message); }),Since); }
void UHorizonInboxManager::MarkEntryRead(const FString& UserId,const FString& EntryId)
{ MarkRead(UserId,EntryId,FOnInboxComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonInboxSnapshot& Value,const FString& Code,const FString& Message) { OnInboxResult.Broadcast(Ok,Value,Code,Message); })); }
void UHorizonInboxManager::RemoveEntry(const FString& UserId,const FString& EntryId)
{ Remove(UserId,EntryId,FOnInboxComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonInboxSnapshot& Value,const FString& Code,const FString& Message) { OnInboxResult.Broadcast(Ok,Value,Code,Message); })); }
void UHorizonInboxManager::ClaimEntry(const FString& UserId,const FString& EntryId)
{ Claim(UserId,EntryId,FOnInboxClaimComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonInboxClaim& Value,const FString& Code,const FString& Message) { OnInboxClaimResult.Broadcast(Ok,Value,Code,Message); })); }
