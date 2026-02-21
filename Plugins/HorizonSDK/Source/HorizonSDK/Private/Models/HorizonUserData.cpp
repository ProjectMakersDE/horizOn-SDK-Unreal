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

    // Determine auth type from response
    if (bIsAnonymous)
    {
        AuthType = EHorizonAuthType::Anonymous;
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
