// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Tickable.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonChat.h"
#include "HorizonChatManager.generated.h"
class UHorizonRealtimeManager;
class FHorizonChatHistory;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatFeedChanged,const FString&,ChannelId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatHistoryFailed,const FString&,ChannelId,const FString&,ErrorCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonChatLimitsViewResult,bool,bSuccess,const FHorizonChatLimitsView&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonChatOpenedResult,bool,bSuccess,const FHorizonChatOpened&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonChatSyncedResult,bool,bSuccess,const FHorizonChatSynced&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonChatSentResult,bool,bSuccess,const FHorizonChatSent&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonChatOperationResult,bool,bSuccess,const FHorizonChatOperation&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonChatReportedResult,bool,bSuccess,const FHorizonChatReported&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
/** Authorized, immutable incremental chat feeds. No disconnected HTTP fallback or periodic polling. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonChatManager : public UObject,public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth,UHorizonRealtimeManager* Realtime);
    void GetLimits(const FString& UserId,FOnChatLimitsViewComplete Complete);
    void OpenChannel(const FString& UserId,const FHorizonChatOpenInput& Input,FOnChatOpenedComplete Complete);
    /** Explicit REST read, also while disconnected from realtime. Only an authorized watched topic may adopt a feed. */
    void SyncChannel(const FString& UserId,const FString& ChannelId,FOnChatSyncedComplete Complete);
    void SendMessage(const FString& UserId,const FString& ChannelId,const FString& OperationId,const FString& IssuedAt,const FString& Text,FOnChatSentComplete Complete);
    void GetOperation(const FString& UserId,const FString& OperationId,FOnChatOperationComplete Complete);
    void ReportMessage(const FString& UserId,const FString& ChannelId,const FString& MessageId,const FString& Reason,FOnChatReportedComplete Complete);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") bool WatchChannel(const FString& UserId,const FString& ChannelId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void UnwatchChannel(const FString& ChannelId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void ClearCache();
    UFUNCTION(BlueprintPure,Category="horizOn|Chat") FHorizonChatFeed GetFeed(const FString& ChannelId);
    /** Device copies in local delivery order. No HTTP and no server-expiry deletion. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") FHorizonChatHistoryPage ReadLocalHistory(const FString& ChannelId,const FString& Cursor=TEXT(""),int32 Limit=50);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") bool ClearLocalHistory(const FString& ChannelId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") bool ClearAllLocalHistory();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnChatFeedChanged OnLocalHistoryChanged;
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnChatHistoryFailed OnLocalHistoryFailed;
    /** Local-only recovery using the saved exact input and a verified GetOperation response. */
    FString CaptureLocalHistory(const FString& ChannelId);
    void ArchiveConfirmedSend(const FString& UserId,const FString& Text,const FHorizonChatReceipt& Receipt,const FString& Captured);
    UFUNCTION(BlueprintPure,Category="horizOn|Chat") bool IsChannelSubscribed(const FString& ChannelId);
    UFUNCTION(BlueprintPure,Category="horizOn|Chat") FHorizonChatLimitsView GetCurrentLimits();
    UFUNCTION(BlueprintPure,Category="horizOn|Chat") FHorizonChatResponseMetadata GetLastResponseMetadata();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnChatFeedChanged OnFeedChanged;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void RequestChatLimits(const FString& UserId);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnHorizonChatLimitsViewResult OnLimitsViewResult;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void OpenChatChannel(const FString& UserId,const FHorizonChatOpenInput& Input);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnHorizonChatOpenedResult OnOpenedResult;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void RequestChatSync(const FString& UserId,const FString& ChannelId);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnHorizonChatSyncedResult OnSyncedResult;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void SendChatMessage(const FString& UserId,const FString& ChannelId,const FString& OperationId,const FString& IssuedAt,const FString& Text);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnHorizonChatSentResult OnSentResult;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void RequestChatOperation(const FString& UserId,const FString& OperationId);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnHorizonChatOperationResult OnOperationResult;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat") void ReportChatMessage(const FString& UserId,const FString& ChannelId,const FString& MessageId,const FString& Reason);
    UPROPERTY(BlueprintAssignable,Category="horizOn|Chat") FOnHorizonChatReportedResult OnReportedResult;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override {return !IsTemplate()&&HttpClient!=nullptr;}
    virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(UHorizonChatManager,STATGROUP_Tickables);}
    virtual void BeginDestroy() override;
private:
    friend struct FHorizonChatNative;
    struct FChannelState
    {
        bool Busy=false,Dirty=false,Visible=false,Complete=false;
        uint64 Serial=0,RealtimeGeneration=0,Authority=0;
        FHorizonChatFeed Feed;
        TArray<FOnChatSyncedComplete> Waiters;
        double SynchronizedAt=0;
        FString ServerFloor;
    };
    TMap<FString,FChannelState> Channels;
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    UPROPERTY() UHorizonRealtimeManager* Realtime=nullptr;
    FHorizonChatLimitsView Limits;
    FHorizonChatResponseMetadata ResponseMetadata;
    FString Identity;
    FString HistoryIdentity;
    TSharedPtr<FHorizonChatHistory> History;
    uint64 Context=0,Generation=0,NextSerial=0;
    void SyncIdentity();
    FString Validate(const FString& UserId);
    void Call(const FString& UserId,const FString& Route,const TSharedPtr<FJsonObject>& Body,bool ReadOnly,FOnHttpResponse Complete);
    void StartSync(const FString& ChannelId);
    void Hide(const FString& ChannelId,bool Forget);
    void FinishSync(const FString& ChannelId,bool Success,const FHorizonChatSynced& Result,const FString& Code,const FString& Message);
    UFUNCTION() void OnTopic(const FString& Topic);
    UFUNCTION() void OnSubscribed(const FString& Topic);
    UFUNCTION() void OnRealtimeEvent(const FString& EventJson);
    UFUNCTION() void OnRealtimeError(const FString& Code);
    UFUNCTION() void OnSubscriptionError(const FString& Topic,const FString& Code);
    static FHorizonNetworkResponse Error(const FString& Code);
    void Archive(const FString& Channel,TArray<FHorizonChatMessage> Messages,const FString& Captured);
};
