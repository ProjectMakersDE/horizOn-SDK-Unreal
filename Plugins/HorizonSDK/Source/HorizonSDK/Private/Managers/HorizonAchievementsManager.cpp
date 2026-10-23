// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#include "Managers/HorizonAchievementsManager.h"
#include "Dom/JsonObject.h"
#include "Managers/HorizonPlayerValuesManager.h"
#include "GenericPlatform/GenericPlatformHttp.h"

void UHorizonAchievementsManager::Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth,UHorizonPlayerValuesManager* Values)
{ HttpClient=Http; AuthManager=Auth; PlayerValues=Values; ClearCache(); }
void UHorizonAchievementsManager::SyncIdentity()
{
    const FString Next=HttpClient && AuthManager?AuthManager->GetCurrentUser().UserId+TEXT("|")+HttpClient->GetSessionToken():FString();
    const uint64 Generation=HttpClient?HttpClient->GetRequestContextEpoch():0;
    if (Next!=Identity || Generation!=ContextEpoch)
    { Identity=Next; ContextEpoch=Generation; ++Epoch; ++Ticket; bBusy=false; Revision=ServerMillis=-1; Current={}; }
}
void UHorizonAchievementsManager::ClearCache() { SyncIdentity(); ++Epoch; ++Ticket; bBusy=false; Current={}; }
FHorizonAchievementsSnapshot UHorizonAchievementsManager::GetCurrentAchievements() { SyncIdentity(); return Current; }
bool UHorizonAchievementsManager::IsKey(const FString& Value)
{
    if (Value.IsEmpty() || Value.Len()>24) return false;
    for (int32 I=0;I<Value.Len();++I) { const TCHAR C=Value[I]; if (!(C>='a' && C<='z') && !(C>='0' && C<='9') && (I==0 || (C!='.' && C!='_' && C!='-'))) return false; }
    return true;
}
int64 UHorizonAchievementsManager::TimeMillis(const FString& Value)
{ FDateTime Time; return FDateTime::ParseIso8601(*Value,Time)?Time.ToUnixTimestamp()*1000+Time.GetMillisecond():-1; }
FHorizonNetworkResponse UHorizonAchievementsManager::Error(const FString& Code,const FString& Message)
{ FHorizonNetworkResponse R; R.ServerErrorCode=Code; R.ErrorMessage=Message; return R; }
FString UHorizonAchievementsManager::Begin(const FString& User)
{
    SyncIdentity(); FGuid Id;
    if (!AuthManager || !HttpClient || !AuthManager->IsSignedIn() || HttpClient->GetSessionToken().IsEmpty() || HttpClient->GetSessionToken()!=AuthManager->GetCurrentUser().AccessToken) return TEXT("SESSION_REQUIRED");
    if (User!=AuthManager->GetCurrentUser().UserId) return TEXT("SESSION_FORBIDDEN");
    if (!FGuid::ParseExact(User,EGuidFormats::DigitsWithHyphens,Id)) return TEXT("INVALID_REQUEST");
    return bBusy?TEXT("REQUEST_IN_PROGRESS"):TEXT("");
}
void UHorizonAchievementsManager::Call(const FString& User,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Complete,bool Mutation)
{
    const FString Failure=Begin(User);
    if (!Failure.IsEmpty()) { Complete.ExecuteIfBound(Error(Failure,Failure)); return; }
    bBusy=true; if (Mutation) { ++Epoch; if (PlayerValues) PlayerValues->ClearCache(); }
    const uint64 SentTicket=++Ticket,SentContext=ContextEpoch; const int64 SentEpoch=Epoch; const FString SentIdentity=Identity;
    TWeakObjectPtr<UHorizonAchievementsManager> Weak(this);
    const auto Callback=FOnHttpResponse::CreateLambda([Weak,SentTicket,SentContext,SentEpoch,SentIdentity,Complete](const FHorizonNetworkResponse& Response)
    {
        auto* Self=Weak.Get(); if (!Self) return; Self->SyncIdentity();
        if (Self->Ticket!=SentTicket || Self->ContextEpoch!=SentContext || Self->Epoch!=SentEpoch || Self->Identity!=SentIdentity)
        { Complete.ExecuteIfBound(Error(TEXT("STALE_RESPONSE"),TEXT("Session or cache changed; retry the same keys explicitly"))); return; }
        Self->bBusy=false;
        if (Response.StatusCode==401 || Response.StatusCode==403) Self->ClearCache();
        Complete.ExecuteIfBound(Response);
    });
    if (Body) HttpClient->PostJson(Body.ToSharedRef(),Route,true,Callback); else HttpClient->Get(Route,true,Callback);
}
bool UHorizonAchievementsManager::CanAccept(const FString& User,const FHorizonAchievementsSnapshot& S) const
{ return S.bPresent && S.UserId==User && S.Revision>=Revision && TimeMillis(S.ServerTime)>=ServerMillis; }
bool UHorizonAchievementsManager::Adopt(const FString& User,FHorizonAchievementsSnapshot& S)
{
    if (!CanAccept(User,S)) return false;
    if (!S.bDefinitionsIncluded)
    {
        if (!Current.bPresent || Current.DefinitionVersion!=S.DefinitionVersion) return false;
        S.Definitions=Current.Definitions;
    }
    Revision=S.Revision; ServerMillis=TimeMillis(S.ServerTime); Current=S; return true;
}
void UHorizonAchievementsManager::GetAchievements(const FString& User,FOnAchievementsComplete Complete,const FString& Version)
{ Read(User,Complete,Version,false); }
void UHorizonAchievementsManager::Read(const FString& User,FOnAchievementsComplete Complete,const FString& Version,bool FullRetry)
{
    SyncIdentity();
    if (Version.Len()>512) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Invalid opaque definition version")); return; }
    const FString Known=FullRetry?FString():Version.IsEmpty() && Current.bPresent?Current.DefinitionVersion:Version;
    FString Route=TEXT("/api/v1/app/achievements?userId=")+FGenericPlatformHttp::UrlEncode(User);
    if (!Known.IsEmpty()) Route+=TEXT("&definitionVersion=")+FGenericPlatformHttp::UrlEncode(Known);
    Call(User,Route,nullptr,FOnHttpResponse::CreateWeakLambda(this,[this,User,Complete,FullRetry](const FHorizonNetworkResponse& Response)
    {
        if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
        FHorizonAchievementsSnapshot S;
        if (!FHorizonAchievementsSnapshot::TryFromJson(Response.JsonData,S)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid achievement snapshot")); return; }
        if (!CanAccept(User,S)) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older achievement snapshot")); return; }
        if (!S.bDefinitionsIncluded && (!Current.bPresent || Current.DefinitionVersion!=S.DefinitionVersion))
        {
            if (!FullRetry) { Read(User,Complete,TEXT(""),true); return; }
            Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("No matching materialized definitions")); return;
        }
        if (!Adopt(User,S)) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Older achievement snapshot")); return; }
        Complete.ExecuteIfBound(true,S,TEXT(""),TEXT(""));
    }));
}
void UHorizonAchievementsManager::Unlock(const FString& User,const FString& Key,FOnAchievementUnlockComplete Complete)
{
    if (!IsKey(Key)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Invalid original key")); return; }
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"),User); Body->SetStringField(TEXT("key"),Key);
    Call(User,TEXT("/api/v1/app/achievements/unlock"),Body,FOnHttpResponse::CreateWeakLambda(this,[this,User,Key,Complete](const FHorizonNetworkResponse& Response)
    {
        if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
        FHorizonAchievementUnlockResponse R;
        if (!FHorizonAchievementUnlockResponse::TryFromJson(Response.JsonData,R))
        { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid unlock result")); return; }
        const auto* Progress=R.Achievements.Progress.FindByPredicate([&](const FHorizonAchievementProgress& P) { return P.Key==Key; });
        const bool ValidDelivery=R.DeliveryResults.IsEmpty()
            ? Progress && (R.bReplayed || Progress->RewardState==TEXT("complete"))
            : R.DeliveryResults.Num()==1 && R.DeliveryResults[0].Key==Key;
        if (!ValidDelivery || (!R.bReplayed && (R.NewlyUnlocked.Num()!=1 || R.NewlyUnlocked[0]!=Key)))
        { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid unlock result")); return; }
        if (!Adopt(User,R.Achievements)) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Cannot adopt achievement snapshot")); return; }
        Complete.ExecuteIfBound(true,R,TEXT(""),TEXT(""));
    }),true);
}
void UHorizonAchievementsManager::DeliverRewards(const FString& User,const TArray<FString>& Keys,FOnAchievementDeliveryComplete Complete)
{
    TSet<FString> Unique;
    for (const auto& Key:Keys) { if (!IsKey(Key) || Unique.Contains(Key)) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Keep one to eight unique original keys")); return; } Unique.Add(Key); }
    if (Keys.IsEmpty() || Keys.Num()>8) { Complete.ExecuteIfBound(false,{},TEXT("INVALID_REQUEST"),TEXT("Keep one to eight original keys")); return; }
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("userId"),User); TArray<TSharedPtr<FJsonValue>> Array;
    for (const auto& Key:Keys) Array.Add(MakeShared<FJsonValueString>(Key)); Body->SetArrayField(TEXT("keys"),Array);
    Call(User,TEXT("/api/v1/app/achievements/rewards/deliver"),Body,FOnHttpResponse::CreateWeakLambda(this,[this,User,Unique,Complete](const FHorizonNetworkResponse& Response)
    {
        if (!Response.bSuccess) { Complete.ExecuteIfBound(false,{},Response.GetErrorCodeString(),Response.ErrorMessage); return; }
        FHorizonAchievementDeliveryResponse R;
        if (!FHorizonAchievementDeliveryResponse::TryFromJson(Response.JsonData,R) || R.Results.Num()!=Unique.Num() || R.Results.ContainsByPredicate([&](const FHorizonAchievementDeliveryResult& Item) { return !Unique.Contains(Item.Key); }))
        { Complete.ExecuteIfBound(false,{},TEXT("INVALID_RESPONSE"),TEXT("Invalid delivery result; retain original keys")); return; }
        if (!Adopt(User,R.Achievements)) { Complete.ExecuteIfBound(false,{},TEXT("STALE_RESPONSE"),TEXT("Cannot adopt achievement snapshot")); return; }
        Complete.ExecuteIfBound(true,R,TEXT(""),TEXT(""));
    }),true);
}
void UHorizonAchievementsManager::RequestAchievements(const FString& User,const FString& Version)
{ GetAchievements(User,FOnAchievementsComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonAchievementsSnapshot& S,const FString& Code,const FString& Message) { OnAchievementsResult.Broadcast(Ok,S,Code,Message); }),Version); }
void UHorizonAchievementsManager::UnlockAchievement(const FString& User,const FString& Key)
{ Unlock(User,Key,FOnAchievementUnlockComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonAchievementUnlockResponse& R,const FString& Code,const FString& Message) { OnUnlockResult.Broadcast(Ok,R,Code,Message); })); }
void UHorizonAchievementsManager::DeliverAchievementRewards(const FString& User,const TArray<FString>& Keys)
{ DeliverRewards(User,Keys,FOnAchievementDeliveryComplete::CreateWeakLambda(this,[this](bool Ok,const FHorizonAchievementDeliveryResponse& R,const FString& Code,const FString& Message) { OnDeliveryResult.Broadcast(Ok,R,Code,Message); })); }
