#pragma once

#include <TArc/Core/ClientModule.h>

class StyleSheetInspectorModule : public DAVA::TArc::ClientModule
{
    void PostInit() override;

    DAVA_VIRTUAL_REFLECTION(StyleSheetInspectorModule, DAVA::TArc::ClientModule);
};
