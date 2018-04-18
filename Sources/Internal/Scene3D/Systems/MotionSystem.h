#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Entity/SceneSystem.h"
#include "Scene3D/Components/SkeletonComponent.h"
#include "Scene3D/Systems/BaseSimulationSystem.h"

namespace DAVA
{
class Component;
class Entity;
class MotionComponent;
class MotionSingleComponent;
class SimpleMotion;

class MotionSystem : public BaseSimulationSystem
{
public:
    DAVA_VIRTUAL_REFLECTION(MotionSystem, SceneSystem);

    MotionSystem(Scene* scene);
    ~MotionSystem();

    void AddEntity(Entity* entity) override;
    void RemoveEntity(Entity* entity) override;
    void PrepareForRemove() override;
    void UnregisterEntity(Entity* entity) override;

    void ImmediateEvent(Component* component, uint32 event) override;
    void ProcessFixed(float32 timeElapsed) override;

protected:
    void SetScene(Scene* scene) override;

private:
    void UpdateMotionLayers(MotionComponent* motionComponent, float32 dTime);

    Vector<MotionComponent*> activeComponents;
    MotionSingleComponent* motionSingleComponent = nullptr;
};

} //ns
