// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Models/HorizonSharedObjects.h"
#include "HorizonAsync_SharedObjects.generated.h"
class UHorizonSubsystem;
UCLASS(Abstract)
class HORIZONSDK_API UHorizonAsync_SharedBase : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()
protected:
    void Freeze(const UObject* World);
    UHorizonSubsystem* Resolve(FString& Code);
    TWeakObjectPtr<const UObject> World;
    TWeakObjectPtr<UHorizonSubsystem> OriginalSubsystem;
    FString User;
    uint64 Context=0;
    bool Activated=false;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSharedCollectionsAsyncResult,bool,bSuccess,const FHorizonSharedCollectionPage&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_SharedCollections : public UHorizonAsync_SharedBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnSharedCollectionsAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedCollections* GetCollections(const UObject* WorldContextObject,const FString& AfterKey,int32 Limit,int64 CatalogRevision);
    virtual void Activate() override;
private:
    FString After;int32 Limit=50;int64 Revision=-1;
    void Complete(bool Ok,const FHorizonSharedCollectionPage& Result,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSharedUsageAsyncResult,bool,bSuccess,const FHorizonSharedUsage&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_SharedUsage : public UHorizonAsync_SharedBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnSharedUsageAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedUsage* GetUsage(const UObject* WorldContextObject,const FString& Collection);
    virtual void Activate() override;
private:
    FString Collection;
    void Complete(bool Ok,const FHorizonSharedUsage& Result,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSharedObjectAsyncResult,bool,bSuccess,const FHorizonSharedObjectRead&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_SharedObject : public UHorizonAsync_SharedBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnSharedObjectAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedObject* GetObject(const UObject* WorldContextObject,const FString& Collection,const FString& ObjectId);
    virtual void Activate() override;
private:
    FString Collection,Id;
    void Complete(bool Ok,const FHorizonSharedObjectRead& Result,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSharedQueryAsyncResult,bool,bSuccess,const FHorizonSharedQueryResult&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_SharedQuery : public UHorizonAsync_SharedBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnSharedQueryAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedQuery* QueryObjects(const UObject* WorldContextObject,const FString& Collection,const FHorizonSharedQuery& Query);
    virtual void Activate() override;
private:
    FString Collection;FHorizonSharedQuery Query;
    void Complete(bool Ok,const FHorizonSharedQueryResult& Result,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSharedWriteAsyncResult,bool,bSuccess,const FHorizonSharedWriteResult&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_SharedWrite : public UHorizonAsync_SharedBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnSharedWriteAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedWrite* CreateObject(const UObject* WorldContextObject,const FString& Collection,int64 ExpectedUsageRevision,const FHorizonSharedObjectInput& Input,const FString& GroupId);
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedWrite* UpdateObject(const UObject* WorldContextObject,const FString& Collection,const FString& ObjectId,int64 ExpectedObjectRevision,const FHorizonSharedObjectInput& Input);
    virtual void Activate() override;
private:
    FString Collection,Id,Group;int64 Revision=0;FHorizonSharedObjectInput Input;bool Create=false;
    void Complete(bool Ok,const FHorizonSharedWriteResult& Result,const FString& Code,const FString& Message);
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnSharedDeleteAsyncResult,bool,bSuccess,const FHorizonSharedDeleteResult&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
UCLASS()
class HORIZONSDK_API UHorizonAsync_SharedDelete : public UHorizonAsync_SharedBase
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FOnSharedDeleteAsyncResult OnComplete;
    UFUNCTION(BlueprintCallable,meta=(BlueprintInternalUseOnly="true",WorldContext="WorldContextObject"),Category="horizOn|Shared Objects") static UHorizonAsync_SharedDelete* DeleteObject(const UObject* WorldContextObject,const FString& Collection,const FString& ObjectId,int64 ExpectedObjectRevision);
    virtual void Activate() override;
private:
    FString Collection,Id;int64 Revision=0;
    void Complete(bool Ok,const FHorizonSharedDeleteResult& Result,const FString& Code,const FString& Message);
};
