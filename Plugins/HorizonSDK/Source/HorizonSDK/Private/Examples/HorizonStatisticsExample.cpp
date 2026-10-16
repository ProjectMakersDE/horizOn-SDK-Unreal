// Copyright (c) 2025-2026 horizOn. All rights reserved.
#include "Examples/HorizonStatisticsExample.h"
#include "HorizonSubsystem.h"
#include "Managers/HorizonStatisticsManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/SecureHash.h"
#include "HorizonSessionSave.h"
#include "Examples/HorizonExampleIdentity.h"
#include "Transport/HorizonStatisticsTransportContract.h"

AHorizonStatisticsExample::AHorizonStatisticsExample() { PrimaryActorTick.bCanEverTick = false; }
void AHorizonStatisticsExample::BeginPlay()
{
	Super::BeginPlay(); auto* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
	Horizon->OnConnected.AddUniqueDynamic(this, &AHorizonStatisticsExample::Connected); Horizon->ConnectToServer();
}
void AHorizonStatisticsExample::Connected()
{
    if (bAuthStarted) return; bAuthStarted=true;
    auto* Horizon=GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
    if (Horizon->Auth->IsSignedIn()) { Authenticated(true); return; }
    AuthProject=UHorizonExampleIdentity::ProjectKey(Horizon); bSignupSent=false;
    if (!UHorizonExampleIdentity::CaptureBeforeAuth(Horizon,AuthOriginal)) { Authenticated(false); return; }
    bRestoring=UHorizonExampleIdentity::ShouldRestoreSession(Horizon);
    if (bRestoring)
    {
        if (!UHorizonExampleIdentity::BeforeRestore(Horizon,AuthProject,true,AuthOriginal)) { Authenticated(false); return; }
        Horizon->Auth->RestoreAnonymousSession(FOnAuthComplete::CreateUObject(this,&AHorizonStatisticsExample::AuthResult));
    }
    else
    {
        if (!UHorizonExampleIdentity::BeforeSignup(Horizon,AuthProject)) { Authenticated(false); return; }
        bSignupSent=true;
        Horizon->Auth->SignUpAnonymous(TEXT("StatisticsExample"),FOnAuthComplete::CreateUObject(this,&AHorizonStatisticsExample::AuthResult),TEXT(""),FOnAnonymousIdentityIssued::CreateLambda([](const FString& Key,const FHorizonUserData& Issued) { return UHorizonExampleIdentity::RecordIssuedSignup(Key,Issued); }));
    }
}
void AHorizonStatisticsExample::AuthResult(bool Success)
{
    auto* SDK=GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
    const bool Accepted=UHorizonExampleIdentity::AfterAuth(SDK,AuthProject,AuthOriginal,bSignupSent,bRestoring,Success);
    AuthOriginal=nullptr; Authenticated(Accepted);
}
void AHorizonStatisticsExample::Authenticated(bool Success)
{
	if (!Success) { UE_LOG(LogTemp, Warning, TEXT("[StatisticsExample] Sign-in/recovery failed; original identity retained")); return; }
	GetGameInstance()->GetSubsystem<UHorizonSubsystem>()->Statistics->GetStatistics(FOnStatisticsLoaded::CreateUObject(this, &AHorizonStatisticsExample::Loaded));
}
void AHorizonStatisticsExample::Loaded(bool Success, const FHorizonStatisticsSnapshot& Snapshot, const FString& Code, const FString&)
{
	if (!Success) { UE_LOG(LogTemp, Error, TEXT("[StatisticsExample] Snapshot failed: %s"), *Code); return; }
	auto* Horizon = GetGameInstance()->GetSubsystem<UHorizonSubsystem>();
	Slot = TEXT("horizon-statistics-") + FMD5::HashAnsiString(*(Horizon->GetHttpClient()->GetApiKey() + TEXT(":") + Snapshot.UserId));
	const bool bExisting = UGameplayStatics::DoesSaveGameExist(Slot, 0);
	Pending = Cast<UHorizonStatisticsPendingBatch>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (bExisting && (!Pending || Pending->UserId != Snapshot.UserId || !HorizonTransportContract::IsStatisticsOperationId(TCHAR_TO_UTF8(*Pending->OperationId)) || Pending->Updates.Num()!=2 || Pending->Updates[0].Key!=TEXT("kills") || Pending->Updates[0].Value!=3 || Pending->Updates[1].Key!=TEXT("best_score") || Pending->Updates[1].Value!=1200)) { UE_LOG(LogTemp, Error, TEXT("[StatisticsExample] Pending intent needs recovery; preserving it")); return; }
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
    auto* Stored=Cast<UHorizonStatisticsPendingBatch>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    bool Matches=Pending && Stored && Pending->UserId==Stored->UserId && Pending->OperationId==Stored->OperationId && Pending->Updates.Num()==Stored->Updates.Num();
    if (Matches) for (int32 I=0;I<Pending->Updates.Num();++I) Matches=Matches && Pending->Updates[I].Key==Stored->Updates[I].Key && Pending->Updates[I].Value==Stored->Updates[I].Value;
	if (!Matches || !UGameplayStatics::DeleteGameInSlot(Slot, 0)) UE_LOG(LogTemp, Warning, TEXT("[StatisticsExample] Receipt accepted; saved intent retained for safe replay"));
	UE_LOG(LogTemp, Log, TEXT("[StatisticsExample] Revision %lld, replayed %s"), Snapshot.Revision, Snapshot.bReplayed ? TEXT("true") : TEXT("false"));
}
