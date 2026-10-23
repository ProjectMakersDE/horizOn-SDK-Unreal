// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonSharedObjects.h"
#include "HorizonSharedObjectsExample.generated.h"
class UHorizonSubsystem;
/** The sample's original local intent is independent of regional hosts. */
UCLASS()
class HORIZONSDK_API UHorizonSharedOriginal : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString Project;
    UPROPERTY() FString UserId;
    UPROPERTY() FString Collection;
    UPROPERTY() FString IntentId;
    UPROPERTY() FString Action;
    UPROPERTY() FString ObjectId;
    UPROPERTY() FString GroupId;
    UPROPERTY() int64 ExpectedRevision=-1;
    UPROPERTY() FHorizonSharedObjectInput Input;
    /** Counted UTF-8 encoded as ASCII Base64; FString SaveGame may truncate Unicode after NUL. */
    UPROPERTY() TArray<FString> SearchStringBase64;
    UPROPERTY() FString OriginalHash;
    UPROPERTY() int64 SendSerial=0;
    UPROPERTY() bool Confirmed=false;
    UPROPERTY() FString ResolvedObjectId;
};
/** Existing session only. No BeginPlay auth, polling, traversal or writes. The marker nonce is a sample game format, not SDK payload interpretation. */
UCLASS()
class HORIZONSDK_API AHorizonSharedObjectsExample : public AActor
{
    GENERATED_BODY()
public:
    /** One owner usage + one owner page. Pass only an explicitly selected continuation cursor. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void RefreshOwned(const FString& Collection,const FString& GroupId=TEXT(""),const FString& Cursor=TEXT(""));
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void QueryRectangle(const FString& Collection,int64 MinX,int64 MaxX,int64 MinY,int64 MaxY,const FString& Cursor=TEXT(""));
    /** Varied bounded sampling only; the game owns combat, target binding and validated-run start/submit. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void QueryOpponents(const FString& Collection,const FString& PowerField,int64 MinPower,int64 MaxPower);
    /** Adds this sample's unique nonce prefix to the opaque bytes, then persists before its single creation attempt. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void CreateMarker(const FHorizonSharedObjectInput& Input);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void UpdateMarker(const FString& ObjectId,int64 ExpectedRevision,const FHorizonSharedObjectInput& Input);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void DeleteMarker(const FString& ObjectId,int64 ExpectedRevision);
    /** Create performs only owner reconciliation reads; update/delete retain their exact original tuple. */
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") void RetryOriginal();
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") bool ConfirmRecoveredCreate(const FString& ObjectId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects Example") bool AcknowledgeAfterRefresh();
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects Example") FHorizonSharedQueryResult GetObjects() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects Example") FHorizonSharedUsage GetUsage() const;
private:
    friend struct FHorizonSharedObjectsNative;
    UHorizonSubsystem* Session() const;
    bool Fresh() const;
    bool AllowRead(const FString& Key,bool Continuation,uint64 Context);
    void MarkRead(const FString& Key,int32 PollAfter);
    UHorizonSharedOriginal* Prepare(const FString& Action,const FString& Id,int64 Revision,const FHorizonSharedObjectInput& Input);
    void Start(const FString& Action,const FString& Id,int64 Revision,const FHorizonSharedObjectInput& Input);
    void Send(UHorizonSharedOriginal* Original,const FString& Slot);
    void ReadQuery(const FString& Collection,const FHorizonSharedQuery& Query,const FString& RefreshKey);
    bool Busy=false,FreshOwner=false;
    FString SnapshotCollection,SnapshotGroup,OriginalAtRefresh,RefreshKey;
    uint64 SnapshotContext=0,SnapshotGeneration=0;
    TMap<FString,double> RefreshAfter;
    FHorizonSharedUsage Usage;
    FHorizonSharedQueryResult Objects;
};
