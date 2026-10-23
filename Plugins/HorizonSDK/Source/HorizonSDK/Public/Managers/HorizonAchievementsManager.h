// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonAchievements.h"
#include "HorizonAchievementsManager.generated.h"
class UHorizonPlayerValuesManager;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonAchievementsSnapshotResult,bool,bSuccess,const FHorizonAchievementsSnapshot&,Snapshot,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonAchievementUnlockResult,bool,bSuccess,const FHorizonAchievementUnlockResponse&,Result,const FString&,Code,const FString&,Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonAchievementDeliveryResult,bool,bSuccess,const FHorizonAchievementDeliveryResponse&,Result,const FString&,Code,const FString&,Message);
/** Explicit bounded requests. Rewards are credited atomically by the server; refresh values after unlocking. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonAchievementsManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth,UHorizonPlayerValuesManager* Values=nullptr);
    void GetAchievements(const FString& UserId,FOnAchievementsComplete Complete,const FString& DefinitionVersion=FString());
    void Unlock(const FString& UserId,const FString& Key,FOnAchievementUnlockComplete Complete);
    void DeliverRewards(const FString& UserId,const TArray<FString>& Keys,FOnAchievementDeliveryComplete Complete);
    UFUNCTION(BlueprintPure,Category="horizOn|Achievements") FHorizonAchievementsSnapshot GetCurrentAchievements();
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements") void ClearCache();
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements") void RequestAchievements(const FString& UserId,const FString& DefinitionVersion=TEXT(""));
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements") void UnlockAchievement(const FString& UserId,const FString& Key);
    UFUNCTION(BlueprintCallable,Category="horizOn|Achievements") void DeliverAchievementRewards(const FString& UserId,const TArray<FString>& Keys);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Achievements") FOnHorizonAchievementsSnapshotResult OnAchievementsResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Achievements") FOnHorizonAchievementUnlockResult OnUnlockResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Achievements") FOnHorizonAchievementDeliveryResult OnDeliveryResult;
    static bool IsKey(const FString& Value);
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    UPROPERTY() UHorizonPlayerValuesManager* PlayerValues=nullptr;
    FString Identity;
    uint64 ContextEpoch=0,Ticket=0;
    int64 Epoch=0,Revision=-1,ServerMillis=-1;
    bool bBusy=false;
    FHorizonAchievementsSnapshot Current;
    void SyncIdentity();
    FString Begin(const FString& UserId);
    void Call(const FString& UserId,const FString& Route,const TSharedPtr<FJsonObject>& Body,FOnHttpResponse Complete,bool Mutation=false);
    void Read(const FString& UserId,FOnAchievementsComplete Complete,const FString& Version,bool FullRetry);
    bool CanAccept(const FString& UserId,const FHorizonAchievementsSnapshot& Snapshot) const;
    bool Adopt(const FString& UserId,FHorizonAchievementsSnapshot& Snapshot);
    static int64 TimeMillis(const FString& Value);
    static FHorizonNetworkResponse Error(const FString& Code,const FString& Message);
};
