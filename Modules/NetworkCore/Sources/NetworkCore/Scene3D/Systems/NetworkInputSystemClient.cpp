#ifndef SERVER

#include "NetworkCore/NetworkCoreUtils.h"
#include "NetworkCore/Scene3D/Components/NetworkTransformComponent.h"
#include "NetworkCore/Scene3D/Components/NetworkReplicationComponent.h"
#include "NetworkCore/Scene3D/Components/SingleComponents/NetworkTimeSingleComponent.h"
#include "NetworkCore/Scene3D/Components/SingleComponents/NetworkClientSingleComponent.h"
#include "NetworkCore/Scene3D/Components/SingleComponents/NetworkStatisticsSingleComponent.h"
#include "NetworkCore/Scene3D/Components/SingleComponents/NetworkClientConnectionSingleComponent.h"
#include "NetworkInputSystem.h"
#include "NetworkTimeSystem.h"

#include "Debug/DVAssert.h"
#include "DeviceManager/DeviceManager.h"
#include "Engine/Engine.h"
#include "Engine/EngineContext.h"
#include "Input/Keyboard.h"
#include "Logger/Logger.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Components/SingleComponents/ActionsSingleComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Scene.h"
#include "Time/SystemTimer.h"
#include <Reflection/ReflectionRegistrator.h>

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(NetworkInputSystem)
{
    ReflectionRegistrator<NetworkInputSystem>::Begin()[M::Tags("network", "input")]
    .ConstructorByPointer<Scene*>()
    .Method("ProcessFixed", &NetworkInputSystem::ProcessFixed)[M::SystemProcess(SP::Group::ENGINE_END, SP::Type::FIXED, 4.0f)]
    .Method("ProcessFixedClientBegin", &NetworkInputSystem::ProcessFixedClientBegin)[M::SystemProcess(SP::Group::ENGINE_BEGIN, SP::Type::FIXED, 18.0f)]
    .End();
}

NetworkPackedQuaternion GetPackedCameraDelta(NetworkInputComponent* netInputComp,
                                             Camera* camera)
{
    Quaternion result;
    uint64 pack = result.Pack();
    Quaternion currOrient = camera->GetOrientation();
    if (!netInputComp->HasLastCameraOrientation())
    {
        netInputComp->SetLastCameraOrientation(currOrient);
#ifdef DISABLE_LOSSY_PACK
        return NetworkPackedQuaternion{ result };
#else
        return pack;
#endif
    }
    const Quaternion& lastOrient = netInputComp->GetLastCameraOrientation();
    if (currOrient == lastOrient)
    {
#ifdef DISABLE_LOSSY_PACK
        return NetworkPackedQuaternion{ result };
#else
        return pack;
#endif
    }
    Quaternion currOrientInv(currOrient);
    currOrientInv.Inverse();
    result = currOrientInv * lastOrient;
    result.Normalize();
#ifndef DISABLE_LOSSY_PACK
    pack = result.Pack();
    result.Unpack(pack);
#endif
    camera->RestoreCameraTransform();
    camera->Rotate(result);
    netInputComp->SetLastCameraOrientation(camera->GetOrientation());
#ifdef DISABLE_LOSSY_PACK
    return NetworkPackedQuaternion{ result };
#else
    return pack;
#endif
}

NetworkInputSystem::NetworkInputSystem(Scene* scene)
    : SceneSystem(scene, ComponentUtils::MakeMask<NetworkInputComponent>())
{
    client = scene->GetSingleComponentForRead<NetworkClientSingleComponent>(this)->GetClient();
    DVASSERT(client != nullptr);
    netConnectionsComp = scene->GetSingleComponentForRead<NetworkClientConnectionSingleComponent>(this);
    DVASSERT(netConnectionsComp);
}

void NetworkInputSystem::AddEntity(Entity* entity)
{
    entities.insert(entity);
}

void NetworkInputSystem::RemoveEntity(Entity* entity)
{
    entities.erase(entity);
}

void NetworkInputSystem::ProcessFixedClientBegin(float32 timeElapsed)
{
    const NetworkTimeSingleComponent* netTimeComp = GetScene()->GetSingleComponentForRead<NetworkTimeSingleComponent>(this);
    if (!netTimeComp->IsInitialized())
    {
        return;
    }

    /*
    On client here we assign client frames to actions.
    */

    for (Entity* entity : entities)
    {
        for (auto& actions : GetCollectedActionsForClient(GetScene(), entity))
        {
            actions.clientFrameId = netTimeComp->GetFrameId();
        }
    }
}

void NetworkInputSystem::ProcessFixed(float32 timeElapsed)
{
    if (netConnectionsComp->IsJustConnected())
    {
        for (Entity* entity : entities)
        {
            NetworkInputComponent* netInputComp = entity->GetComponent<NetworkInputComponent>();
            netInputComp->ModifyHistory().Clear();
        }
    }

    const NetworkTimeSingleComponent* netTimeComp = GetScene()->GetSingleComponentForRead<NetworkTimeSingleComponent>(this);
    if (!netTimeComp->IsInitialized())
    {
        return;
    }

    /*
        On client here we assign client frames to actions.
     */

    for (Entity* entity : entities)
    {
        for (auto& actions : GetCollectedActionsForClient(GetScene(), entity))
        {
            actions.clientFrameId = netTimeComp->GetFrameId();
        }

        NetworkInputComponent::Data inputData;
        uint64 packedActions = 0;
        PackDigitalActions(GetScene(), packedActions, entity);
        inputData.analogStates = PackAnalogActions(GetScene(), packedActions, entity);
        inputData.actions = packedActions;

        NetworkInputComponent* netInputComp = entity->GetComponent<NetworkInputComponent>();
        CameraComponent* cameraComp = entity->GetComponent<CameraComponent>();
        if (nullptr != cameraComp)
        {
            inputData.cameraDelta = GetPackedCameraDelta(netInputComp, cameraComp->GetCamera());
        }
        netInputComp->SaveHistory(netTimeComp->GetFrameId(), inputData);
    }

    if (netTimeComp->GetFrameId() % NetworkStatisticsSingleComponentDetail::TIMESTAMPS_DELAY == 0)
    {
        std::unique_ptr<NetStatTimestamps> timestamps(new NetStatTimestamps());
        timestamps->client.input = static_cast<uint32>(SystemTimer::GetMs());
        NetworkStatisticsSingleComponent* statsComp = GetScene()->GetSingleComponentForWrite<NetworkStatisticsSingleComponent>(this);
        if (statsComp)
        {
            statsComp->AddTimestamps(NetStatTimestamps::GetKey(netTimeComp->GetFrameId()), std::move(timestamps));
        }
    }

    SendLastBuckets();
}

void NetworkInputSystem::SendLastBuckets()
{
    PacketParams packetParams = PacketParams::Unreliable(PacketParams::INPUT_CHANNEL_ID);
    NetworkStatisticsSingleComponent* statsComp = GetScene()->GetSingleComponentForWrite<NetworkStatisticsSingleComponent>(this);
    InputPacketHeader header;
    header.frameId = 0;
    for (Entity* entity : entities)
    {
        NetworkReplicationComponent* networkReplicationComponent = entity->GetComponent<NetworkReplicationComponent>();
        header.entityId = networkReplicationComponent->GetNetworkID();

        NetworkInputComponent* netInputComp = entity->GetComponent<NetworkInputComponent>();
        const NetworkInputComponent::History& inputHistory = netInputComp->GetHistory();
        auto beginIt = inputHistory.Begin();
        auto endIt = inputHistory.End();
        if (beginIt != endIt)
        {
            header.frameId = inputHistory.GetMaxFrameId();
            header.framesCount = inputHistory.GetSize();
            const uint32 dataSize = sizeof(NetworkInputComponent::Data);
            const uint32 maxDataSize = header.framesCount * dataSize;
            uint32 maxPacketSize = INPUT_PACKET_HEADER_SIZE + maxDataSize;

            uint64 tsKey = NetStatTimestamps::GetKey(header.frameId);
            NetStatTimestamps* timestamps = (statsComp) ? statsComp->GetTimestamps(tsKey) : nullptr;
            header.hasNetStat = (nullptr != timestamps);
            if (header.hasNetStat)
            {
                timestamps->client.sendToNet = static_cast<uint32>(SystemTimer::GetMs());
                maxPacketSize += sizeof(NetStatTimestamps);
            }

            uint32 packetSize = 0;
            std::unique_ptr<uint8[]> packet(new uint8[maxPacketSize]);
            Memcpy(packet.get() + packetSize, &header, INPUT_PACKET_HEADER_SIZE);
            packetSize += INPUT_PACKET_HEADER_SIZE;

            auto prevIt = beginIt;
            for (auto it = beginIt; it != endIt; ++it)
            {
                if (it != beginIt && it.Data() == prevIt.Data())
                {
                    packet[packetSize] = DUPLICATE_INPUT_MARK;
                    packetSize += 1;
                    prevIt = it;
                    continue;
                }
                Memcpy(packet.get() + packetSize, &it.Data(), dataSize);
                packetSize += dataSize;
                prevIt = it;
            }

            if (header.hasNetStat)
            {
                Memcpy(packet.get() + packetSize, reinterpret_cast<void*>(timestamps), sizeof(NetStatTimestamps));
                packetSize += sizeof(NetStatTimestamps);
                statsComp->RemoveTimestamps(tsKey);
            }

            client->Send(packet.get(), packetSize, packetParams);
        }
    }

    if (statsComp)
    {
        statsComp->StartFrameRTTMeasurement(header.frameId);
    }
}
};

#endif
