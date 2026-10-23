// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.
#include "AsyncActions/HorizonAsync_Statistics.h"
#include "HorizonBlueprintLibrary.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonStatisticsManager.h"

UHorizonAsync_GetStatistics* UHorizonAsync_GetStatistics::GetStatistics(const UObject* WorldContextObject)
{
	auto* Action = NewObject<UHorizonAsync_GetStatistics>(); Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject); return Action;
}
void UHorizonAsync_GetStatistics::Activate()
{
	auto* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Statistics) { OnFailure.Broadcast(TEXT("SDK_NOT_CONNECTED"), TEXT("Statistics manager unavailable")); SetReadyToDestroy(); return; }
	Subsystem->Statistics->GetStatistics(FOnStatisticsLoaded::CreateUObject(this, &UHorizonAsync_GetStatistics::Complete));
}
void UHorizonAsync_GetStatistics::Complete(bool bSuccess, const FHorizonStatisticsSnapshot& Result, const FString& Code, const FString& Message)
{
	if (bSuccess) OnSuccess.Broadcast(Result); else OnFailure.Broadcast(Code, Message);
	SetReadyToDestroy();
}

UHorizonAsync_GetStatisticsDefinitions* UHorizonAsync_GetStatisticsDefinitions::GetStatisticsDefinitions(const UObject* WorldContextObject)
{
	auto* Action = NewObject<UHorizonAsync_GetStatisticsDefinitions>(); Action->WorldContext = WorldContextObject;
	Action->RegisterWithGameInstance(WorldContextObject); return Action;
}
void UHorizonAsync_GetStatisticsDefinitions::Activate()
{
	auto* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Statistics) { OnFailure.Broadcast(TEXT("SDK_NOT_CONNECTED"), TEXT("Statistics manager unavailable")); SetReadyToDestroy(); return; }
	Subsystem->Statistics->GetDefinitions(FOnStatisticsDefinitionsLoaded::CreateUObject(this, &UHorizonAsync_GetStatisticsDefinitions::Complete));
}
void UHorizonAsync_GetStatisticsDefinitions::Complete(bool bSuccess, const FHorizonStatisticsCatalog& Result, const FString& Code, const FString& Message)
{
	if (bSuccess) OnSuccess.Broadcast(Result); else OnFailure.Broadcast(Code, Message);
	SetReadyToDestroy();
}

UHorizonAsync_UpdateStatistics* UHorizonAsync_UpdateStatistics::UpdateStatistics(const UObject* WorldContextObject, const TArray<FHorizonStatisticUpdate>& Updates, const FString& OperationId)
{
	auto* Action = NewObject<UHorizonAsync_UpdateStatistics>(); Action->WorldContext = WorldContextObject;
	Action->OriginalUpdates = Updates; Action->OriginalOperationId = OperationId;
	Action->RegisterWithGameInstance(WorldContextObject); return Action;
}
void UHorizonAsync_UpdateStatistics::Activate()
{
	auto* Subsystem = UHorizonBlueprintLibrary::GetHorizonSubsystem(WorldContext.Get());
	if (!Subsystem || !Subsystem->Statistics) { OnFailure.Broadcast(TEXT("SDK_NOT_CONNECTED"), TEXT("Statistics manager unavailable")); SetReadyToDestroy(); return; }
	Subsystem->Statistics->UpdateStatistics(OriginalUpdates, OriginalOperationId, FOnStatisticsLoaded::CreateUObject(this, &UHorizonAsync_UpdateStatistics::Complete));
}
void UHorizonAsync_UpdateStatistics::Complete(bool bSuccess, const FHorizonStatisticsSnapshot& Result, const FString& Code, const FString& Message)
{
	if (bSuccess) OnSuccess.Broadcast(Result); else OnFailure.Broadcast(Code, Message);
	SetReadyToDestroy();
}
