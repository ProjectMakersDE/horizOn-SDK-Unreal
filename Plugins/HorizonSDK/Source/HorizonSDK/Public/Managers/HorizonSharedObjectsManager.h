// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Http/HorizonHttpClient.h"
#include "Managers/HorizonAuthManager.h"
#include "Models/HorizonSharedObjects.h"
#include "HorizonSharedObjectsManager.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonSharedCollectionsResult,bool,bSuccess,const FHorizonSharedCollectionPage&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonSharedUsageResult,bool,bSuccess,const FHorizonSharedUsage&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonSharedObjectResult,bool,bSuccess,const FHorizonSharedObjectRead&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonSharedQueryResult,bool,bSuccess,const FHorizonSharedQueryResult&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonSharedWriteResult,bool,bSuccess,const FHorizonSharedWriteResult&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHorizonSharedDeleteResult,bool,bSuccess,const FHorizonSharedDeleteResult&,Result,const FString&,ErrorCode,const FString&,ErrorMessage);
/** Seven explicit bounded App operations. A cursor grants no authorization. All CAS writes send once. */
UCLASS(BlueprintType)
class HORIZONSDK_API UHorizonSharedObjectsManager : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UHorizonHttpClient* Http,UHorizonAuthManager* Auth);
    void GetCollections(const FString& UserId,FOnSharedCollectionsComplete Complete,const FString& AfterKey=TEXT(""),int32 Limit=50,int64 CatalogRevision=-1);
    void GetUsage(const FString& UserId,const FString& Collection,FOnSharedUsageComplete Complete);
    void GetObject(const FString& UserId,const FString& Collection,const FString& ObjectId,FOnSharedObjectComplete Complete);
    void QueryObjects(const FString& UserId,const FString& Collection,const FHorizonSharedQuery& Query,FOnSharedQueryComplete Complete);
    void CreateObject(const FString& UserId,const FString& Collection,int64 ExpectedUsageRevision,const FHorizonSharedObjectInput& Input,FOnSharedWriteComplete Complete,const FString& GroupId=TEXT(""));
    void UpdateObject(const FString& UserId,const FString& Collection,const FString& ObjectId,int64 ExpectedObjectRevision,const FHorizonSharedObjectInput& Input,FOnSharedWriteComplete Complete);
    void DeleteObject(const FString& UserId,const FString& Collection,const FString& ObjectId,int64 ExpectedObjectRevision,FOnSharedDeleteComplete Complete);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void ClearCache();
    uint64 GetCacheGeneration();
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects") FHorizonSharedCollectionPage GetCurrentCollections();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Shared Objects") FOnHorizonSharedCollectionsResult OnCollectionsResult;
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects") FHorizonSharedUsage GetCurrentUsage();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Shared Objects") FOnHorizonSharedUsageResult OnUsageResult;
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects") FHorizonSharedObjectRead GetCurrentObject();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Shared Objects") FOnHorizonSharedObjectResult OnObjectResult;
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects") FHorizonSharedQueryResult GetCurrentQuery();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Shared Objects") FOnHorizonSharedQueryResult OnQueryResult;
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects") FHorizonSharedWriteResult GetCurrentWrite();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Shared Objects") FOnHorizonSharedWriteResult OnWriteResult;
    UFUNCTION(BlueprintPure,Category="horizOn|Shared Objects") FHorizonSharedDeleteResult GetCurrentDelete();
    UPROPERTY(BlueprintAssignable,Category="horizOn|Shared Objects") FOnHorizonSharedDeleteResult OnDeleteResult;
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void RequestCollections(const FString& UserId,const FString& AfterKey,int32 Limit,int64 CatalogRevision);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void RequestUsage(const FString& UserId,const FString& Collection);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void RequestObject(const FString& UserId,const FString& Collection,const FString& ObjectId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void RequestQuery(const FString& UserId,const FString& Collection,const FHorizonSharedQuery& Query);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void CreateSharedObject(const FString& UserId,const FString& Collection,int64 ExpectedUsageRevision,const FHorizonSharedObjectInput& Input,const FString& GroupId);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void UpdateSharedObject(const FString& UserId,const FString& Collection,const FString& ObjectId,int64 ExpectedObjectRevision,const FHorizonSharedObjectInput& Input);
    UFUNCTION(BlueprintCallable,Category="horizOn|Shared Objects") void DeleteSharedObject(const FString& UserId,const FString& Collection,const FString& ObjectId,int64 ExpectedObjectRevision);
private:
    UPROPERTY() UHorizonHttpClient* HttpClient=nullptr;
    UPROPERTY() UHorizonAuthManager* AuthManager=nullptr;
    FString Identity,Scope;
    uint64 Context=0,Epoch=0,Ticket=0;
    bool Busy=false;
    int64 RevisionFloor=-1,ServerMillis=-1;
    FHorizonSharedCollectionPage CurrentCollections;
    FHorizonSharedUsage CurrentUsage;
    FHorizonSharedObjectRead CurrentObject;
    FHorizonSharedQueryResult CurrentQuery;
    FHorizonSharedWriteResult CurrentWrite;
    FHorizonSharedDeleteResult CurrentDelete;
    void Sync();
    void ClearSnapshots();
    bool Accept(const FString& ServerTime,int64 Revision=-1);
    void Call(const FString& User,const FString& Collection,const FString& Route,const FString& Verb,const TSharedPtr<FJsonObject>& Body,int64 OriginalRevision,FOnHttpResponse Complete);
    static FHorizonNetworkResponse Error(const FString& Code);
};
