#pragma once

#include "CoreMinimal.h"
#include "HorizonTypes.h"
#include "HorizonUserData.generated.h"

class FJsonObject;

USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonUserData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString UserId;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString Email;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    EHorizonAuthType AuthType = EHorizonAuthType::Anonymous;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    FString AccessToken;

    UPROPERTY()
    FString AnonymousToken;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    bool bIsEmailVerified = false;

    UPROPERTY(BlueprintReadOnly, Category = "horizOn|User")
    bool bIsAnonymous = false;

    bool IsValid() const { return !UserId.IsEmpty() && !AccessToken.IsEmpty(); }

    void Clear();

    /** Populate from auth response JSON. Field names from server:
     * userId, username, email, accessToken, anonymousToken, authStatus, isAnonymous, isVerified, googleId */
    void UpdateFromAuthResponse(const TSharedPtr<FJsonObject>& JsonObject);
};
