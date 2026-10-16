#pragma once
#include "Managers/HorizonValidatedActionsManager.h"
/** Shared across generic/match starts; invalidation never releases another in-flight owner. */
struct FHorizonRunStartLease
{
    TWeakObjectPtr<UHorizonValidatedActionsManager> Owner;
    uint64 Id=0;
    FHorizonRunStartLease(UHorizonValidatedActionsManager* Manager,uint64 Token):Owner(Manager),Id(Token){}
    static TSharedPtr<FHorizonRunStartLease> Acquire(UHorizonValidatedActionsManager* Manager){const uint64 Token=Manager?Manager->AcquireRunStart():0;if(!Token)return {};return MakeShared<FHorizonRunStartLease>(Manager,Token);}
    void Release(){if(auto* Manager=Owner.Get())Manager->ReleaseRunStart(Id);Id=0;}
    ~FHorizonRunStartLease(){Release();}
};
