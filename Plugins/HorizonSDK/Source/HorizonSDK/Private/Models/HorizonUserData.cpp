#include "Models/HorizonUserData.h"
#include "Dom/JsonObject.h"

void FHorizonUserData::Clear()
{
    UserId.Empty();
    Email.Empty();
    DisplayName.Empty();
    AuthType = EHorizonAuthType::Anonymous;
    AccessToken.Empty();
    AnonymousToken.Empty();
    bIsEmailVerified = false;
    bIsAnonymous = false;
    AppleUserId.Empty();
    bIsPrivateRelayEmail = false;
}

void FHorizonUserData::UpdateFromAuthResponse(const TSharedPtr<FJsonObject>& JsonObject)
{
    if (!JsonObject.IsValid()) return;

    UserId = JsonObject->GetStringField(TEXT("userId"));
    DisplayName = JsonObject->GetStringField(TEXT("username"));
    Email = JsonObject->GetStringField(TEXT("email"));
    AccessToken = JsonObject->GetStringField(TEXT("accessToken"));
    bIsAnonymous = JsonObject->GetBoolField(TEXT("isAnonymous"));
    bIsEmailVerified = JsonObject->GetBoolField(TEXT("isVerified"));

    // AnonymousToken may not be present in all responses
    if (JsonObject->HasField(TEXT("anonymousToken")))
    {
        AnonymousToken = JsonObject->GetStringField(TEXT("anonymousToken"));
    }

    // Apple-specific fields (non-breaking — default to empty/false for non-Apple users)
    if (JsonObject->HasField(TEXT("appleUserId")))
    {
        AppleUserId = JsonObject->GetStringField(TEXT("appleUserId"));
    }
    else
    {
        AppleUserId.Empty();
    }

    if (JsonObject->HasField(TEXT("isPrivateRelayEmail")))
    {
        bIsPrivateRelayEmail = JsonObject->GetBoolField(TEXT("isPrivateRelayEmail"));
    }
    else
    {
        bIsPrivateRelayEmail = false;
    }

    // Determine auth type from response
    if (bIsAnonymous)
    {
        AuthType = EHorizonAuthType::Anonymous;
    }
    else if (!AppleUserId.IsEmpty())
    {
        AuthType = EHorizonAuthType::Apple;
    }
    else if (JsonObject->HasField(TEXT("googleId")) && !JsonObject->GetStringField(TEXT("googleId")).IsEmpty())
    {
        AuthType = EHorizonAuthType::Google;
    }
    else
    {
        AuthType = EHorizonAuthType::Email;
    }
}
