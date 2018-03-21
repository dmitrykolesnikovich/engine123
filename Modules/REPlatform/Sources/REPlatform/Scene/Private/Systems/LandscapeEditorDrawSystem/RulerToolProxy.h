#pragma once

#include <Base/BaseTypes.h>
#include <Base/BaseObject.h>

namespace DAVA
{
class Texture;
class RulerToolProxy : public BaseObject
{
protected:
    ~RulerToolProxy();

public:
    RulerToolProxy(int32 size);

    int32 GetSize();
    const Asset<Texture>& GetTexture() const;

protected:
    Asset<Texture> rulerToolTexture;
    int32 size;
    bool spriteChanged;
};
} // namespace DAVA
