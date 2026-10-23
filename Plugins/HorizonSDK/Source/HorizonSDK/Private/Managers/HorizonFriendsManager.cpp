// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonFriendsManager.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "GenericPlatform/GenericPlatformHttp.h"

void UHorizonFriendsManager::Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth)
{ HttpClient=Http; AuthManager=Auth; ClearCache(); }
void UHorizonFriendsManager::SyncIdentity()
{
    const FString Next=HttpClient && AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();
    const uint64 Generation=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if (Next!=Identity || Generation!=ContextEpoch)
    { Identity=Next; ContextEpoch=Generation; ++Epoch; ++Ticket; bBusy=false; Revision=ServerMillis=-1; Current={}; }
}
void UHorizonFriendsManager::ClearCache() { SyncIdentity(); ++Epoch; ++Ticket; bBusy=false; Current={}; }
FHorizonFriendsSnapshot UHorizonFriendsManager::GetCurrentFriends() { SyncIdentity(); return Current; }
int64 UHorizonFriendsManager::TimeMillis(const FString& Value)
{ FDateTime Time; return FDateTime::ParseIso8601(*Value,Time)?Time.ToUnixTimestamp()*1000+Time.GetMillisecond():-1; }
FHorizonNetworkResponse UHorizonFriendsManager::Error(const FString& Code,const FString& Message)
{ FHorizonNetworkResponse R; R.ServerErrorCode=Code; R.ErrorMessage=Message; return R; }
FString UHorizonFriendsManager::Begin(const FString& User)
{
    SyncIdentity(); FGuid Id;
    if (!AuthManager || !HttpClient || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken) return TEXT("SESSION_REQUIRED");
    if (User!=AuthManager->GetCurrentUser().UserId) return TEXT("SESSION_FORBIDDEN");
    if (!FGuid::ParseExact(User,EGuidFormats::DigitsWithHyphens,Id)) return TEXT("INVALID_REQUEST");
    return bBusy?TEXT("REQUEST_IN_PROGRESS"):TEXT("");
}
void UHorizonFriendsManager::Call(const FString& User,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Complete,bool Mutation)
{
    const FString Failure=Begin(User);
    if (!Failure.IsEmpty()) { Complete.ExecuteIfBound(Error(Failure,Failure)); return; }
    bBusy=true; if (Mutation) ++Epoch;
    const uint64 SentTicket=++Ticket,SentContext=ContextEpoch; const int64 SentEpoch=Epoch; const FString SentIdentity=Identity;
    TWeakObjectPtr<UHorizonFriendsManager> Weak(this);
    const auto Callback=FOnHttpResponse::CreateLambda([Weak,SentTicket,SentContext,SentEpoch,SentIdentity,Complete](const FHorizonNetworkResponse& Response)
    {
        auto* Self=Weak.Get(); if (!Self) return; Self->SyncIdentity();
        if (Self->Ticket!=SentTicket || Self->ContextEpoch!=SentContext || Self->Epoch!=SentEpoch || Self->Identity!=SentIdentity)
        { Complete.ExecuteIfBound(Error(TEXT("STALE_RESPONSE"),TEXT("Session or cache changed; reconcile the original action explicitly"))); return; }
        Self->bBusy=false;
        if (Response.StatusCode==401 || Response.StatusCode==403) Self->ClearCache();
        Complete.ExecuteIfBound(Response);
    });
    if (Body) HttpClient->PostJson(Body.ToSharedRef(),Route,true,Callback); else HttpClient->Get(Route,true,Callback);
}

bool UHorizonFriendsManager::Adopt(const FString& User,const FHorizonFriendsSnapshot& S)
{
    if (!S.bPresent || S.UserId!=User || S.Revision<Revision || TimeMillis(S.ServerTime)<ServerMillis) return false;
    Revision=S.Revision; ServerMillis=TimeMillis(S.ServerTime); Current=S; return true;
}
void UHorizonFriendsManager::Receive(const FString& User,const FHorizonNetworkResponse& Response,FOnFriendsComplete Complete)
{
    if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
    FHorizonFriendsSnapshot S;
    if (!FHorizonFriendsSnapshot::TryFromJson(Response.JsonData,S)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid friends snapshot")); return; }
    if (!Adopt(User,S)) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older or foreign friends snapshot")); return; }
    Complete.ExecuteIfBound(true,S,TEXT(""),TEXT(""));
}
void UHorizonFriendsManager::GetFriends(const FString& User,FOnFriendsComplete Complete)
{
    Call(User,TEXT("/api/v1/app/friends?userId=")+FGenericPlatformHttp::UrlEncode(User),nullptr,
        FOnHttpResponse::CreateWeakLambda(this,[this,User,Complete](const FHorizonNetworkResponse& Response) { Receive(User,Response,Complete); }));
}
void UHorizonFriendsManager::FindPlayer(const FString& User,const FString& Query,FOnFriendLookupComplete Complete)
{
    if (!FHorizonFriendLookup::IsPlayerQuery(Query)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Use one exact UUID or canonical H-code")); return; }
    const FString Route=TEXT("/api/v1/app/friends/find?userId=")+FGenericPlatformHttp::UrlEncode(User)+TEXT("&query=")+FGenericPlatformHttp::UrlEncode(Query);
    Call(User,Route,nullptr,FOnHttpResponse::CreateWeakLambda(this,[Query,Complete](const FHorizonNetworkResponse& Response)
    {
        if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
        FHorizonFriendLookup R;
        if (!FHorizonFriendLookup::TryFromJson(Response.JsonData,R) || !(R.FriendCode==Query || R.UserId.Equals(Query,ESearchCase::IgnoreCase)))
        { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Lookup did not match the exact public identity")); return; }
        Complete.ExecuteIfBound(true,R,TEXT(""),TEXT(""));
    }));
}
void UHorizonFriendsManager::Mutate(const FString& User,const FString& Target,int64 Expected,const FString& RequestId,const FString& Action,FOnFriendsComplete Complete)
{
    FGuid Peer,Request;
    const bool NeedsRequest=Action==TEXT("accept") || Action==TEXT("decline") || Action==TEXT("cancel");
    if (!FGuid::ParseExact(Target,EGuidFormats::DigitsWithHyphens,Peer) || User.Equals(Target,ESearchCase::IgnoreCase) || Expected<0 || Expected>9007199254740991LL ||
        (NeedsRequest && !FGuid::ParseExact(RequestId,EGuidFormats::DigitsWithHyphens,Request)) || (!NeedsRequest && !RequestId.IsEmpty()))
    { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Keep explicit revision, target and original request identity")); return; }
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"),User); Body->SetStringField(TEXT("targetUserId"),Target);
    Body->SetField(TEXT("expectedRevision"),MakeShared<FJsonValueNumberString>(LexToString(Expected)));
    if (NeedsRequest) Body->SetStringField(TEXT("requestId"),RequestId);
    Call(User,TEXT("/api/v1/app/friends/")+Action,Body,
        FOnHttpResponse::CreateWeakLambda(this,[this,User,Complete](const FHorizonNetworkResponse& Response) { Receive(User,Response,Complete); }),true);
}
void UHorizonFriendsManager::SendRequest(const FString& User,const FString& Target,int64 Expected,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,TEXT(""),TEXT("request"),Complete); }
void UHorizonFriendsManager::AcceptRequest(const FString& User,const FString& Target,int64 Expected,const FString& RequestId,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,RequestId,TEXT("accept"),Complete); }
void UHorizonFriendsManager::DeclineRequest(const FString& User,const FString& Target,int64 Expected,const FString& RequestId,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,RequestId,TEXT("decline"),Complete); }
void UHorizonFriendsManager::CancelRequest(const FString& User,const FString& Target,int64 Expected,const FString& RequestId,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,RequestId,TEXT("cancel"),Complete); }
void UHorizonFriendsManager::RemoveFriend(const FString& User,const FString& Target,int64 Expected,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,TEXT(""),TEXT("remove"),Complete); }
void UHorizonFriendsManager::BlockPlayer(const FString& User,const FString& Target,int64 Expected,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,TEXT(""),TEXT("block"),Complete); }
void UHorizonFriendsManager::UnblockPlayer(const FString& User,const FString& Target,int64 Expected,FOnFriendsComplete Complete)
{ Mutate(User,Target,Expected,TEXT(""),TEXT("unblock"),Complete); }
void UHorizonFriendsManager::RequestFriends(const FString& User)
{ GetFriends(User,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::FindFriendPlayer(const FString& User,const FString& Query)
{ FindPlayer(User,Query,FOnFriendLookupComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendLookup& R,const FString& Code,const FString& Message) { OnLookupResult.Broadcast(Ok,R,Code,Message); })); }
void UHorizonFriendsManager::SendFriendRequest(const FString& User,const FString& Target,int64 Expected)
{ SendRequest(User,Target,Expected,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::AcceptFriendRequest(const FString& User,const FString& Target,int64 Expected,const FString& RequestId)
{ AcceptRequest(User,Target,Expected,RequestId,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::DeclineFriendRequest(const FString& User,const FString& Target,int64 Expected,const FString& RequestId)
{ DeclineRequest(User,Target,Expected,RequestId,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::CancelFriendRequest(const FString& User,const FString& Target,int64 Expected,const FString& RequestId)
{ CancelRequest(User,Target,Expected,RequestId,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::RemoveFriendPlayer(const FString& User,const FString& Target,int64 Expected)
{ RemoveFriend(User,Target,Expected,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::BlockFriendPlayer(const FString& User,const FString& Target,int64 Expected)
{ BlockPlayer(User,Target,Expected,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
void UHorizonFriendsManager::UnblockFriendPlayer(const FString& User,const FString& Target,int64 Expected)
{ UnblockPlayer(User,Target,Expected,FOnFriendsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonFriendsSnapshot& S,const FString& Code,const FString& Message) { OnFriendsResult.Broadcast(Ok,S,Code,Message); })); }
