// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Managers/HorizonValidatedActionsManager.h"
#include "Models/HorizonMatches.h"
#include "HorizonMatchesManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMatchesResult,bool,bSuccess,const FHorizonMyMatches&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMatchLimitsResult,bool,bSuccess,const FHorizonMatchLimitsView&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonOpenMatchesResult,bool,bSuccess,const FHorizonMatchPage&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMatchReadResult,bool,bSuccess,const FHorizonMatchRead&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMatchCreateResult,bool,bSuccess,const FHorizonMatchCreated&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMatchReportResult,bool,bSuccess,const FHorizonMatchReported&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonMatchRunResult,bool,bSuccess,const FHorizonValidatedRun&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
class UHorizonRealtimeManager;
/** Explicit asynchronous matches. The server validates turns and consensus, never the game's combat. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonMatchesManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth,UHorizonValidatedActionsManager* Validated);
    void SetRealtime(UHorizonRealtimeManager* Realtime);
    void GetMatches(const FString& UserId,FOnMatchesComplete Complete);
    void GetLimits(const FString& UserId,FOnMatchLimitsComplete Complete);
    void FindOpenMatches(const FString& UserId,int64 MinStrength,int64 MaxStrength,FOnOpenMatchesComplete Complete,int32 Limit=20,const FString& Cursor=TEXT(""));
    void GetMatch(const FString& UserId,const FString& MatchId,FOnMatchComplete Complete);
    void CreateMatch(const FString& UserId,int64 ExpectedPlayerRevision,const FHorizonMatchCreateInput& Input,FOnMatchCreatedComplete Complete);
    void JoinMatch(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,FOnMatchComplete Complete);
    void DeclineInvite(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,FOnMatchComplete Complete);
    void StartMatch(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,const TArray<FString>& TurnOrder,FOnMatchComplete Complete);
    void SubmitTurn(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,const TArray<uint8>& Data,bool bFinish,FOnMatchComplete Complete);
    void AdvanceTimeout(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,FOnMatchComplete Complete);
    void LeaveMatch(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,FOnMatchComplete Complete);
    void CancelMatch(const FString& UserId,const FString& MatchId,int64 ExpectedRevision,FOnMatchComplete Complete);
    void ReportResult(const FString& UserId,const FString& MatchId,int64 FinalStateRevision,const TArray<uint8>& Result,const TArray<FString>& WinnerIds,FOnMatchReportedComplete Complete);
    /** Adopts only the returned original match-bound run into the existing Validated Actions manager. */
    void StartRankedRun(const FString& UserId,const FString& MatchId,int64 FinalStateRevision,FOnValidatedRunStarted Complete);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void ClearCache();
    uint64 GetCacheGeneration();
    UFUNCTION(BlueprintPure,Category="horizOn|Matches") FHorizonMyMatches GetCurrentMatches();
    UFUNCTION(BlueprintPure,Category="horizOn|Matches") FHorizonMatchLimitsView GetCurrentLimits();
    UFUNCTION(BlueprintPure,Category="horizOn|Matches") FHorizonMatchPage GetCurrentOpenMatches();
    UFUNCTION(BlueprintPure,Category="horizOn|Matches") FHorizonMatchRead GetCurrentMatch();
    UFUNCTION(BlueprintPure,Category="horizOn|Matches") FHorizonMatchCreated GetCurrentCreated();
    UFUNCTION(BlueprintPure,Category="horizOn|Matches") FHorizonMatchReported GetCurrentReported();
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void RequestMatches(const FString& UserId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void RequestMatchLimits(const FString& UserId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void RequestOpenMatches(const FString& UserId, int64 MinStrength, int64 MaxStrength, int32 Limit, const FString& Cursor);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void RequestMatch(const FString& UserId, const FString& MatchId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void CreatePlayerMatch(const FString& UserId, int64 ExpectedPlayerRevision, const FHorizonMatchCreateInput& Input);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void JoinPlayerMatch(const FString& UserId, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void DeclineMatchInvite(const FString& UserId, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void AdvanceMatchTimeout(const FString& UserId, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void LeavePlayerMatch(const FString& UserId, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void CancelPlayerMatch(const FString& UserId, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void StartPlayerMatch(const FString& UserId, const FString& MatchId, int64 ExpectedRevision, const TArray<FString>& TurnOrder);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void SubmitPlayerTurn(const FString& UserId, const FString& MatchId, int64 ExpectedRevision, const TArray<uint8>& Data, bool bFinish);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void ReportMatchResult(const FString& UserId, const FString& MatchId, int64 FinalStateRevision, const TArray<uint8>& Result, const TArray<FString>& WinnerIds);
    UFUNCTION(BlueprintCallable,Category="horizOn|Matches") void RequestRankedMatchRun(const FString& UserId, const FString& MatchId, int64 FinalStateRevision);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonMatchesResult OnMatchesResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonMatchLimitsResult OnMatchLimitsResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonOpenMatchesResult OnOpenMatchesResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonMatchReadResult OnMatchReadResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonMatchCreateResult OnMatchCreateResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonMatchReportResult OnMatchReportResult;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Matches") FOnHorizonMatchRunResult OnMatchRunResult;
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    UPROPERTY() UHorizonValidatedActionsManager* ValidatedActions=nullptr;
    UPROPERTY() UHorizonRealtimeManager* Realtime=nullptr;
    UFUNCTION() void OnRealtimeEvent(const FString& EventJson);
    UFUNCTION() void OnRealtimeTopic(const FString& Topic);
    FString Identity,Scope,AdoptedRunId,AdoptedMatchId;
    int64 AdoptedFinalRevision=-1;
    FHorizonValidatedRun AdoptedOriginalRun;
    TMap<FString,FString> SourceRuns;
    bool SourceMatches(const FString& MatchId,int64 FinalRevision,const FString& RunId) const;
    void RememberSource(const FString& MatchId,int64 FinalRevision,const FString& RunId);
    uint64 Context=0,Epoch=0,Ticket=0;
    bool Busy=false;
    int64 ServerMillis=-1;
    TMap<FString,int64> RevisionFloors;
    FHorizonMyMatches CurrentMatches;
    FHorizonMatchLimitsView CurrentLimits;
    FHorizonMatchPage CurrentOpen;
    FHorizonMatchRead CurrentMatch;
    FHorizonMatchCreated CurrentCreated;
    FHorizonMatchReported CurrentReported;
    void Sync();
    void ClearSnapshots();
    bool Accept(const FString& ServerTime,const TMap<FString,int64>& Versions={});
    void Call(const FString& User,const FString& Route,const FString& Verb,const TSharedPtr<FJsonObject>& Body,int64 OriginalRevision,FOnHttpResponse Complete);
    void Action(const FString& User,const FString& MatchId,int64 OriginalRevision,const FString& Action,const TSharedPtr<FJsonObject>& Body,FOnMatchComplete Complete);
    static FHorizonNetworkResponse Error(const FString& Code);
};
