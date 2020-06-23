#include "terrainstorage.hpp"

#include "../../model/foreign/land.hpp"

namespace CSVForeign
{

    TerrainStorage::TerrainStorage(const CSMWorld::Data &data, ESM4::FormId world)
        : mData(data), mWorld(world)
    {
    }

    const ESM4Terrain::Land *TerrainStorage::getLand(int cellX, int cellY)
    {
        // The cell isn't guaranteed to have Land. This is because the terrain implementation
        // has to wrap the vertices of the last row and column to the next cell, which may be a nonexisting cell
        int index = mData.getForeignLands().searchId(cellX, cellY, mWorld);
        if (index == -1)
            return NULL;

        const CSMForeign::Land& land = mData.getForeignLands().getRecord(index).get();
        //int mask = ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX;
        //land.loadData (mask);
        return &land;
    }

    const ESM4::LandTexture *TerrainStorage::getLandTexture(ESM4::FormId formId)
    {
        int numRecords = mData.getLandTextures().getSize();

        for (int i = 0; i < numRecords; ++i)
        {
            const CSMForeign::IdRecord<ESM4::LandTexture> *ltex = &mData.getForeignLandTextures().getRecord(i).get();
            if (ltex->mFormId == formId/* && ltex->mPluginIndex == plugin*/)
                return ltex;
        }

        return NULL;
    }

    void TerrainStorage::getBounds(float &minX, float &maxX, float &minY, float &maxY)
    {
        // not needed at the moment - this returns the bounds of the whole world, but we only edit individual cells
        throw std::runtime_error("getBounds not implemented");
    }

}
