#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HorizonExampleIdentity.generated.h"
class UHorizonSubsystem;
class UHorizonSessionSave;
struct FHorizonUserData;
/** Example-only local recovery guard. No HTTP and no replacement identities after uncertain signup. */
UCLASS()
class HORIZONSDK_API UHorizonExampleIdentity : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString ProjectHash;
    UPROPERTY() FString UserId;
    UPROPERTY() FString AccessToken;
    UPROPERTY() FString AnonymousToken;
    UPROPERTY() FString DisplayName;
    UPROPERTY() FString PreviousCacheHash;
    UPROPERTY() bool bAnonymous=true;
    UPROPERTY() bool bSignupUncertain=false;
    static FString Project(const FString& Value);
    static FString ProjectKey(UHorizonSubsystem* SDK);
    static FString Slot(const FString& ApiKey);
    static bool ShouldRestoreGuest(UHorizonSubsystem* SDK);
    static bool ShouldRestoreSession(UHorizonSubsystem* SDK);
    static bool RecordIssuedSignup(const FString& ApiKey,const FHorizonUserData& Issued);
    static bool CaptureBeforeAuth(UHorizonSubsystem* SDK,UHorizonExampleIdentity*& Original);
    static bool BeforeSignup(UHorizonSubsystem* SDK,const FString& ApiKey);
    static bool BeforeRestore(UHorizonSubsystem* SDK,const FString& ApiKey,bool GuestOnly,UHorizonExampleIdentity*& Original);
    static bool AfterAuth(UHorizonSubsystem* SDK,const FString& ApiKey,UHorizonExampleIdentity* Original,bool SignupSent,bool Restoring,bool Success);
    FString CacheHash() const;
private:
    bool Valid() const;
    bool Same(const UHorizonExampleIdentity* Other) const;
    bool Stage(bool Force=false) const;
    bool Store(const FString& Name) const;
    bool Backup() const;
    static UHorizonExampleIdentity* Read(const FString& Name,bool& Blocked);
    static UHorizonExampleIdentity* Cache();
    static UHorizonExampleIdentity* Bound(const FString& ApiKey,const UHorizonExampleIdentity* Record);
    static bool SaveGuest(const FString& ApiKey,const UHorizonExampleIdentity* Record);
};
