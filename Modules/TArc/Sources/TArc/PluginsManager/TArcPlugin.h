#pragma once

#include <PluginManager/Plugin.h>
#include <Reflection/ReflectedType.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Base/BaseTypes.h>

namespace DAVA
{
class EngineContext;
class TArcPlugin
{
public:
    struct PluginDescriptor
    {
        PluginDescriptor(const String& appName, const String& plugName, const String& shortDescr,
                         const String& fullDescr, int32 majorVer, int32 minorVer)
            : applicationName(appName)
            , pluginName(plugName)
            , shortDescription(shortDescr)
            , fullDescription(fullDescr)
            , majorVersion(majorVer)
            , minorVersion(minorVer)
        {
        }

        String applicationName;
        String pluginName;
        String shortDescription;
        String fullDescription;
        int32 majorVersion;
        int32 minorVersion;
    };

    TArcPlugin(const EngineContext* context);
    virtual ~TArcPlugin() = default;

    virtual const ReflectedType* GetModuleType() const = 0;
    virtual const PluginDescriptor& GetDescription() const = 0;
};

template <typename T>
class TypedTArcPlugin : public TArcPlugin
{
public:
    TypedTArcPlugin(const EngineContext* context, const TArcPlugin::PluginDescriptor& descriptor)
        : TArcPlugin(context)
        , descr(descriptor)
    {
    }

    const ReflectedType* GetModuleType() const override
    {
        return ReflectedTypeDB::Get<T>();
    }

    const PluginDescriptor& GetDescription() const override
    {
        return descr;
    }

private:
    PluginDescriptor descr;
};
} // namespace DAVA

#define CREATE_PLUGINS_ARRAY_FUNCTION_NAME CreatePluginsArray
#define DELETE_PLUGINS_ARRAY DeletePluginArray
#define DESTROY_PLUGIN_FUNCTION_NAME DestroyPlugin

using TCreatePluginFn = DAVA::TArcPlugin** (*)(const DAVA::EngineContext* context);
using TDestroyPluginsArray = void (*)(DAVA::TArcPlugin** pluginsArray);
using TDestroyPluginFn = void (*)(DAVA::TArcPlugin* plugin);

#define START_PLUGINS_DECLARATION()\
extern "C" { \
    PLUGIN_FUNCTION_EXPORT void DESTROY_PLUGIN_FUNCTION_NAME(DAVA::TArcPlugin* plugin) \
    { \
        delete plugin; \
    } \
    PLUGIN_FUNCTION_EXPORT void DELETE_PLUGINS_ARRAY(DAVA::TArcPlugin** pluginsArray) \
    { \
        delete[] pluginsArray; \
    } \
    PLUGIN_FUNCTION_EXPORT DAVA::TArcPlugin** CREATE_PLUGINS_ARRAY_FUNCTION_NAME(const DAVA::EngineContext* context) \
    { \
        DAVA::TArcPlugin** plugins = new DAVA::TArcPlugin*[32]; \
        memset(plugins, 0, 32 * sizeof(DAVA::TArcPlugin*)); \
        DAVA::int32 counter = 0

#define DECLARE_PLUGIN(moduleType, descr)\
    { \
        plugins[counter++] = new DAVA::TypedTArcPlugin<moduleType>(context, descr); \
    }

#define END_PLUGINS_DECLARATION()\
        return plugins; \
    } \
} \
\
int DAVAMain(DAVA::Vector<DAVA::String> cmdline)\
{\
    return 0;\
}
