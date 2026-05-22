#include "Models/HorizonLeaderboardBoard.h"
#include "Dom/JsonObject.h"

FHorizonLeaderboardBoard FHorizonLeaderboardBoard::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	FHorizonLeaderboardBoard Board;
	if (!JsonObject.IsValid())
	{
		return Board;
	}

	JsonObject->TryGetStringField(TEXT("id"), Board.Id);
	JsonObject->TryGetStringField(TEXT("apiKeyId"), Board.ApiKeyId);
	JsonObject->TryGetStringField(TEXT("key"), Board.Key);
	JsonObject->TryGetStringField(TEXT("name"), Board.Name);
	JsonObject->TryGetStringField(TEXT("sortOrder"), Board.SortOrder);
	JsonObject->TryGetBoolField(TEXT("isActive"), Board.bIsActive);
	JsonObject->TryGetStringField(TEXT("createdAt"), Board.CreatedAt);
	JsonObject->TryGetStringField(TEXT("updatedAt"), Board.UpdatedAt);

	double ScoreCount = 0;
	if (JsonObject->TryGetNumberField(TEXT("scoreCount"), ScoreCount))
	{
		Board.ScoreCount = static_cast<int64>(ScoreCount);
	}

	return Board;
}
