#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonMatches.h"
#include "HorizonMatchesExample.generated.h"
class UHorizonSubsystem;
/** One durable project/player original. Hosts and sessions never enter the pending namespace. */
UCLASS()
class HORIZONSDK_API UHorizonMatchOriginal : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString Project;
    UPROPERTY() FString UserId;
    UPROPERTY() FString IntentId;
    UPROPERTY() FString Action;
    UPROPERTY() FString MatchId;
    UPROPERTY() int64 ExpectedRevision=-1;
    UPROPERTY() FHorizonMatchCreateInput Create;
    UPROPERTY() FString CreateNameBase64;
    UPROPERTY() TArray<uint8> Data;
    UPROPERTY() TArray<FString> Ids;
    UPROPERTY() bool bFinish=false;
    UPROPERTY() int64 SendSerial=0;
    UPROPERTY() FString OriginalHash;
    UPROPERTY() bool bConfirmed=false;
    UPROPERTY() FString ResolvedMatchId;
    UPROPERTY() FHorizonValidatedRun Run;
    UPROPERTY() FString RunBoardBase64;
    UPROPERTY() bool bSubmitPrepared=false;
    UPROPERTY() int64 Score=0;
    UPROPERTY() TArray<uint8> InputLog;
    UPROPERTY() FString SubmitHash;
    UPROPERTY() int64 StorageGeneration=0;
    UPROPERTY() FString StorageHash;
    UPROPERTY() FString RejectionCode;
    UPROPERTY() FString TerminalCode;
    UPROPERTY() bool bSubmitConfirmed=false;
};
/** Existing session, explicit player actions only. Use one Actor in each of two authenticated clients. */
UCLASS()
class HORIZONSDK_API AHorizonMatchesExample : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void Refresh(const FString& MatchId=TEXT(""));
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void FindOpen(int64 MinStrength,int64 MaxStrength,const FString& Cursor=TEXT(""));
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") bool WatchMatch(const FString& MatchId);
    /** Prefixes the reviewed name with this sample's unique recovery nonce; leaves opaque game bytes unchanged. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void CreateLobby(const FHorizonMatchCreateInput& Input);
    /** join/decline/start/turn/timeout/leave/cancel/result/ranked. Ids are turn order or sorted winners. Result/ranked revision is the original finalStateRevision. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void StartAction(const FString& Action,const FString& MatchId,int64 OriginalRevision,const TArray<uint8>& Data,const TArray<FString>& Ids,bool bFinish=false);
    /** Uses a reviewed minimal pending source after full match detail has expired. Persists before its single POST. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void StartRankedFromPendingResult(const FString& MatchId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void RetryOriginal();
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") bool ConfirmRecoveredCreate(const FString& MatchId);
    /** A deliberate acknowledgement after reading the exact original match; never infers a game winner or value grant. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") bool AcknowledgeAfterRefresh();
    /** Uses only the already returned original match-bound run. Same saved score/input after uncertainty. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches Example") void SubmitRankedResult(int64 Score,const TArray<uint8>& InputLog);
    UFUNCTION(BlueprintPure,Category="horizOn|Matches Example") FHorizonMyMatches GetMatches() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Matches Example") FHorizonMatchRead GetMatch() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Matches Example") FHorizonMatchPage GetOpenMatches() const;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches Example") FString LastErrorCode;
private:
    friend struct FHorizonConfiguredNative;
    friend struct FHorizonMatchesNative;
    UHorizonSubsystem* Session() const;
    bool Fresh() const;
    bool AllowRead(const FString& Key,bool Continuation,uint64 Context);
    void MarkRead(const FString& Key,int32 Poll);
    UHorizonMatchOriginal* Prepare(const FString& Action,const FString& MatchId,int64 Revision,const FHorizonMatchCreateInput& Create,const TArray<uint8>& Data,const TArray<FString>& Ids,bool Finish);
    void Send(UHorizonMatchOriginal* Original,const FString& Slot);
    bool Busy=false,bRefreshedNotFound=false;
    uint64 SnapshotContext=0,SnapshotGeneration=0;
    FString OriginalAtRefresh,RefreshedMatchId;
    TMap<FString,double> RefreshAfter;
    FHorizonMyMatches MyMatches;
    FHorizonMatchRead Match;
    FHorizonMatchPage Open;
};
