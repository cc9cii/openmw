#include "cells.hpp"

#include <iostream> // FIXME: for testing only
#include <stdexcept>

#include <OgreResourceGroupManager.h>

#include <extern/esm4/common.hpp>
#include <extern/esm4/formid.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/cellstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "class.hpp"
#include "esmstore.hpp"
#include "containerstore.hpp"
#include "cellstore.hpp"

#undef LANDSCAPE_LOD_TEST
#undef DISTANT_LOD_TEST

MWWorld::CellStore *MWWorld::Cells::getCellStore (const ESM::Cell *cell)
{
    if (cell->mData.mFlags & ESM::Cell::Interior)
    {
        std::string lowerName(Misc::StringUtils::lowerCase(cell->mName));
        std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

        if (result==mInteriors.end())
        {
            result = mInteriors.insert (std::make_pair (lowerName, std::move(CellStore (cell)))).first;
        }

        return &result->second;
    }
    else
    {
        std::map<std::pair<int, int>, CellStore>::iterator result =
            mExteriors.find (std::make_pair (cell->getGridX(), cell->getGridY()));

        if (result==mExteriors.end())
        {
            result = mExteriors.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(cell->getGridX(), cell->getGridY()),
                                        std::forward_as_tuple(CellStore (cell))).first;
        }

        return &result->second;
    }
}

void MWWorld::Cells::clear()
{
    mInteriors.clear();
    mExteriors.clear();
    std::fill(mIdCache.begin(), mIdCache.end(), std::make_pair("", (MWWorld::CellStore*)0));
    mIdCacheIndex = 0;
}

MWWorld::Ptr MWWorld::Cells::getPtrAndCache (const std::string& name, CellStore& cellStore)
{
    Ptr ptr = getPtr (name, cellStore);

    if (!ptr.isEmpty() && ptr.isInCell())
    {
        mIdCache[mIdCacheIndex].first = name;
        mIdCache[mIdCacheIndex].second = &cellStore;
        if (++mIdCacheIndex>=mIdCache.size())
            mIdCacheIndex = 0;
    }

    return ptr;
}

void MWWorld::Cells::writeCell (ESM::ESMWriter& writer, CellStore& cell) const
{
    if (cell.getState()!=CellStore::State_Loaded)
        cell.load (mStore, mReader);

    ESM::CellState cellState;

    cell.saveState (cellState);

    writer.startRecord (ESM::REC_CSTA);
    cellState.mId.save (writer);
    cellState.save (writer);
    cell.writeFog(writer);
    cell.writeReferences (writer);
    writer.endRecord (ESM::REC_CSTA);
}

MWWorld::Cells::Cells (const MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& reader)
: mStore (store), mReader (reader),
  mIdCache (40, std::pair<std::string, CellStore *> ("", (CellStore*)0)), /// \todo make cache size configurable
  mIdCacheIndex (0)
{}

MWWorld::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().search(x, y);

        if (!cell)
        {
            // Cell isn't predefined. Make one on the fly.
            ESM::Cell record;

            record.mData.mFlags = ESM::Cell::HasWater;
            record.mData.mX = x;
            record.mData.mY = y;
            record.mWater = 0;
            record.mMapColor = 0;

            cell = MWBase::Environment::get().getWorld()->createRecord (record);
        }
        result = mExteriors.emplace(std::piecewise_construct,
                                    std::forward_as_tuple(x, y),
                                    std::forward_as_tuple(CellStore (cell))).first;
    }

    if (result->second.getState()!=CellStore::State_Loaded)
    {
        // Multiple plugin support for landscape data is much easier than for references. The last plugin wins.
        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::CellStore *MWWorld::Cells::getInterior (const std::string& name)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

    if (result==mInteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().search(lowerName);
        if (!cell) // might be a foreign cell
            return getForeignInterior(name);

        result = mInteriors.insert (std::make_pair (lowerName, std::move(CellStore (cell)))).first;
    }

    if (result->second.getState()!=CellStore::State_Loaded)
    {
        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::CellStore *MWWorld::Cells::getCell (const ESM::CellId& id)
{
    if (id.mPaged)
        return getExterior (id.mIndex.mX, id.mIndex.mY);

    return getInterior (id.mWorldspace);
}

// in line with COE to COW
MWWorld::CellStore *MWWorld::Cells::getWorldCellGrid (const std::string& worldName, std::int16_t x, std::int16_t y)
{
#if 0
    // -- start of tests
    {
    std::string lowerWorld = Misc::StringUtils::lowerCase(world);
    const MWWorld::ForeignWorld *foreignWorld = mStore.getForeign<MWWorld::ForeignWorld>().find(lowerWorld);
    if (!foreignWorld)
        return 0; // FIXME: print an error? maybe it should have been tested before this method was called and we should throw instead

    std::map<std::pair<int, int>, ESM4::FormId>::const_iterator it = foreignWorld->mCells.begin();
    for (; it != foreignWorld->mCells.end(); ++it)
    {
        std::cout << "cell: " << ESM4::formIdToString(it->second)
            << std::dec << ", x: " << it->first.first << ", y: " << it->first.second
            << std::endl; // FIXME: debug

        if (it->first.first == x && it->first.second == y)
        {
            const MWWorld::ForeignCell *foreignCell = mStore.getForeign<MWWorld::ForeignCell>().find(it->second);
            if (!foreignCell)
                return 0; // FIXME: print an error? maybe it should have been tested before this method was called and we should throw instead

            std::cout << "cell found: " << ESM4::formIdToString(it->second) << std::endl;
        }

    //return 0;
    }
    }
    // -- end of tests
#endif
    ESM4::FormId worldId = mStore.getForeign<ForeignWorld>().getFormId(worldName);

    return getWorldCellGrid(worldId, x, y);
}

void MWWorld::Cells::initNewWorld(const ForeignWorld *world)
{
//#if 0
    std::vector<std::string> locations;
    std::vector<std::string> files;

    const Ogre::ResourceGroupManager& groupMgr = Ogre::ResourceGroupManager::getSingleton();
    const Ogre::StringVector groups = groupMgr.getResourceGroups();

    // FIXME: how to ensure the data directory is read *after* the BSA archives?
    Ogre::StringVector::const_reverse_iterator it = groups.rbegin();
    for (; it != groups.rend(); ++it)
    {
        // FIXME: this probably ignores loose files
        if (1)//(*it).find("TES4BSA") != std::string::npos)
            locations.push_back(*it);
    }

    for (std::size_t i = 0; i < locations.size(); ++i)
    {
#ifdef LANDSCAPE_LOD_TEST
        std::string worldIdDecimal = std::to_string(unsigned int(world->mFormId));

        // TES5 seems to use meshes\terrain\<worldname>\<type>.<size>.<x>.<y>.btr
        // where type can be objects, trees, <worldname> (e.g. tamriel)
        // the .btr file looks like a NIF, starting with BSMultiBoundNode (Nifskope can open)
        const Ogre::StringVectorPtr res
            //= groupMgr.findResourceNames(locations[i], "meshes\\landscape\\lod\\"+worldIdDecimal+"*");
            = groupMgr.findResourceNames(locations[i], "meshes\\landscape\\lod\\*"); // TES4
        Ogre::StringVector::const_iterator it = res->begin();
        for (; it != res->end(); ++it)
        {
            std::int32_t lod[4];
            std::size_t next = 21; // 'meshes\landscape\lod\', TES4
            std::size_t pos = 0;
            for (std::size_t j = 0; j < 4; ++j)
            {
                pos = (*it).find_first_of(".", next);
                if (pos == std::string::npos)
                    break; // j

                lod[j] = std::stoi((*it).substr(next, pos-(next)), nullptr, 10);
                next = pos+1;
            }

            if (1)//lod[0] != 0x3c)
            {
                std::cout << ESM4::formIdToString(lod[0]) << " "
                          << lod[1] << "," << lod[2] << "," << lod[3] << std::endl;
            }
        }
#endif // LANDSCAPE_LOD_TEST
        // TES5 doesn't seem to have a file equivalent to ".cmp"
        // FIXME: this initialises *all* worlds, not just the parameter 'world'
        const Ogre::StringVectorPtr res2
            //= groupMgr.findResourceNames(locations[i], "distantlod\\"+world->mEditorId+"*");
            = groupMgr.findResourceNames(locations[i], "distantlod\\*"); // TES4
        Ogre::StringVector::const_iterator it2 = res2->begin();
        for (; it2 != res2->end(); ++it2)
        {
            std::size_t pos = (*it2).find(".cmp"); // TES4
            if (pos != std::string::npos)
            {
                std::string worldEditorId = (*it2).substr(11, pos-11); // 11 for 'distantlod\', TES4
                if (Misc::StringUtils::lowerCase(world->mEditorId) == worldEditorId)
                    const_cast<ForeignWorld*>(world)->addVisibleDistGrids(*it2, locations[i]); // FIXME const

#if 0
                //std::cout << locations[i] << " " << *it2 << std::endl;

                Ogre::DataStreamPtr file = groupMgr.openResource(*it2, locations[i]);

                std::map<std::string, std::vector<std::pair<std::int16_t, std::int16_t> > >::iterator lb
                    = mVisibleDistStaticGrids.lower_bound(worldEditorId);

                if (lb != mVisibleDistStaticGrids.end()
                        && !(mVisibleDistStaticGrids.key_comp()(worldEditorId, lb->first)))
                {
#if 0
                    while (!file->eof())
                    {
                        std::int16_t x, y;
                        file->read(&y, sizeof(y)); // NOTE: that y comes before x
                        file->read(&x, sizeof(x));
                        lb->second.push_back(std::pair<std::int16_t, std::int16_t>(x, y));
                        //std::cout << "(" << x << "," << y << ")" << std::endl;
                    }
#else
                    // FIXME: redesign so that it doesn't get called many times
                    //throw std::runtime_error ("initNewWorld repeat");
#endif
                }
                else // world EditorId not found, create an entry
                {
                    std::vector<std::pair<std::int16_t, std::int16_t> > grid;
                    while (!file->eof())
                    {
                        std::int16_t x, y;
                        file->read(&y, sizeof(y)); // NOTE: that y comes before x
                        file->read(&x, sizeof(x));
                        grid.push_back(std::pair<std::int16_t, std::int16_t>(x, y));
                        //std::cout << "(" << x << "," << y << ")" << std::endl;
                    }
                    // FIXME: either don't bother with the last entry or do something with it
                    //std::cout << worldEditorId << std::endl;
                    mVisibleDistStaticGrids.insert(lb, std::make_pair(worldEditorId, grid));
                }
#endif
            }
            else
            {
//#if 0
                if ((*it2).find(".lod") == std::string::npos) // FIXME: for testing only
                    throw std::runtime_error ("unknown file type in distantlod");
//#endif
                std::size_t pos = (*it2).find_first_of("_");
                if (pos == std::string::npos)
                    continue; // it2

                std::string editorId = (*it2).substr(11, pos-11); // 11 for 'distantlod\'

                std::int32_t lod[2]; // lod[0] is x and lod[1] is y
                //std::size_t next = 11+world->mEditorId.size()+1;
                std::size_t next = 11+editorId.size()+1;
                for (std::size_t j = 0; j < 2; ++j)
                {
                    pos = (*it2).find_first_of("_.", next);
                    if (pos == std::string::npos)
                        break; // probably should throw

                    lod[j] = std::stoi((*it2).substr(next, pos-(next)), nullptr, 10);
                    next = pos+1;
                }
#ifdef DISTANT_LOD_TEST
                struct Refr
                {
                    ESM4::FormId baseObj;

                    // FIXME: these should be a vector as there can be many instances
                    float posX;
                    float posY;
                    float posZ;
                    float rotX;
                    float rotY;
                    float rotZ;
                    float scale;
                };

                Ogre::DataStreamPtr file = groupMgr.openResource(*it2, locations[i]);

                std::uint32_t numObj;
                file->read(&numObj, sizeof(numObj));
                for (std::size_t j = 0; j < numObj; ++j)
                {
                    Refr r;
                    file->read(&(r.baseObj), sizeof(ESM4::FormId));

                    std::uint32_t numInst;
                    file->read(&numInst, sizeof(numInst));
                    for (std::size_t k = 0; k < numInst; ++k)
                    {
                        file->read(&(r.posX), sizeof(float));
                        file->read(&(r.posY), sizeof(float));
                        file->read(&(r.posZ), sizeof(float));
                    }
                    for (std::size_t k = 0; k < numInst; ++k)
                    {
                        file->read(&(r.rotX), sizeof(float));
                        file->read(&(r.rotY), sizeof(float));
                        file->read(&(r.rotZ), sizeof(float));
                    }
                    for (std::size_t k = 0; k < numInst; ++k)
                    {
                        file->read(&(r.scale), sizeof(float));
                    }
                }
                std::cout << "lod " << *it2 << " " << numObj << std::endl;
#endif // DISTANT_LOD_TEST
            }
        }
    }
    // FIXME: doesn't belong here?
//#if 0
    std::map<ESM4::FormId, CellStore>::iterator lb = mForeignDummys.lower_bound(world->mFormId);
    if (lb == mForeignDummys.end() || (mForeignDummys.key_comp()(world->mFormId, lb->first)))
    {
        const ForeignCell *dummyCell = world->getDummyCell();
        if (dummyCell)
        {
            mForeignDummys.insert(lb, { world->mFormId, CellStore(dummyCell, true/*foreign*/, true/*dummy*/) });
        }
        else
            throw std::runtime_error("Dummy cell not found for this world");
    }
//#endif
}

// NOTE: creates a new CellStore if it doesn't exist but do not load (can't load here becasue
//       not all mod files would have been loaded when the world is first created)
//
//       the caller needs to load() if required
//
MWWorld::CellStore *MWWorld::Cells::getWorldCellGrid(ESM4::FormId worldId, std::int16_t x, std::int16_t y)
{
    // find the world for the given form id
    const ForeignWorld *world = mStore.getForeign<ForeignWorld>().find(worldId);
    if (!world)
        return nullptr; // FIXME: maybe exception?

    // now find the cell's formid for the given x, y
    const std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>& cellGridMap = world->getCellGridMap();
    std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>::const_iterator it
        = cellGridMap.find(std::make_pair(x, y));

    if (it == cellGridMap.end())
        return nullptr; // FIXME: maybe exception?

    // get the cell given the formid
    const ForeignCell *cell = mStore.getForeign<ForeignCell>().search(it->second);
    if (!cell)
        return nullptr; // FIXME: maybe exception?

    CellStore *cellStore;

    // does the world exist?
    std::map<ESM4::FormId, CellGridMap>::iterator lb = mForeignExteriors.lower_bound(worldId);
    if (lb != mForeignExteriors.end() && !(mForeignExteriors.key_comp()(worldId, lb->first)))
    {
        // found world
        std::pair<CellGridMap::iterator, bool> res
            = lb->second.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(x, y),
                                 std::forward_as_tuple(CellStore(cell, true/*foreign*/)));

        cellStore = &res.first->second;

        // There may be a CellStore already at that location!
        // This can happen if a mod updates the record, just overwrite it for now // FIXME
        // FIXME: Is CellStore constructed twice here?
        // FIXME: always loading!!!
        //if (!res.second)
            //res.first->second = CellStore(cell, true); // can't use [] as it needs a default constructor
    }
    else // insert a new world
    {
        initNewWorld(world); // also inserts the dummy cell (but not loaded)

        CellGridMap ci;
        ci.emplace(std::piecewise_construct,
            std::forward_as_tuple(x, y),
            std::forward_as_tuple(CellStore(cell, true/*foreign*/)));

        mForeignExteriors.insert(lb,
                std::map<ESM4::FormId, CellGridMap>::value_type(world->mFormId, std::move(ci)));

        CellGridMap::iterator iter = mForeignExteriors[worldId].find(std::pair<int, int>(x, y));
        cellStore = &iter->second;
    }

    return cellStore;
}

MWWorld::CellStore *MWWorld::Cells::getWorldDummyCell (ESM4::FormId worldId)
{
    std::map<ESM4::FormId, CellStore>::iterator it = mForeignDummys.find(worldId);
    if (it != mForeignDummys.end())
        return &it->second;

    // check if the world exists and init as required
    const ForeignWorld *world = mStore.getForeign<ForeignWorld>().find(worldId);
    if (world)
    {
        initNewWorld(world);

        // duplicated code, is there a better way?
        std::map<ESM4::FormId, CellStore>::iterator it = mForeignDummys.find(worldId);
        if (it != mForeignDummys.end())
            return &it->second;
    }

    return nullptr;
}

// If one does not exist insert in mForeignInteriors. Load as required.
MWWorld::CellStore *MWWorld::Cells::getForeignInterior (const std::string& name)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    std::map<std::string, CellStore>::iterator result = mForeignInteriors.find(lowerName);

    if (result == mForeignInteriors.end())
    {
        const ForeignCell *cell = mStore.getForeign<ForeignCell>().search(lowerName);

        result = mForeignInteriors.insert(std::make_pair(lowerName, std::move(CellStore(cell, true)))).first;
    }

    return &result->second;
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name, CellStore& cell,
    bool searchInContainers)
{
    if (cell.getState()==CellStore::State_Unloaded)
        cell.preload (mStore, mReader);

    if (cell.getState()==CellStore::State_Preloaded)
    {
        if (cell.hasId (name))
        {
            cell.load (mStore, mReader);
        }
        else
            return Ptr();
    }

    Ptr ptr = cell.search (name);

    if (!ptr.isEmpty())
        return ptr;
    else
    {
        ptr = cell.searchViaEditorId(name);

        if (!ptr.isEmpty())
            return ptr;
    }

    if (searchInContainers)
        return cell.searchInContainer (name);

    return Ptr();
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name)
{
    // First check the cache
    for (std::vector<std::pair<std::string, CellStore *> >::iterator iter (mIdCache.begin());
        iter!=mIdCache.end(); ++iter)
        if (iter->first==name && iter->second)
        {
            Ptr ptr = getPtr (name, *iter->second);
            if (!ptr.isEmpty())
                return ptr;
        }

    // Then check cells that are already listed
    // Search in reverse, this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
    // there is one at -22,16 and one at -2,-9, the latter should be used.
    for (std::map<std::pair<int, int>, CellStore>::reverse_iterator iter = mExteriors.rbegin();
        iter!=mExteriors.rend(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (std::map<std::string, CellStore>::iterator iter = mInteriors.begin();
        iter!=mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    MWWorld::Store<ESM::Cell>::iterator iter;

    for (iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}

void MWWorld::Cells::getExteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

void MWWorld::Cells::getInteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

int MWWorld::Cells::countSavedGameRecords() const
{
    int count = 0;

    for (std::map<std::string, CellStore>::const_iterator iter (mInteriors.begin());
        iter!=mInteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    for (std::map<std::pair<int, int>, CellStore>::const_iterator iter (mExteriors.begin());
        iter!=mExteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    return count;
}

void MWWorld::Cells::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
{
    for (std::map<std::pair<int, int>, CellStore>::iterator iter (mExteriors.begin());
        iter!=mExteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell (writer, iter->second);
            progress.increaseProgress();
        }

    for (std::map<std::string, CellStore>::iterator iter (mInteriors.begin());
        iter!=mInteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell (writer, iter->second);
            progress.increaseProgress();
        }
}

bool MWWorld::Cells::readRecord (ESM::ESMReader& reader, uint32_t type,
    const std::map<int, int>& contentFileMap)
{
    if (type==ESM::REC_CSTA)
    {
        ESM::CellState state;
        state.mId.load (reader);

        CellStore *cellStore = 0;

        try
        {
            cellStore = getCell (state.mId);
        }
        catch (...)
        {
            // silently drop cells that don't exist anymore
            reader.skipRecord();
            return true;
            /// \todo log
        }

        state.load (reader);
        cellStore->loadState (state);

        if (state.mHasFogOfWar)
            cellStore->readFog(reader);

        if (cellStore->getState()!=CellStore::State_Loaded)
            cellStore->load (mStore, mReader);

        cellStore->readReferences (reader, contentFileMap);

        return true;
    }

    return false;
}
