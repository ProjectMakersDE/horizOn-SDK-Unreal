#include "HorizonConfig.h"

UHorizonConfig::UHorizonConfig()
{
}

const UHorizonConfig* UHorizonConfig::Get()
{
    return GetDefault<UHorizonConfig>();
}

FName UHorizonConfig::GetCategoryName() const
{
    return FName(TEXT("Plugins"));
}
