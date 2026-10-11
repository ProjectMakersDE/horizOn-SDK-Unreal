// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonCloudSaveExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonCloudSaveManager.h"
#include "Engine/GameInstance.h"

AHorizonCloudSaveExample::AHorizonCloudSaveExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonCloudSaveExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonCloudSaveExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonCloudSaveExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[CloudSaveExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonCloudSaveExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonCloudSaveExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->CloudSave)
	{
		UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] Auth or CloudSave manager not available."));
		return;
	}

	UHorizonCloudSaveManager* CloudSave = Horizon->CloudSave;

	// Cloud save operations require a signed-in user.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[CloudSave](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] FAILED: sign-up did not complete."));
				return;
			}

			const FString Payload = TEXT("{\"level\":3,\"coins\":120}");
			UE_LOG(LogTemp, Log, TEXT("[CloudSaveExample] Saving data..."));

			CloudSave->Save(Payload, FOnRequestComplete::CreateLambda(
				[CloudSave](bool bSaveSuccess, const FString& ErrorMessage)
				{
					if (!bSaveSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] FAILED to save: %s"), *ErrorMessage);
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[CloudSaveExample] Saved. Loading it back..."));

					CloudSave->Load(FOnStringComplete::CreateLambda(
						[CloudSave](bool bLoadSuccess, const FString& Data)
						{
							if (!bLoadSuccess)
							{
								UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] FAILED to load: %s"), *Data);
								return;
							}

							UE_LOG(LogTemp, Log, TEXT("[CloudSaveExample] SUCCESS: loaded %s"), *Data);

							// Several devices: load a snapshot with its revision, then save only while the
							// cloud save is still at that revision. On a conflict another device saved
							// first: nothing was written, load again and decide.
							CloudSave->LoadSnapshot(FOnCloudSaveSnapshotComplete::CreateLambda(
								[CloudSave](const FHorizonCloudSaveSnapshot& Snapshot)
								{
									if (!Snapshot.bSuccess)
									{
										UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] FAILED to load snapshot: %s"), *Snapshot.ErrorMessage);
										return;
									}

									// Snapshot.Data is {"level":3,"coins":120}. A game builds the new save from
									// Snapshot.Data: only the snapshot's data belongs to its revision.
									const FString Updated = TEXT("{\"level\":3,\"coins\":130}");
									CloudSave->Save(Updated, Snapshot.Revision, FOnCloudSaveWriteComplete::CreateLambda(
										[](const FHorizonCloudSaveWriteResult& Result)
										{
											if (Result.IsConflict())
											{
												UE_LOG(LogTemp, Warning, TEXT("[CloudSaveExample] Another device saved first. Load the new snapshot and merge."));
											}
											else if (Result.IsSaved())
											{
												UE_LOG(LogTemp, Log, TEXT("[CloudSaveExample] Saved at revision %lld."), Result.Revision);
											}
											else
											{
												UE_LOG(LogTemp, Error, TEXT("[CloudSaveExample] FAILED to save: %s"), *Result.ErrorMessage);
											}
										}));
								}));
						}));
				}));
		}));
}
