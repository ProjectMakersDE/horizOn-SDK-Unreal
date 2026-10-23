// Copyright (c) 2026 ProjectMakers. Licensed under the MIT License.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHorizonSDK, Log, All);

class FHorizonSDKModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
