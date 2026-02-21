#include "HorizonSessionSave.h"
#include "Kismet/GameplayStatics.h"

const FString UHorizonSessionSave::SlotName = TEXT("HorizonSession");
const int32 UHorizonSessionSave::UserIndex = 0;

bool UHorizonSessionSave::SaveToDisk() const
{
    return UGameplayStatics::SaveGameToSlot(const_cast<UHorizonSessionSave*>(this), SlotName, UserIndex);
}

UHorizonSessionSave* UHorizonSessionSave::LoadFromDisk()
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        return nullptr;
    }
    return Cast<UHorizonSessionSave>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
}

void UHorizonSessionSave::DeleteFromDisk()
{
    if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
    }
}

bool UHorizonSessionSave::HasSavedSession()
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}
