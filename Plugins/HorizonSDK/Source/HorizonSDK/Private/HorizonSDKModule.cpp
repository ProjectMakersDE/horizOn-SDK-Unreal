#include "HorizonSDKModule.h"

DEFINE_LOG_CATEGORY(LogHorizonSDK);

#define LOCTEXT_NAMESPACE "FHorizonSDKModule"

void FHorizonSDKModule::StartupModule()
{
}

void FHorizonSDKModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHorizonSDKModule, HorizonSDK)
