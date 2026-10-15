// Copyright (c) 2025-2026 horizOn. All rights reserved.
#include "Examples/HorizonStatisticsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonStatisticsManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
#include "HorizonSessionSave.h"

AHorizonStatisticsExample::AHorizonStatisticsExample() { PrimaryActorTick.bCanEverTick = false; }
void AHorizonStatisticsExample::BeginPlay()
{
	Super::BeginPlay(); auto* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonStatisticsExample::Connected); Horizon->ConnectToServer();
}
void AHorizonStatisticsExample::Connected()
{
	auto* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
	if (Horizon->Auth->IsSignedIn()) { Authenticated(true); return; }
	if (!UHorizonSessionSave::HasSavedSession())
	{
		Horizon->Auth->SignUpAnonymous(TEXT("StatisticsExample"), FOnAuthComplete::CreateUObject(this, &AHorizonStatisticsExample::Authenticated)); return;
	}
	TWeakObjectPtr<AHorizonStatisticsExample> Weak(this);
	Horizon->Auth->RestoreAnonymousSession(FOnAuthComplete::CreateLambda([Weak](bool Success)
	{
		if (auto* Self = Weak.Get())
		{
			// A failed restore can be transient. Preserve its credential and pending intent.
			Self->Authenticated(Success);
		}
	}));
}
void AHorizonStatisticsExample::Authenticated(bool Success)
{
	if (!Success) { UE_LOG(LogTemp, Error, TEXT("[StatisticsExample] Sign-in failed")); return; }
	GetGameInstance()->GetSubsystem<UHorizonSubsystem>()->Statistics->GetStatistics(FOnStatisticsLoaded::CreateUObject(this, &AHorizonStatisticsExample::Loaded));
}
void AHorizonStatisticsExample::Loaded(bool Success, const FHorizonStatisticsSnapshot& Snapshot, const FString& Code, const FString&)
{
	if (!Success) { UE_LOG(LogTemp, Error, TEXT("[StatisticsExample] Snapshot failed: %s"), *Code); return; }
	auto* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
	Slot = TEXT("horizon-statistics-") + FMD5::HashAnsiString(*(Horizon->GetHttpClient()->GetApiKey() + TEXT(":") + Snapshot.UserId));
	const bool bExisting = UGameplayStatics::DoesSaveGameExist(Slot, 0);
	Pending = Cast<UHorizonStatisticsPendingBatch>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (bExisting && (!Pending || Pending->UserId != Snapshot.UserId)) { UE_LOG(LogTemp, Error, TEXT("[StatisticsExample] Pending intent needs recovery; preserving it")); return; }
	if (!Pending)
	{
		Pending = Cast<UHorizonStatisticsPendingBatch>(UGameplayStatics::CreateSaveGameObject(UHorizonStatisticsPendingBatch::StaticClass()));
		Pending->UserId = Snapshot.UserId; Pending->OperationId = Horizon->Statistics->CreateOperationId();
		FHorizonStatisticUpdate Kills; Kills.Key = TEXT("kills"); Kills.Value = 3;
		FHorizonStatisticUpdate Score; Score.Key = TEXT("best_score"); Score.Value = 1200;
		Pending->Updates = {Kills, Score};
		if (!UGameplayStatics::SaveGameToSlot(Pending, Slot, 0)) { UE_LOG(LogTemp, Error, TEXT("[StatisticsExample] Could not persist batch; nothing sent")); return; }
	}
	Horizon->Statistics->UpdateStatistics(Pending->Updates, Pending->OperationId, FOnStatisticsLoaded::CreateUObject(this, &AHorizonStatisticsExample::Applied));
}
void AHorizonStatisticsExample::Applied(bool Success, const FHorizonStatisticsSnapshot& Snapshot, const FString& Code, const FString&)
{
	if (!Success) { UE_LOG(LogTemp, Warning, TEXT("[StatisticsExample] Pending original batch retained: %s"), *Code); return; }
	if (!UGameplayStatics::DeleteGameInSlot(Slot, 0)) UE_LOG(LogTemp, Warning, TEXT("[StatisticsExample] Receipt accepted; saved intent retained for safe replay"));
	UE_LOG(LogTemp, Log, TEXT("[StatisticsExample] Revision %lld, replayed %s"), Snapshot.Revision, Snapshot.bReplayed ? TEXT("true") : TEXT("false"));
}
