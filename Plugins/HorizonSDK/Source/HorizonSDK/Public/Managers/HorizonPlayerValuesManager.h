#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonPlayerValues.h"
#include "HorizonPlayerValuesManager.generated.h"

DECLARE_DELEGATE_FourParams(FOnPlayerValuesComplete, bool, const FHorizonPlayerValuesState&, const FString&, const FString&);
DECLARE_DELEGATE_FourParams(FOnPlayerValuesOffersComplete, bool, const FHorizonPlayerValuesOffers&, const FString&, const FString&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonPlayerValuesResult, bool, bSuccess, const FHorizonPlayerValuesState&, State, const FString&, ErrorCode, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonPlayerValuesOffersResult, bool, bSuccess, const FHorizonPlayerValuesOffers&, Data, const FString&, ErrorCode, const FString&, ErrorMessage);

/** Atomic server balances. Never automatically create a new operation ID after an uncertain result. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonPlayerValuesManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Client, UHorizonAuthManager* Auth);
    void GetState(const FString& UserId, FOnPlayerValuesComplete OnComplete);
    /** Adopt only a complete authorized server snapshot, e.g. an atomic Inbox claim. */
    bool AcceptServerState(const FString& UserId,const FHorizonPlayerValuesState& Value);
    int64 GetCacheEpoch();
    void GetOffers(const FString& UserId, FOnPlayerValuesOffersComplete OnComplete);
    void Purchase(const FString& UserId, const FString& OfferKey, const FString& OperationId, FOnPlayerValuesComplete OnComplete);
    void Adjust(const FString& UserId, const FString& Key, int64 Amount, const FString& Reason, const FString& OperationId, FOnPlayerValuesComplete OnComplete);
    UFUNCTION(BlueprintCallable, Category="horizOn|PlayerValues") FString CreateOperationId();
    UFUNCTION(BlueprintPure, Category="horizOn|PlayerValues") bool HasServerClock();
    UFUNCTION(BlueprintPure, Category="horizOn|PlayerValues") FHorizonPlayerValuesState GetCurrentState();
    UFUNCTION(BlueprintPure, Category="horizOn|PlayerValues") FHorizonPlayerValuesOffers GetCurrentOffers();
    UFUNCTION(BlueprintCallable, Category="horizOn|PlayerValues") void ClearCache();
    UFUNCTION(BlueprintCallable, Category="horizOn|PlayerValues") void RequestState(const FString& UserId);
    UFUNCTION(BlueprintCallable, Category="horizOn|PlayerValues") void RequestOffers(const FString& UserId);
    UFUNCTION(BlueprintCallable, Category="horizOn|PlayerValues") void PurchaseOffer(const FString& UserId, const FString& OfferKey, const FString& OperationId);
    UFUNCTION(BlueprintCallable, Category="horizOn|PlayerValues") void AdjustValue(const FString& UserId, const FString& Key, int64 Amount, const FString& Reason, const FString& OperationId);
    UPROPERTY(BlueprintReadOnly, Category="horizOn|PlayerValues") FString LastErrorCode;
    UPROPERTY(BlueprintAssignable, Category="horizOn|PlayerValues") FOnHorizonPlayerValuesResult OnValuesResult;
    UPROPERTY(BlueprintAssignable, Category="horizOn|PlayerValues") FOnHorizonPlayerValuesOffersResult OnOffersResult;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient = nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager = nullptr;
    FHorizonPlayerValuesState State;
    FHorizonPlayerValuesOffers Offers;
    FString Identity;
    int64 Epoch = 0, Revision = -1, Definition = -1, ServerMillis = -1;
    double ObservedAt = 0;
    bool Begin(const FString& UserId);
    void SyncIdentity();
    UFUNCTION() void ResetIdentity();
    bool IsCurrent(const FString& SentIdentity, int64 SentEpoch);
    bool AcceptClock(const FString& Time);
    bool AcceptState(const FHorizonPlayerValuesState& Value, const FString& UserId);
    void StateCall(const FString& UserId, const FString& Route, const TSharedPtr<FJsonObject>& Body, FOnPlayerValuesComplete OnComplete);
    static bool ValidOperationId(const FString& Value);
    static int64 TimeMillis(const FString& Value);
#if WITH_DEV_AUTOMATION_TESTS
    friend struct FPlayerValuesTransportTestState;
#endif
};
