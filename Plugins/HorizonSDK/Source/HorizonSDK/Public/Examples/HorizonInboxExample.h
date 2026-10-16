#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonInbox.h"
#include "HorizonInboxExample.generated.h"
class UHorizonSubsystem;

UCLASS()
class HORIZONSDK_API UHorizonInboxPendingClaim : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString UserId;
    UPROPERTY() FString ProjectScope;
    UPROPERTY() FString EntryId;
};

/** Call only after your existing player session is restored. All actions are explicit UI actions. */
UCLASS()
class HORIZONSDK_API AHorizonInboxExample : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox Example") void LoadInbox();
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox Example") void MarkRead(const FString& EntryId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox Example") void Remove(const FString& EntryId);
    /** Persist the original entry before sending. Calling again retries that same entry only. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Inbox Example") void Claim(const FString& EntryId);
    UFUNCTION(BlueprintPure,Category="horizOn|Inbox Example") FHorizonInboxSnapshot GetCurrentInbox() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Inbox Example") FString GetPendingClaimEntryId() const;
private:
    UHorizonSubsystem* Session() const;
    void Loaded(bool Success,const FHorizonInboxSnapshot& Inbox,const FString& Code,const FString& Message);
    bool bClaiming=false;
};
