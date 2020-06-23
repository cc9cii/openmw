#ifndef CSV_FOREIGN_TERRAINSTORAGE_H
#define CSV_FOREIGN_TERRAINSTORAGE_H

#include <components/esm4terrain/storage.hpp>

#include "../../model/world/data.hpp"

namespace ESM4
{
    struct LandTexture;
    typedef std::uint32_t FormId;
}

namespace ESM4Terrain
{
    struct Land;
}

namespace CSVForeign
{

    /**
     * @brief A bridge between the terrain component and OpenCS's terrain data storage.
     */
    class TerrainStorage : public ESM4Terrain::Storage
    {
    public:
        TerrainStorage(const CSMWorld::Data& data, ESM4::FormId world = 0x3c); // assume Tamriel

    private:
        const CSMWorld::Data& mData;
        ESM4::FormId mWorld;

        virtual const ESM4Terrain::Land* getLand (int cellX, int cellY);
        virtual const ESM4::LandTexture* getLandTexture(ESM4::FormId formId);

        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY);

        virtual const ESM4::TextureSet *getTextureSet(ESM4::FormId formId) { return nullptr; } // FIXME
    };

}

#endif
