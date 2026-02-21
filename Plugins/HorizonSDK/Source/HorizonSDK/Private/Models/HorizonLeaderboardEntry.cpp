#include "Models/HorizonLeaderboardEntry.h"
#include "Dom/JsonObject.h"

FHorizonLeaderboardEntry FHorizonLeaderboardEntry::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
    FHorizonLeaderboardEntry Entry;
    if (JsonObject.IsValid())
    {
        Entry.Position = static_cast<int32>(JsonObject->GetNumberField(TEXT("position")));
        Entry.Username = JsonObject->GetStringField(TEXT("username"));
        Entry.Score = static_cast<int64>(JsonObject->GetNumberField(TEXT("score")));
    }
    return Entry;
}
