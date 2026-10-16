#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonInbox.h"
#include "HorizonInboxManager.generated.h"
class UHorizonPlayerValuesManager;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonInboxResult,bool,bSuccess,const FHorizonInboxSnapshot&,Inbox,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonInboxClaimResult,bool,bSuccess,const FHorizonInboxClaim&,Claim,const FString&,ErrorCode,const FString&,ErrorMessage);

/** Explicit snapshots and entry-bound actions; no hidden polling, signup or reward claims. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonInboxManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth,UHorizonPlayerValuesManager* Values);
    void GetInbox(const FString& UserId,FOnInboxComplete Complete,const FString& Since=FString());
    void MarkRead(const FString& UserId,const FString& EntryId,FOnInboxComplete Complete);
    void Remove(const FString& UserId,const FString& EntryId,FOnInboxComplete Complete);
    /** Retry only this same EntryId after an uncertain result; adopt the returned full balances. */
    void Claim(const FString& UserId,const FString& EntryId,FOnInboxClaimComplete Complete);
    UFUNCTION(BlueprintPure,Category="horizOn|Inbox") FHorizonInboxSnapshot GetCurrentInbox();
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox") void ClearCache();
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox") void RequestInbox(const FString& UserId,const FString& Since=TEXT(""));
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox") void MarkEntryRead(const FString& UserId,const FString& EntryId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox") void RemoveEntry(const FString& UserId,const FString& EntryId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox") void ClaimEntry(const FString& UserId,const FString& EntryId);
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Inbox") FString LastErrorCode;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Inbox") FOnHorizonInboxResult OnInboxResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Inbox") FOnHorizonInboxClaimResult OnInboxClaimResult;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient = nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager = nullptr;
    UPROPERTY() UHorizonPlayerValuesManager* PlayerValues = nullptr;
    FHorizonInboxSnapshot Current;
    FString Identity;
    int64 Epoch=0,MaxRevision=-1,MaxBroadcastRevision=-1,MaxServerTime=-1;
    bool bReading=false,bMutating=false;
    struct FReadGroup
    {
        FString User,Since,Identity;
        int64 Epoch=0;
        TArray<FOnInboxComplete> Callbacks;
    };
    FReadGroup ActiveRead;
    FReadGroup QueuedRead;
    void SyncIdentity();
    UFUNCTION() void ResetIdentity();
    bool Begin(const FString& UserId);
    bool IsCurrent(const FString& SentIdentity,int64 SentEpoch);
    bool CanAccept(const FHorizonInboxSnapshot& Snapshot,const FString& UserId) const;
    void Adopt(FHorizonInboxSnapshot Snapshot);
    void StartRead(FReadGroup Group,bool FullRetry=false);
    void ReceiveRead(const FHorizonNetworkResponse& Response,bool FullRetry);
    void FinishRead(bool Success,const FHorizonInboxSnapshot& Snapshot,const FString& Code,const FString& Message);
    void Mutate(const FString& UserId,const FString& EntryId,const FString& Action,FOnInboxComplete Complete);
    void Fail(const FString& Code,FOnInboxComplete Complete,const FString& Message=TEXT("Inbox request failed"));
    static int64 TimeMillis(const FString& Value);
    static bool ValidSince(const FString& Value);
};
