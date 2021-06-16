#include "terrainstorage.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/foreignland.hpp"


#include "../mwworld/cellstore.hpp"

namespace Foreign
{

    // called from RenderingManager::enableTerrain(bool enable, ESM4::FormId worldId)
    TerrainStorage::TerrainStorage(ESM4::FormId world)
        : mWorld(world)
    {
    }

    // NOTE: CellStore *World::getWorldCell (ESM4::FormId worldId, int x, int y)
    // loads as required (LAND formid comes from loaded CellStore) and we may need a
    // neighbouring land info (whose terrain and objects may not be loaded)
    const ESM4Terrain::Land *TerrainStorage::getLand(int cellX, int cellY)
    {
        const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();
#if 0
        // find the world for the given editor id
        const MWWorld::ForeignWorld *world = esmStore.getForeign<MWWorld::ForeignWorld>().find(mWorld);
        if (!world)
            return; // FIXME: maybe exception?

        // now find the cell's formid for the given x, y
        std::map<std::pair<int, int>, ESM4::FormId>::const_iterator it
            = world->mCells.find(std::make_pair(cellX, cellY));

        if (it == world->mCells.end())
            return 0; // FIXME: maybe exception?

        // get the cell given the formid
        const ForeignCell *cell = esmStore.getForeign<ForeignCell>().find(it->second);
        if (!cell)
            return 0; // FIXME: maybe exception?
#endif
        // FIXME: can't remember why these particular cells are excluded
        //if (!(cellX == 6 && cellY == -65) && !(cellX == 7 && cellY == -65))
        {
            MWWorld::CellStore *cell = MWBase::Environment::get().getWorld()->getWorldCell(mWorld, cellX, cellY);

            // The cell isn't guaranteed to have Land. This is because the terrain implementation
            // has to wrap the vertices of the last row and column to the next cell, which may be a nonexisting cell
            if (cell)
                return esmStore.getForeign<MWWorld::ForeignLand>().search(cell->getForeignLandId());
            else
                return 0;
        }
        //else
            //return 0;
    }

    const ESM4::LandTexture *TerrainStorage::getLandTexture(ESM4::FormId formId)
    {
        const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();

        return esmStore.getForeign<ESM4::LandTexture>().search(formId);
    }

    // FIXME: incorporate this into getLandTexture() instead?
    const ESM4::TextureSet *TerrainStorage::getTextureSet(ESM4::FormId formId)
    {
        const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();

        return esmStore.getForeign<ESM4::TextureSet>().search(formId);
    }

    void TerrainStorage::getBounds(float &minX, float &maxX, float &minY, float &maxY)
    {
        minX = 0, minY = 0, maxX = 0, maxY = 0;

        const MWWorld::ESMStore &esmStore = MWBase::Environment::get().getWorld()->getStore();

        // find the world for the given formid
        const MWWorld::ForeignWorld *world = esmStore.getForeign<MWWorld::ForeignWorld>().find(mWorld);
        if (!world)
            return; // FIXME: maybe exception?

        const std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>& cellGridMap = world->getCellGridMap();
        std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>::const_iterator it = cellGridMap.begin();
        for (; it != cellGridMap.end(); ++it)
        {
            if (it->first.first < minX)
                minX = static_cast<float>(it->first.first);
            if (it->first.first > maxX)
                maxX = static_cast<float>(it->first.first);
            if (it->first.second < minY)
                minY = static_cast<float>(it->first.second);
            if (it->first.second > maxY)
                maxY = static_cast<float>(it->first.second);
        }

        // since grid coords are at cell origin, we need to add 1 cell
        maxX += 1;
        maxY += 1;
    }

}
