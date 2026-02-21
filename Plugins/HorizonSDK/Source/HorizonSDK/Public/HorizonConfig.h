#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "HorizonTypes.h"
#include "HorizonConfig.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "horizOn SDK"))
class HORIZONSDK_API UHorizonConfig : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UHorizonConfig();

    /** API key from horizOn dashboard */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (DisplayName = "API Key"))
    FString ApiKey;

    /** Backend host URLs. Single host = direct connection. Multiple = ping-based selection. */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (DisplayName = "Backend Hosts"))
    TArray<FString> Hosts;

    /** Connection timeout in seconds */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (ClampMin = "5", ClampMax = "60"))
    int32 ConnectionTimeoutSeconds = 10;

    /** Maximum retry attempts for failed requests */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (ClampMin = "0", ClampMax = "10"))
    int32 MaxRetryAttempts = 3;

    /** Delay between retries in seconds */
    UPROPERTY(Config, EditAnywhere, Category = "Connection", meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float RetryDelaySeconds = 1.0f;

    /** Minimum log level for SDK messages */
    UPROPERTY(Config, EditAnywhere, Category = "Logging")
    EHorizonLogLevel LogLevel = EHorizonLogLevel::Info;

    /** Get the CDO config instance */
    static const UHorizonConfig* Get();

    virtual FName GetCategoryName() const override;
};
