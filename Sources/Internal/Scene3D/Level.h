#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Base/Any.h"
#include "Base/BaseTypes.h"
#include "Math/AABBox3.h"
#include "Reflection/Reflection.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class DataNode;
class Scene;
class Camera;
class LevelEntity;

class Level : public AssetBase
{
public:
    struct Key
    {
        Key() = default;
        Key(const FilePath& filepath_)
            : path(filepath_)
        {
        }
        FilePath path;
    };

    Level(const Any& assetKey);

    /*
        Targets: up to 4 millions of objects.

        File structure
        [Header]
        [DataNodeInfoTable]
        [ChunkInfoTable]
        [Entities]
    */
    struct Header
    {
        enum
        {
            CURRENT_VERSION = 2,
            SUPPORTED_VERSION = 2,
        };

        char8 signature[4] = { 'L', 'V', 'L', 'B' };
        uint32 version = CURRENT_VERSION;
        uint32 allEntitiesCount = 0; // number of entities in whole map
        AABBox3 worldBounds;
    };

    /*
        Estimation for table size is 6MB per 500.000 entities.
        Average objects count - 80~100 objects per block 100x100 meters.
        8km x 8km = 80 x 80 chunks * 4bytes * 128 indices = 3.2MB.
        During development Chunks can have place up to 256 objects each, and during export shrink indices.
    */

    struct Chunk
    {
        enum eState
        {
            STATE_NOT_REQUESTED,
            STATE_REQUESTED,
        };

        Chunk() = default;

        Chunk(const Chunk& other);
        Chunk(Chunk&& other);

        Chunk& operator=(const Chunk& other);
        Chunk& operator=(Chunk&& other);

        Vector<uint32> entitiesIndices;
        Vector<Asset<LevelEntity>> entitiesLoaded;

        uint32 state = STATE_NOT_REQUESTED;

        uint32 visitedLastFrameIndex = 0;
    };

    struct ChunkCoord
    {
        ChunkCoord() = default;
        ChunkCoord(int32 x_, int32 y_)
            : x(x_)
            , y(y_){};

        bool operator==(const ChunkCoord& other) const
        {
            return x == other.x && y == other.y;
        }
        bool operator!=(const ChunkCoord& other) const
        {
            return x != other.x || y != other.y;
        }

        bool operator<(const ChunkCoord& other) const
        {
            if (x != other.x)
            {
                return x < other.x;
            }

            return y < other.y;
        }

        int32 x = 0;
        int32 y = 0;
    };

    struct ChunkBounds
    {
        ChunkCoord min = ChunkCoord(0, 0);
        ChunkCoord max = ChunkCoord(-1, -1);

        bool operator==(const ChunkBounds& other) const
        {
            return min == other.min && max == other.max;
        }
        bool operator!=(const ChunkBounds& other) const
        {
            return min != other.min || max != other.max;
        }
    };

    struct EntityInfo
    {
        uint32 fileOffset;
        uint32 fileSize;
        int16 boundMinX;
        int16 boundMinY;
        int16 boundMinZ;
        int16 boundMaxX;
        int16 boundMaxY;
        int16 boundMaxZ;
    };

    struct ChunkGrid
    {
        const float32 chunkSize = 100.0f; // 100x100 square meters
        ChunkGrid() = default;

        void SetWorldBounds(const AABBox3& newWorldBounds);

        uint32 GetChunkAddress(const ChunkCoord& coord) const;
        Chunk* GetChunk(uint32 address);
        Chunk* GetChunk(const ChunkCoord& coord);
        ChunkBounds ProjectBoxOnGrid(const AABBox3& entityBox) const;
        ChunkCoord GetChunkCoord(const Vector3& position) const;

        ChunkBounds worldChunkBounds;
        uint32 chunkXCount = 0;
        uint32 chunkYCount = 0;
        AABBox3 worldBounds;
        Chunk specialStreamingSettingsChunk;
        Vector<Chunk> chunkData;
    };

    // Actual level vars
    ChunkGrid loadedChunkGrid;
    Vector<EntityInfo> loadedInfoArray;
    UnorderedMap<uint32, Entity*> entitiesAddedToScene;
    RefPtr<File> levelFile;

    DAVA_VIRTUAL_REFLECTION(Level, AssetBase);
};

class LevelEntity : public AssetBase
{
public:
    struct Key
    {
        Key() = default;
        Key(Level* level_, uint32 entityIndex_)
            : level(level_)
            , entityIndex(entityIndex_)
        {
        }
        Level* level;
        uint32 entityIndex;
    };

    LevelEntity(const Any& assetKey);
    ~LevelEntity();

    Entity* rootEntity = nullptr;

    DAVA_VIRTUAL_REFLECTION(LevelEntity, AssetBase);
};

template <>
bool AnyCompare<Level::Key>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<Level::Key>;

template <>
bool AnyCompare<LevelEntity::Key>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);
extern template struct AnyCompare<LevelEntity::Key>;

} // namespace DAVA
