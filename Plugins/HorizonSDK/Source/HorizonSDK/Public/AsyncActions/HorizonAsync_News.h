// Copyright (c) 2025-2026 horizOn. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonNewsEntry.h"
#include "HorizonAsync_News.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewsEntriesResult, const TArray<FHorizonNewsEntry>&, Entries);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewsAsyncFailure, const FString&, ErrorMessage);

/**
 * Async Blueprint node: Load news entries from the horizOn server.
 */
UCLASS()
class HORIZONSDK_API UHorizonAsync_News : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnNewsEntriesResult OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnNewsAsyncFailure OnFailure;

	/**
	 * Load news entries from the server.
	 * @param Limit         Maximum number of entries (capped at 100).
	 * @param LanguageCode  Language filter (e.g. "en", "de").
	 * @param bUseCache     Return cached data if within TTL.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DisplayName = "Load News"), Category = "horizOn|News")
	static UHorizonAsync_News* LoadNews(const UObject* WorldContextObject, int32 Limit, const FString& LanguageCode, bool bUseCache);

	virtual void Activate() override;

private:
	TWeakObjectPtr<const UObject> WorldContext;
	int32 LimitValue;
	FString Language;
	bool bCache;

	void HandleResult(bool bSuccess, const TArray<FHorizonNewsEntry>& Entries);
};
