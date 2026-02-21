// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Managers/HorizonNewsManager.h"
#include "HorizonSDKModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ============================================================
// Initialization
// ============================================================

void UHorizonNewsManager::Initialize(UHorizonHttpClient* InHttpClient)
{
	HttpClient = InHttpClient;
	UE_LOG(LogHorizonSDK, Log, TEXT("HorizonNewsManager initialized."));
}

// ============================================================
// LoadNews
// ============================================================

void UHorizonNewsManager::LoadNews(int32 Limit, const FString& LanguageCode, bool bUseCache, FOnNewsComplete OnComplete)
{
	// Cap limit at 100
	Limit = FMath::Clamp(Limit, 1, 100);

	// Check cache validity
	if (bUseCache && CachedNews.Num() > 0)
	{
		const double Now = FPlatformTime::Seconds();
		if ((Now - CacheTimestamp) < CacheTTLSeconds)
		{
			UE_LOG(LogHorizonSDK, Verbose, TEXT("News::LoadNews -- Returning %d cached entries."), CachedNews.Num());
			OnComplete.ExecuteIfBound(true, CachedNews);
			return;
		}
	}

	const FString Endpoint = FString::Printf(TEXT("api/v1/app/news?limit=%d&languageCode=%s"), Limit, *LanguageCode);

	TWeakObjectPtr<UHorizonNewsManager> WeakSelf(this);
	FOnNewsComplete CapturedOnComplete = OnComplete;

	HttpClient->Get(Endpoint, false,
		FOnHttpResponse::CreateLambda(
			[WeakSelf, CapturedOnComplete](const FHorizonNetworkResponse& Response)
			{
				UHorizonNewsManager* Self = WeakSelf.Get();
				if (!Self)
				{
					return;
				}

				if (!Response.bSuccess)
				{
					UE_LOG(LogHorizonSDK, Warning, TEXT("News::LoadNews -- Failed: %s"), *Response.ErrorMessage);
					CapturedOnComplete.ExecuteIfBound(false, TArray<FHorizonNewsEntry>());
					return;
				}

				TArray<FHorizonNewsEntry> Entries;

				// The response is a JSON array, not an object.
				// We need to parse the raw response body as an array.
				if (Response.JsonData.IsValid())
				{
					// If the HTTP client wrapped it in an object, try the array directly
					// Check if there's a wrapper array field
					const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;

					// Try common wrapper patterns
					if (Response.JsonData->TryGetArrayField(TEXT("items"), ItemsArray) ||
						Response.JsonData->TryGetArrayField(TEXT("news"), ItemsArray) ||
						Response.JsonData->TryGetArrayField(TEXT("entries"), ItemsArray))
					{
						for (const TSharedPtr<FJsonValue>& Value : *ItemsArray)
						{
							if (Value.IsValid() && Value->Type == EJson::Object)
							{
								Entries.Add(FHorizonNewsEntry::FromJson(Value->AsObject()));
							}
						}
					}
				}

				// If we got no entries from object parsing, try parsing BinaryData as a JSON array
				if (Entries.Num() == 0 && Response.BinaryData.Num() > 0)
				{
					FString RawBody;
					const FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Response.BinaryData.GetData()), Response.BinaryData.Num());
					RawBody = FString(Converter.Length(), Converter.Get());

					TArray<TSharedPtr<FJsonValue>> JsonArray;
					TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawBody);
					if (FJsonSerializer::Deserialize(Reader, JsonArray))
					{
						for (const TSharedPtr<FJsonValue>& Value : JsonArray)
						{
							if (Value.IsValid() && Value->Type == EJson::Object)
							{
								Entries.Add(FHorizonNewsEntry::FromJson(Value->AsObject()));
							}
						}
					}
				}

				Self->CachedNews = Entries;
				Self->CacheTimestamp = FPlatformTime::Seconds();

				UE_LOG(LogHorizonSDK, Log, TEXT("News::LoadNews -- Loaded %d news entries."), Entries.Num());
				CapturedOnComplete.ExecuteIfBound(true, Entries);
			}
		));
}

// ============================================================
// GetNewsById
// ============================================================

FHorizonNewsEntry UHorizonNewsManager::GetNewsById(const FString& Id) const
{
	for (const FHorizonNewsEntry& Entry : CachedNews)
	{
		if (Entry.Id == Id)
		{
			return Entry;
		}
	}
	return FHorizonNewsEntry();
}

// ============================================================
// Cache
// ============================================================

void UHorizonNewsManager::ClearCache()
{
	CachedNews.Empty();
	CacheTimestamp = 0.0;
	UE_LOG(LogHorizonSDK, Verbose, TEXT("News cache cleared."));
}
