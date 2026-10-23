// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonMatches.h"
#include "HorizonAsync_Matches.generated.h"
class UHorizonSubsystem;
UCLASS(Abstract)
class HORIZONSDK_API UHorizonAsync_MatchesBase : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
protected:
    void Freeze(const UObject* ContextObject);
    UHorizonSubsystem* Resolve(FString& Code);
    TWeakObjectPtr<const UObject> World;
    TWeakObjectPtr<UHorizonSubsystem> OriginalSubsystem;
    FString User;
    uint64 Context=0;
    bool Activated=false;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMatchesAsyncResult,bool,bSuccess,const FHorizonMyMatches&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_Matches : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnMatchesAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_Matches* GetMatches(const UObject* WorldContextObject);
    virtual void Activate() override;
private:
    FString Operation;
    void Complete(bool Ok,const FHorizonMyMatches& Value,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMatchLimitsAsyncResult,bool,bSuccess,const FHorizonMatchLimitsView&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_MatchLimits : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnMatchLimitsAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchLimits* GetLimits(const UObject* WorldContextObject);
    virtual void Activate() override;
private:
    FString Operation;
    void Complete(bool Ok,const FHorizonMatchLimitsView& Value,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnOpenMatchesAsyncResult,bool,bSuccess,const FHorizonMatchPage&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_OpenMatches : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnOpenMatchesAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_OpenMatches* FindOpenMatches(const UObject* WorldContextObject, int64 MinStrength, int64 MaxStrength, int32 Limit, const FString& Cursor);
    virtual void Activate() override;
private:
    FString Operation;
    int64 MinStrength=0;
    int64 MaxStrength=0;
    int32 Limit=0;
    FString Cursor;
    void Complete(bool Ok,const FHorizonMatchPage& Value,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMatchReadAsyncResult,bool,bSuccess,const FHorizonMatchRead&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_MatchRead : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnMatchReadAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* GetMatch(const UObject* WorldContextObject, const FString& MatchId);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* JoinMatch(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* DeclineInvite(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* AdvanceTimeout(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* LeaveMatch(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* CancelMatch(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* StartMatch(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision, const TArray<FString>& TurnOrder);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRead* SubmitTurn(const UObject* WorldContextObject, const FString& MatchId, int64 ExpectedRevision, const TArray<uint8>& Data, bool bFinish);
    virtual void Activate() override;
private:
    FString Operation;
    FString MatchId;
    int64 ExpectedRevision=0;
    TArray<FString> TurnOrder;
    TArray<uint8> Data;
    bool bFinish=false;
    void Complete(bool Ok,const FHorizonMatchRead& Value,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMatchCreateAsyncResult,bool,bSuccess,const FHorizonMatchCreated&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_MatchCreate : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnMatchCreateAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchCreate* CreateMatch(const UObject* WorldContextObject, int64 ExpectedPlayerRevision, const FHorizonMatchCreateInput& Input);
    virtual void Activate() override;
private:
    FString Operation;
    int64 ExpectedPlayerRevision=0;
    FHorizonMatchCreateInput Input;
    void Complete(bool Ok,const FHorizonMatchCreated& Value,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMatchReportAsyncResult,bool,bSuccess,const FHorizonMatchReported&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_MatchReport : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnMatchReportAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchReport* ReportResult(const UObject* WorldContextObject, const FString& MatchId, int64 FinalStateRevision, const TArray<uint8>& Result, const TArray<FString>& WinnerIds);
    virtual void Activate() override;
private:
    FString Operation;
    FString MatchId;
    int64 FinalStateRevision=0;
    TArray<uint8> Result;
    TArray<FString> WinnerIds;
    void Complete(bool Ok,const FHorizonMatchReported& Value,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMatchRunAsyncResult,bool,bSuccess,const FHorizonValidatedRun&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_MatchRun : public UHorizonAsync_MatchesBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnMatchRunAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Matches") static UHorizonAsync_MatchRun* StartRankedRun(const UObject* WorldContextObject, const FString& MatchId, int64 FinalStateRevision);
    virtual void Activate() override;
private:
    FString Operation;
    FString MatchId;
    int64 FinalStateRevision=0;
    void Complete(bool Ok,const FHorizonValidatedRun& Value,const FString& Code,const FString& Message);
};
