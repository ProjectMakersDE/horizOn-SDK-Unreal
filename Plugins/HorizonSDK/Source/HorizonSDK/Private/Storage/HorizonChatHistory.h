// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include "Models/HorizonChat.h"

/** Append-only device copies. Only the small per-channel ID index is cached in memory. */
class FHorizonChatHistory
{
public:
    FHorizonChatHistory(const FString& ApiKey,const FString& User,const FString& Base=FString());
    FString Capture(const FString& Channel) const;
    bool Append(const FString& Channel,const FHorizonChatMessage& Message,const FString& Captured,bool& Changed,FString& Error);
    FHorizonChatHistoryPage Read(const FString& Channel,const FString& Cursor,int32 Limit);
    bool Clear(const FString& Channel,FString& Error);
private:
    struct FIndex {TMap<FString,FString> Ids;int64 End=0,IndexEnd=0;FString Generation;};
    FString Directory;
    TMap<FString,FIndex> Indexes;
    FString Path(const FString& Channel,const TCHAR* Suffix) const;
    bool Open(const FString& Channel,FIndex*& Out,FString& Error);
};
