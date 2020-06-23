#include "cellcollection.hpp"

#include <stdexcept>
#include <cassert>

#include <iostream> // FIXME
#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>
#include <extern/esm4/common.hpp>

#include "../world/record.hpp"
#include "../world/universalid.hpp"

#include "worldcollection.hpp"
#include "cellgroupcollection.hpp"

CSMForeign::CellCollection::CellCollection (WorldCollection& worlds, CellGroupCollection& cellGroups)
 : mWorlds(worlds), mCellGroups(cellGroups)
{
}

CSMForeign::CellCollection::~CellCollection ()
{
}

// http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format/CELL
//
// The block and subblock groups for an interior cell are determined by the last two decimal
// digits of the lower 3 bytes of the cell form ID (the modindex is not included in the
// calculation). For example, for form ID 0x000CF2=3314, the block is 4 and the subblock is 1.
//
// The block and subblock groups for an exterior cell are determined by the X-Y coordinates of
// the cell. Each block contains 16 subblocks (4x4) and each subblock contains 64 cells (8x8).
// So each block contains 1024 cells (32x32).

int CSMForeign::CellCollection::load (ESM4::Reader& reader, bool base)
{
    // load the record from file
    CSMForeign::Cell record;
    IdCollection<Cell>::loadRecord(record, reader);

    // check if it is deleted and process accordingly
    int index = searchFormId(record.mFormId);
    if ((record.mFlags & ESM4::Rec_Deleted) != 0)
    {
        std::cout << "cell deleted " << record.mId << std::endl; // FIXME: debug only
        if (index == -1)
        {
            // deleting a record that does not exist
            // ignore it for now
            /// \todo report the problem to the user
            return -1;
        }

        if (base)
        {
            this->removeRows(index, 1);
            return -1;
        }

        // existing but not base
        std::unique_ptr<CSMWorld::Record<Cell> > baseRecord(new CSMWorld::Record<Cell>(this->getRecord(index)));
        baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
        this->setRecord(index, std::move(baseRecord));
        return index;
    }

    // cache the cell's formId to its parent world
    // FIXME: Is this the right place to cache the formId?  Is there an alternative where a map
    // is maintained (in CellCollection) so that all the cells (or cell groups) for a given
    // world can be retrieved? e.g.:
    //
    // std::unordered_map<ESM4::FormId, std::vector<ESM4::FormId> > mCellsInWorldMap;
    //
    // std::vector<ESM4::FormId> getCells(ESM4::FormId world) const
    // {
    //     const std::unordered_map<ESM4::FormId, std::vector<ESM4::FormId> >::iterator it
    //         = mCellsInWorldMap.find(world);
    //
    //     if (it != mCellsinWorldMap.end())
    //         return *it;
    //     else
    //         return 0;
    // }
    //
    // Returing a copy of a vector may not be so efficient, but then this isn't a frequent
    // operation (hopefully only when changing a world space). Also the compiler may use move
    // semantics automatically (if using C++11).
    //
    World *world = mWorlds.getWorld(reader.currWorld()); // FIXME: const issue with Collection
    if (world)
        world->mCells.push_back(record.mFormId);

    // reader.currCellGrid() is set during the load, i.e. loadRecord (sub record XCLC for an exterior cell)
    if (reader.hasCellGrid())
    {
#if 0
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_WorldChild) && "Unexpected group while loading cell");
#else
        // DLCFrostcrag.esp has Grp_CellTemporaryChild here
        if ((reader.grp().type != ESM4::Grp_ExteriorSubCell &&
                reader.grp().type != ESM4::Grp_WorldChild))
            std::cout << "unexpected group " << reader.grp().type << std::endl;
#endif

        ESM4::FormId worldId = reader.currWorld();
        std::map<ESM4::FormId, CoordinateIndex>::iterator lb = mPositionIndex.lower_bound(worldId);

        if (lb != mPositionIndex.end() && !(mPositionIndex.key_comp()(worldId, lb->first)))
        {
            std::pair<CoordinateIndex::iterator, bool> res = lb->second.insert(
                { std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), record.mFormId });

            // sometimes there are more than one cell with the same co-ordinates
            // use the one with the editor id
            //
            // FIXME: this workaround is ok for regionmaps, etc, but also need a way to index
            // the other one as well
            if (!res.second)
            {
                if (!record.mEditorId.empty())
                {
                    // first check if both have editor id's
                    // FIXME: check if a mod is overwriting the original record
                    //if (!getRecord(searchFormId(res.first->second)).get().mEditorId.empty())
                        //std::cout << "two editor ids" << std::endl;
                    res.first->second = record.mFormId;
                }
                else
                {
                    // check if both empty
                    if (getRecord(searchFormId(res.first->second)).get().mEditorId.empty())
                    {
#if 0
                        std::cout << "world " << worldId << ", x " << res.first->first.first
                            << ", y " << res.first->first.second << std::endl;
                        std::cout << "cell " << ESM4::formIdToString(id) << ", x " << reader.currCellGrid().grid.x
                            << ", y " << reader.currCellGrid().grid.y << std::endl;
#endif
                    }
                }
            }
        }
        else
            mPositionIndex.insert(lb, std::map<ESM4::FormId, CoordinateIndex>::value_type(worldId,
                { {std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), record.mFormId } }));

        ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, record.mCellId);

        // FIXME: use the loading sequence to then validate using the existance of cell grid
        record.isInterior = false;
    }
    else
    {
        // Toddland, EmptyWorld, Bloated Float at Sea, Skingrad, Plane of Oblivion, Bravil,
        // etc, etc, are in Grp_WorldChild but without any grids
        //
        // TestRender, EmptyCell, OblivionMQKvatchBridge, thehill, Hawkhaven02, 000009bf, 0000169f,
        // etc, etc, are in Grp_ExteriorSubCell but witout any grids
#if 0
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_InteriorSubCell ||
                reader.grp().type == ESM4::Grp_WorldChild) && "Unexpected group while loading cell");
#else
        // DLCFrostcrag.esp has Grp_CellTemporaryChild here
        if ((reader.grp().type != ESM4::Grp_ExteriorSubCell &&
                reader.grp().type != ESM4::Grp_InteriorSubCell &&
                reader.grp().type != ESM4::Grp_WorldChild))
            std::cout << "unexpected group " << reader.grp().type << std::endl;
#endif

        if (!record.mEditorId.empty()) // can't use Full Name since they are not unique
            record.mCellId = record.mEditorId; // FIXME: check if editor id's are uplicated
        else
            record.mCellId = ESM4::formIdToString(record.mFormId); // use formId string instead of "#x y"

        // FIXME: use the loading sequence to then validate using the existance of cell grid
        record.isInterior = true;

// FIXME: for testing internal cell block and sub-block numbers
//
// There doesn't seem to be any logical grouping of the cells within the block or sub-block.  e.g.
//
//  Starting record group Interior Cell: block 0x0
//  Starting record group Interior Sub Cell: block 0x0
//
//  EditorId: XPAichan01
//  EditorId: XPSwampgasHole03
//  EditorId: SageGlenHollow
//  EditorId: Elenglynn
//  EditorId: TestCheydinhalUpper
//  EditorId: ChorrolMarkTest
//  EditorId: KvatchChapelUndercroft
//  EditorId: ICArcaneUniversitySpellmaker
//  EditorId: GoblinJimsCave
//  EditorId: ICTempleDistrictSeridursHouseUpstairs
//  EditorId: ICImperialLegionWatchTowerNECaptainsQuarters
//  EditorId: HackdirtMoslinsDryGoodsBasement
#if 0
        int block = record.mFormId % 100; // FIXME: for debugging only
        std::cout << "CELL block: " << block % 10 << ", sub block: " << block / 10 << ", EditorId: "
            << (record.mEditorId.empty() ? "" : record.mEditorId) << std::endl; // FIXME: debug
        //std::cout << "CELL formId in decimal: " << std::dec << record.mFormId << std::endl; // FIXME: debug
#endif
    }

    record.mWorld = mWorlds.getIdString(record.mParent); // FIXME: assumes our world is already loaded
    record.mWorldFormId = ESM4::formIdToString(record.mParent);
    if (!record.mRegions.empty())
        ESM4::formIdToString(record.mRegions.back(), record.mRegion);

    // cache the record's formId to its cell group
    int cellIndex = mCellGroups.searchFormId(record.mFormId);
    if (cellIndex == -1)
    {
        using CSMWorld::Record;

        // new cell group
        CellGroup cellGroup;
        cellGroup.mFormId = record.mFormId; // same as CellGroup's CELL
        cellGroup.mId = ESM4::formIdToString(record.mFormId);

        std::unique_ptr<Record<CellGroup> > record2(new Record<CellGroup>);
        record2->mState = CSMWorld::RecordBase::State_BaseOnly;
        record2->mBase = cellGroup;

        mCellGroups.insertRecord(std::move(record2), mCellGroups.getSize());
    }
    // FIXME: below only needed if we're modifyng something
    // Maybe we can't replace but have to merge?
    else
    {
#if 0
        // existing cell group
        std::unique_ptr<Record<CellGroup> > record2(new Record<CellGroup>(mCellGroups.getRecord(cellIndex)));
        record2->mState = CSMWorld::RecordBase::State_ModifiedOnly;
        CellGroup &cellGroup = record2->get();

        mCellGroups.setRecord(cellIndex, std::move(record2));
#endif
    }

    if (record.mFormId == 0x00007be6)
        std::cout << "#-1 14" << std::endl;

    // load the record to the collection
    // FIXME: trouble here is that a cell is not a single record but a collection of things
    // e.g. Knights.esp adds a XCLR subrecord (not replace) to cell 00006599 (WenyandawikExterior)
    // and it has child GRUP's to replaces REFR 00188ec0 (ARRingOuterWall03)
    return IdCollection<Cell>::load(record, base, index);
}

ESM4::FormId CSMForeign::CellCollection::searchCoord (std::int16_t x, std::int16_t y, ESM4::FormId world) const
{
    std::map<ESM4::FormId, CoordinateIndex>::const_iterator iter = mPositionIndex.find(world);
    if (iter == mPositionIndex.end())
        return 0; // can't find world // FIXME: exception instead?

    CoordinateIndex::const_iterator it = iter->second.find(std::make_pair(x, y));
    if (it == iter->second.end())
        return 0; // cann't find coordinate // FIXME: exception instead?

    return it->second;
}

CSMForeign::Cell *CSMForeign::CellCollection::getCell(ESM4::FormId formId)
{
    int index = searchFormId(formId);
    if (index == -1)
        return nullptr;

    return &getModifiableRecord(index).get();
}
