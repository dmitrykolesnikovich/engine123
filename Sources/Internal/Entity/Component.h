#pragma once

#include "Base/BaseTypes.h"
#include "Base/Serializable.h"
#include "Base/Introspection.h"
#include "Scene3D/SceneFile/SerializationContext.h"

#include "MemoryManager/MemoryProfiler.h"
#include "Reflection/Reflection.h"

/**
    \defgroup components Component
*/

namespace DAVA
{
class Entity;

class Component : public Serializable, public InspBase
{
    DAVA_ENABLE_CLASS_ALLOCATION_TRACKING(ALLOC_POOL_COMPONENT)

public:
    enum eType
    {
        TRANSFORM_COMPONENT = 0,
        RENDER_COMPONENT,
        LOD_COMPONENT,
        DEBUG_RENDER_COMPONENT,
        SWITCH_COMPONENT,
        CAMERA_COMPONENT,
        LIGHT_COMPONENT,
        PARTICLE_EFFECT_COMPONENT,
        BULLET_COMPONENT,
        UPDATABLE_COMPONENT,
        ANIMATION_COMPONENT,
        COLLISION_COMPONENT, // multiple instances
        PHYSICS_COMPONENT,
        ACTION_COMPONENT, // actions, something simplier than scripts that can influence logic, can be multiple
        SCRIPT_COMPONENT, // multiple instances, not now, it will happen much later.
        USER_COMPONENT,
        SOUND_COMPONENT,
        CUSTOM_PROPERTIES_COMPONENT,
        STATIC_OCCLUSION_COMPONENT,
        STATIC_OCCLUSION_DATA_COMPONENT,
        QUALITY_SETTINGS_COMPONENT, // type as fastname for detecting type of model
        SPEEDTREE_COMPONENT,
        WIND_COMPONENT,
        WAVE_COMPONENT,
        SKELETON_COMPONENT,
        PATH_COMPONENT,
        ROTATION_CONTROLLER_COMPONENT,
        SNAP_TO_LANDSCAPE_CONTROLLER_COMPONENT,
        WASD_CONTROLLER_COMPONENT,
        VISIBILITY_CHECK_COMPONENT,
        SLOT_COMPONENT,
        MOTION_COMPONENT,
        GEO_DECAL_COMPONENT,

#if defined(__DAVAENGINE_PHYSICS_ENABLED__)
        STATIC_BODY_COMPONENT,
        DYNAMIC_BODY_COMPONENT,
        BOX_SHAPE_COMPONENT,
        CAPSULE_SHAPE_COMPONENT,
        SPHERE_SHAPE_COMPONENT,
        PLANE_SHAPE_COMPONENT,
        MESH_SHAPE_COMPONENT,
        CONVEX_HULL_SHAPE_COMPONENT,
        HEIGHT_FIELD_SHAPE_COMPONENT,
        BOX_CHARACTER_CONTROLLER_COMPONENT,
        CAPSULE_CHARACTER_CONTROLLER_COMPONENT,
        WASD_PHYSICS_CONTROLLER_COMPONENT,
#endif

        NON_EXPORTABLE_COMPONENTS, // everything below NON_EXPORTABLE_COMPONENTS will be serialized but won't be exported
        TEXT_COMPONENT = NON_EXPORTABLE_COMPONENTS,

        NON_SERIALIZABLE_COMPONENTS, // everything below NON_SERIALIZABLE_COMPONENTS won't be serialized
        STATIC_OCCLUSION_DEBUG_DRAW_COMPONENT = NON_SERIALIZABLE_COMPONENTS,
        WAYPOINT_COMPONENT,
        EDGE_COMPONENT,

        FIRST_USER_DEFINED_COMPONENT = 52,
        COMPONENT_COUNT = 64
    };

public:
    static Component* CreateByType(const Type* componentType);
    template <typename T>
    static T* CreateByType();

    ~Component() override;

    const Type* GetType() const;
    int32 GetRuntimeType() const;

    /**
        Clone component. Then add cloned component to specified `toEntity` if `toEntity` is not nullptr. Return cloned component.
    */
    virtual Component* Clone(Entity* toEntity) = 0;

    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    inline Entity* GetEntity() const;
    virtual void SetEntity(Entity* entity);

    /**
        \brief This function should be implemented in each node that have data nodes inside it.
     */
    virtual void GetDataNodes(Set<DataNode*>& dataNodes);

    /**
        \brief This function optimize component before export.
    */
    virtual void OptimizeBeforeExport()
    {
    }

    /**
        \brief Function to get data nodes of requested type to specific container you provide.
    */
    template <template <typename> class Container, class T>
    void GetDataNodes(Container<T>& container);

protected:
    Entity* entity = 0;

    DAVA_VIRTUAL_REFLECTION(Component, InspBase);
};

template <typename T>
inline T* Component::CreateByType()
{
    return DynamicTypeCheck<T*>(CreateByType(Type::Instance<T>()));
}

inline Entity* Component::GetEntity() const
{
    return entity;
};

inline ComponentFlags MakeComponentMask(int32 runtimeType)
{
    DVASSERT(runtimeType > 0);

    ComponentFlags flags;
    flags.set(static_cast<size_t>(runtimeType));
    return flags;
}

ComponentFlags MakeComponentMask(const Type* type);

template <typename T>
ComponentFlags MakeComponentMask()
{
    static_assert(std::is_base_of<Component, T>::value, "");
    return MakeComponentMask(Type::Instance<T>());
}

template <template <typename> class Container, class T>
void Component::GetDataNodes(Container<T>& container)
{
    Set<DataNode*> objects;
    GetDataNodes(objects);

    Set<DataNode*>::const_iterator end = objects.end();
    for (Set<DataNode*>::iterator t = objects.begin(); t != end; ++t)
    {
        DataNode* obj = *t;

        T res = dynamic_cast<T>(obj);
        if (res != nullptr)
        {
            container.push_back(res);
        }
    }
}
};
