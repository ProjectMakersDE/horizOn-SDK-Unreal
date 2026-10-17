#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonValidatedActions.h"
#include "HorizonMatches.generated.h"
class FJsonObject;
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchLimits
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 ActiveMatches=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 StateBytes=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 RetainedMatches=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PendingInvites=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 SettlementMatches=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 HistoryRetentionSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 RankedSettlementSeconds=0;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchCapabilities
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bMatches=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bMatchesRanked=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bRealtime=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchConfig
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasRankedLeaderboardKey=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString RankedLeaderboardKey;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchSummary
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString Id;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString Name;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString CreatorId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 Capacity=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString Visibility;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 Strength=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString Status;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FString> ParticipantIds;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FString> InvitedIds;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FString> ForfeitedIds;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasCurrentPlayerId=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString CurrentPlayerId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasTurnDeadline=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString TurnDeadline;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString CreatedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasStartedAt=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString StartedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasFinishedAt=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString FinishedAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasPurgeAt=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString PurgeAt;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasFinalStateRevision=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 FinalStateRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasDisagreement=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bRanked=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bTimeoutDue=false;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchReport
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 FinalStateRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<uint8> Result;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FString> WinnerIds;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ReportedAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchView : public FHorizonMatchSummary
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FString> TurnOrder;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 TurnSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString TimeoutPolicy;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasData=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<uint8> Data;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FHorizonMatchReport> Reports;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasReportDeadline=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ReportDeadline;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasAbsoluteDeadline=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString AbsoluteDeadline;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasRankedLeaderboardKey=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString RankedLeaderboardKey;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasOwnRankedRunId=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString OwnRankedRunId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bNotificationPending=false;
};
/** Minimal immutable settlement source; never reconstructs a purged match or its rewards. */
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchPendingResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString MatchId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 FinalStateRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasOwnRankedRunId=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString OwnRankedRunId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ExpiresAt;
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMyMatches
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString UserId;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 Revision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FHorizonMatchSummary> Active;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FHorizonMatchSummary> Invites;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FHorizonMatchSummary> Recent;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FHorizonMatchPendingResult> PendingResults;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMyMatches& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchLimitsView
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchLimits Limits;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchConfig Config;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchCapabilities Capabilities;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMatchLimitsView& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchPage
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") TArray<FHorizonMatchSummary> Matches;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bHasNextCursor=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString NextCursor;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMatchPage& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchRead
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchView Match;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMatchRead& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchCreated
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchView Match;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int64 PlayerRevision=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMatchCreated& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchReported
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bPresent=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FHorizonMatchView Match;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") bool bReplayed=false;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") int32 PollAfterSeconds=0;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Matches") FString ServerTime;
    static bool TryFromJson(const TSharedPtr<FJsonObject>& Json,FHorizonMatchReported& Out);
};
USTRUCT(BlueprintType)
struct HORIZONSDK_API FHorizonMatchCreateInput
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") FString Name;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") int32 Capacity=2;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") FString Visibility=TEXT("private");
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") int64 Strength=0;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") int32 TurnSeconds=60;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") FString TimeoutPolicy=TEXT("skip");
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") bool bRanked=false;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") TArray<uint8> Data;
    UPROPERTY(BlueprintReadWrite,Category="horizOn|Matches") TArray<FString> InviteeIds;
};
DECLARE_DELEGATE_FourParams(FOnMatchesComplete,bool,const FHorizonMyMatches&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnMatchLimitsComplete,bool,const FHorizonMatchLimitsView&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnOpenMatchesComplete,bool,const FHorizonMatchPage&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnMatchComplete,bool,const FHorizonMatchRead&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnMatchCreatedComplete,bool,const FHorizonMatchCreated&,const FString&,const FString&);
DECLARE_DELEGATE_FourParams(FOnMatchReportedComplete,bool,const FHorizonMatchReported&,const FString&,const FString&);
