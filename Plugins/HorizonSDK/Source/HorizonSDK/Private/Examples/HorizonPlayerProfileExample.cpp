// Copyright (c) 2025-2026 horizOn. All rights reserved.

#include "Examples/HorizonPlayerProfileExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonPlayerProfileManager.h"
#include "Engine/GameInstance.h"

AHorizonPlayerProfileExample::AHorizonPlayerProfileExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AHorizonPlayerProfileExample::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerProfileExample] horizOn subsystem not available."));
		return;
	}

	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonPlayerProfileExample::HandleConnected);
	Horizon->OnConnectionFailed.AddUniqueDynamic(this, &AHorizonPlayerProfileExample::HandleConnectionFailed);

	UE_LOG(LogTemp, Log, TEXT("[PlayerProfileExample] Connecting..."));
	Horizon->ConnectToServer();
}

void AHorizonPlayerProfileExample::HandleConnectionFailed(const FString& ErrorMessage)
{
	UE_LOG(LogTemp, Error, TEXT("[PlayerProfileExample] Connection failed: %s"), *ErrorMessage);
}

void AHorizonPlayerProfileExample::HandleConnected()
{
	UGameInstance* GameInstance = GetGameInstance();
	UHorizonSubsystem* Horizon = GameInstance ? GameInstance->GetSubsystem<UHorizonSubsystem>() : nullptr;
	if (!Horizon || !Horizon->Auth || !Horizon->PlayerProfile)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerProfileExample] Auth or PlayerProfile manager not available."));
		return;
	}

	UHorizonPlayerProfileManager* PlayerProfile = Horizon->PlayerProfile;

	// The player profile requires a signed-in player.
	Horizon->Auth->SignUpAnonymous(TEXT("ExamplePlayer"), FOnAuthComplete::CreateLambda(
		[PlayerProfile](bool bAuthSuccess)
		{
			if (!bAuthSuccess)
			{
				UE_LOG(LogTemp, Error, TEXT("[PlayerProfileExample] FAILED: sign-up did not complete."));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("[PlayerProfileExample] Loading profile..."));

			PlayerProfile->GetProfile(FOnPlayerProfileComplete::CreateLambda(
				[PlayerProfile](bool bSuccess, const FHorizonPlayerProfileResult& Result,
					const FString& ErrorCode, const FString& ErrorMessage)
				{
					if (!bSuccess)
					{
						UE_LOG(LogTemp, Error, TEXT("[PlayerProfileExample] FAILED to load profile (%s): %s"), *ErrorCode, *ErrorMessage);
						return;
					}

					const TArray<FHorizonCosmetic> Avatars = Result.GetCosmetics(TEXT("avatar"));
					UE_LOG(LogTemp, Log, TEXT("[PlayerProfileExample] Catalog has %d avatars, %d frames, %d badges."),
						Avatars.Num(), Result.GetCosmetics(TEXT("frame")).Num(), Result.GetCosmetics(TEXT("badge")).Num());

					const FHorizonCosmetic* FirstAvailable = Avatars.FindByPredicate(
						[](const FHorizonCosmetic& Cosmetic) { return Cosmetic.bAvailable; });
					if (!FirstAvailable)
					{
						UE_LOG(LogTemp, Warning, TEXT("[PlayerProfileExample] No available avatar in the catalog; add a free one in the Dashboard."));
						return;
					}

					UE_LOG(LogTemp, Log, TEXT("[PlayerProfileExample] Selecting avatar %s..."), *FirstAvailable->Id);

					// PUT replaces the whole profile: keep the current frame and badges.
					PlayerProfile->SetProfile(FirstAvailable->Id, Result.Profile.FrameId, Result.Profile.Badges,
						FOnPlayerProfileComplete::CreateLambda(
							[](bool bSetSuccess, const FHorizonPlayerProfileResult& Updated,
								const FString& SetErrorCode, const FString& SetErrorMessage)
							{
								if (!bSetSuccess)
								{
									// For example COSMETIC_LOCKED when the player does not own a locked cosmetic.
									UE_LOG(LogTemp, Error, TEXT("[PlayerProfileExample] FAILED to save profile (%s): %s"),
										*SetErrorCode, *SetErrorMessage);
									return;
								}

								UE_LOG(LogTemp, Log, TEXT("[PlayerProfileExample] SUCCESS: avatar %s, frame %s, %d badges"),
									*Updated.Profile.AvatarId, *Updated.Profile.FrameId, Updated.Profile.Badges.Num());
							}));
				}));
		}));
}
