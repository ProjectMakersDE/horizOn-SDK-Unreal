#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HorizonSessionSave.generated.h"

UCLASS()
class HORIZONSDK_API UHorizonSessionSave : public USaveGame
{
    GENERATED_BODY()

public:
    static const FString SlotName;
    static const int32 UserIndex;

    UPROPERTY()
    FString CachedUserId;

    UPROPERTY()
    FString CachedAccessToken;

    UPROPERTY()
    FString CachedAnonymousToken;

    UPROPERTY()
    FString CachedDisplayName;

    UPROPERTY()
    bool bIsAnonymous = false;

    /** Save current state to disk */
    bool SaveToDisk() const;

    /** Load from disk, returns nullptr if no save exists */
    static UHorizonSessionSave* LoadFromDisk();

    /** Delete saved session from disk */
    static void DeleteFromDisk();

    /** Check if a saved session exists */
    static bool HasSavedSession();
};
