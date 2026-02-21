// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "HorizonAsync_Connect.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHorizonAsyncConnectSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHorizonAsyncConnectFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Connect to the horizOn server.
 *
 * Resolves the best available backend host via ping-based selection.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_Connect : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnHorizonAsyncConnectSuccess OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnHorizonAsyncConnectFailure OnFailure;

	/** Connect to the horizOn backend server. */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Connect to horizOn Server"), Category = "horizOn")
	static UHorizonAsync_Connect* ConnectToServer(const UObject* WorldContextObject);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;

	UFUNCTION()
	void HandleSuccess();

	UFUNCTION()
	void HandleFailure(const FString& ErrorMessage);

	void Cleanup();
};
