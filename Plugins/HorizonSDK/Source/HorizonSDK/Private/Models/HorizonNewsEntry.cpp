#include "Models/HorizonNewsEntry.h"
#include "Dom/JsonObject.h"

FHorizonNewsEntry FHorizonNewsEntry::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
    FHorizonNewsEntry Entry;
    if (JsonObject.IsValid())
    {
        Entry.Id = JsonObject->GetStringField(TEXT("id"));
        Entry.Title = JsonObject->GetStringField(TEXT("title"));
        Entry.Message = JsonObject->GetStringField(TEXT("message"));
        Entry.ReleaseDate = JsonObject->GetStringField(TEXT("releaseDate"));
        Entry.LanguageCode = JsonObject->GetStringField(TEXT("languageCode"));
    }
    return Entry;
}
