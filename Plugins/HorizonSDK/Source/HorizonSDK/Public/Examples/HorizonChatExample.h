// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/SaveGame.h"
#include "Models/HorizonChat.h"
#include "HorizonChatExample.generated.h"
class UHorizonSubsystem;
class UHorizonChatExampleWidget;
/** Two-generation journal: exact UTF-8 is base64 before SaveGame serialization. Never stores a session token. */
UCLASS()
class HORIZONSDK_API UHorizonChatOriginal : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString Project;
    UPROPERTY() FString UserId;
    UPROPERTY() FString Action;
    UPROPERTY() FString OperationId;
    UPROPERTY() FString IssuedAt;
    UPROPERTY() FString ChannelId;
    UPROPERTY() FString MessageId;
    UPROPERTY() FString TextBase64;
    UPROPERTY() FString State=TEXT("pending");
    UPROPERTY() FHorizonChatReceipt Receipt;
    UPROPERTY() FString ReportId;
    UPROPERTY() FString ReportStatus;
    UPROPERTY() int64 SendSerial=0;
    UPROPERTY() int64 StorageGeneration=0;
    UPROPERTY() FString StorageHash;
};
/** Explicit player actions, existing session only. An unresolved journal blocks every fresh intent. */
UCLASS()
class HORIZONSDK_API AHorizonChatExample : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void Open(const FHorizonChatOpenInput& Input);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void SendText(const FString& Text);
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void ReconcileOriginal();
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void RetryOriginal();
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void Report(const FString& MessageId,const FString& Reason);
    UFUNCTION(BlueprintPure,Category="horizOn|Chat Example") FHorizonChatFeed GetFeed() const;
    UFUNCTION(BlueprintPure,Category="horizOn|Chat Example") bool IsSubscribed() const;
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void ShowUI();
    UFUNCTION(BlueprintCallable,Category="horizOn|Chat Example") void CloseChannel();
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat Example") FString LastErrorCode;
    UPROPERTY(BlueprintReadOnly,Category="horizOn|Chat Example") FString ChannelId;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    friend struct FHorizonChatNative;
    UPROPERTY() UHorizonChatExampleWidget* Widget=nullptr;
    bool Busy=false;
    uint64 Selection=0,MissingContext=0;
    FString MissingOriginal;
    UHorizonSubsystem* Session() const;
    void Prepare(const FString& Action,const FString& Text,const FString& Message);
    void Transmit(UHorizonChatOriginal* Original);
};
