// Copyright (c) Extra Life Studios, LLC. All rights reserved.

#include "IAbleCore.h"

#include "AbleCorePrivate.h"

class FAbleCore : public IAbleCore
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FAbleCore, AbleCore)
DEFINE_LOG_CATEGORY(LogAble);


void FAbleCore::StartupModule()
{

}


void FAbleCore::ShutdownModule()
{

}



