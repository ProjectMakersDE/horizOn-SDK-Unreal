#pragma once

#include "Modules/ModuleManager.h"

class FHorizonSDKEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
