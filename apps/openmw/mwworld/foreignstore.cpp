#include "foreignstore.hpp"
#include "esmstore.hpp"

#include <stdexcept>
#include <sstream>
#include <iostream> // FIXME: for debugging

#include <components/esm/esmreader.hpp>
#include <components/esm/esm4reader.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include <extern/esm4/pgrd.hpp> // this one is not in esmstore
#include <extern/esm4/cell.hpp>
#include <extern/esm4/formid.hpp>

#include "cellstore.hpp"

//#define DEBUG_FORMID
#undef DEBUG_FORMID
#undef TEST_GROUP_HEADER_LABELS

namespace MWWorld
{
    ForeignId::ForeignId(ESM4::FormId formId, bool isDeleted)
        : mId(formId), mIsDeleted(isDeleted)
    {}

    template<typename T>
    ForeignStore<T>::ForeignStore()
    {
    }

    template<typename T>
    ForeignStore<T>::ForeignStore(const ForeignStore<T>& orig)
        : mStatic(orig.mStatic)
    {
    }

    template<typename T>
    void ForeignStore<T>::clearDynamic()
    {
        // remove the dynamic part of mShared
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        mDynamic.clear();
    }

    template<typename T>
    const T *ForeignStore<T>::search(const std::string &id) const
    {
        // FIXME: just loop through for now, will need to maintain two maps
        // FIXME: what happens if there are more than one record with the same editor id?
        typename Dynamic::const_iterator dit = mDynamic.begin();
        for (; dit != mDynamic.end(); ++dit)
        {
            if (dit->second.mEditorId == id)
                return &dit->second;
        }

        typename Static::const_iterator it = mStatic.begin();
        for (; it != mStatic.end(); ++it)
        {
            if (it->second.mEditorId == id)
                return &it->second;
        }

        return nullptr;
    }

    template<typename T>
    const T *ForeignStore<T>::search(ESM4::FormId id) const
    {
        // NOTE: below logic is lifted from MWWorld::Store; the differences are that
        // FormId is used instead of std::string and hence no need to convert to lowercase
        typename Dynamic::const_iterator dit = mDynamic.find(id);
        if (dit != mDynamic.end())
            return &dit->second;

        typename Static::const_iterator it = mStatic.find(id);
        if (it != mStatic.end())
            return &(it->second);

        return nullptr;
    }

    template<typename T>
    bool ForeignStore<T>::isDynamic(const std::string &id) const
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        typename Dynamic::const_iterator dit = mDynamic.find(formId);
        return (dit != mDynamic.end());
    }

    template<typename T>
    const T *ForeignStore<T>::find(const std::string &id) const
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        const T *ptr = search(formId);
        if (ptr == nullptr) {
            std::ostringstream msg;
            // FIXME: getRecordType() not implemented
            msg << /*T::getRecordType() <<*/ " '" << id << "' not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }

    template<typename T>
    const T *ForeignStore<T>::findRandom(const std::string &id) const
    {
        std::ostringstream msg;
        // FIXME: getRecordType() not implemented
        msg << /*T::getRecordType() <<*/ " starting with '"<<id<<"' not found";
        throw std::runtime_error(msg.str());
    }

    template<typename T>
    RecordId ForeignStore<T>::load(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        ForeignId result = loadForeign(reader);

        std::string id = ESM4::formIdToString(result.mId);
        return RecordId(id, result.mIsDeleted); // NOTE: id is uppercase (not that it matters)
    }

    template<typename T>
    ForeignId ForeignStore<T>::loadForeign(ESM4::Reader& reader)
    {
        T record;
        record.load(reader);

        bool isDeleted = (record.mFlags & ESM4::Rec_Deleted) != 0;

        std::pair<typename Static::iterator, bool> inserted
            = mStatic.insert(std::make_pair(record.mFormId, record));

        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = record;

        return ForeignId(record.mFormId, isDeleted);
    }

    template<typename T>
    void ForeignStore<T>::setUp()
    {
        // FIXME
    }

    template<typename T>
    typename ForeignStore<T>::iterator ForeignStore<T>::begin() const
    {
        return mShared.begin();
    }

    template<typename T>
    typename ForeignStore<T>::iterator ForeignStore<T>::end() const
    {
        return mShared.end();
    }

    template<typename T>
    size_t ForeignStore<T>::getSize() const
    {
        return mShared.size();
    }

    template<typename T>
    int ForeignStore<T>::getDynamicSize() const
    {
        return (int) mDynamic.size();
    }

    template<typename T>
    void ForeignStore<T>::listForeignIdentifier(std::vector<ESM4::FormId> &list) const
    {
        list.reserve(list.size() + getSize());
        typename std::vector<T *>::const_iterator it = mShared.begin();
        for (; it != mShared.end(); ++it) {
            list.push_back((*it)->mFormId);
        }
    }

    // Used by ESMStore::setUp() to map references to stores (of referenceable object types)
    // Basically pulls all the EditorId strings out of the records and puts them in the list.
    //
    // FIXME: is this useful at all?
    template<typename T>
    void ForeignStore<T>::listIdentifier(std::vector<std::string> &list) const
    {
        list.reserve(list.size() + getSize());
        typename std::vector<T *>::const_iterator it = mShared.begin();
        for (; it != mShared.end(); ++it) {
            list.push_back((*it)->mEditorId);
        }
    }

    template<typename T>
    T *ForeignStore<T>::insert(const T &item)
    {
        std::pair<typename Dynamic::iterator, bool> result =
            mDynamic.insert(std::pair<ESM4::FormId, T>(item.mFormId, item));

        T *ptr = &result.first->second;
        if (result.second) {
            mShared.push_back(ptr);
        } else {
            *ptr = item;
        }

        return ptr;
    }

    template<typename T>
    T *ForeignStore<T>::insertStatic(const T &item)
    {
        std::pair<typename Static::iterator, bool> result =
            mStatic.insert(std::pair<ESM4::FormId, T>(item.mFormId, item));

        T *ptr = &result.first->second;
        if (result.second) {
            mShared.push_back(ptr);
        } else {
            *ptr = item;
        }

        return ptr;
    }

    template<typename T>
    bool ForeignStore<T>::eraseStatic(const std::string &id)
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif

        typename std::map<ESM4::FormId, T>::iterator it = mStatic.find(formId);

        if (it != mStatic.end()) {
            // delete from the static part of mShared
            typename std::vector<T *>::iterator sharedIter = mShared.begin();
            typename std::vector<T *>::iterator end = sharedIter + mStatic.size();

            while (sharedIter != mShared.end() && sharedIter != end) {
                if((*sharedIter)->mFormId == formId) {
                    mShared.erase(sharedIter);
                    break;
                }
                ++sharedIter;
            }
            mStatic.erase(it);
        }

        return true;
    }

    template<typename T>
    bool ForeignStore<T>::erase(const std::string &id) // FIXME: is this ever used?
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        return erase(formId);
    }

    template<typename T>
    bool ForeignStore<T>::erase(ESM4::FormId formId)
    {
        typename Dynamic::iterator it = mDynamic.find(formId);
        if (it == mDynamic.end()) {
            return false;
        }
        mDynamic.erase(it);

        // have to reinit the whole shared part
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        for (it = mDynamic.begin(); it != mDynamic.end(); ++it) {
            mShared.push_back(&it->second);
        }
        return true;
    }

    template<typename T>
    bool ForeignStore<T>::erase(const T &item)
    {
        return erase(item.mFormId);
    }

    template<typename T>
    void ForeignStore<T>::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (typename Dynamic::const_iterator iter (mDynamic.begin()); iter!=mDynamic.end();
             ++iter)
        {
            // FIXME
#if 0
            writer.startRecord (T::sRecordId);
            iter->second.save (writer);
            writer.endRecord (T::sRecordId);
#endif
        }
    }

    template<typename T>
    RecordId ForeignStore<T>::read(ESM::ESMReader& esm)
    {
        T record;

        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        record.load(reader);

        bool isDeleted = (record.mFlags & ESM4::Rec_Deleted) != 0;

        insert(record);

        std::string id = ESM4::formIdToString(record.mFormId);
        return RecordId(id, isDeleted);
    }

    ForeignStore<MWWorld::ForeignWorld>::~ForeignStore()
    {
        std::map<ESM4::FormId, MWWorld::ForeignWorld*>::iterator it = mWorlds.begin();
        for (; it != mWorlds.end(); ++it)
            delete it->second;
    }

    size_t ForeignStore<MWWorld::ForeignWorld>::getSize() const
    {
        return mWorlds.size();
    }

    ForeignWorld *ForeignStore<ForeignWorld>::getWorld(ESM4::FormId worldId)
    {
        std::map<ESM4::FormId, ForeignWorld*>::iterator it = mWorlds.find(worldId);
        if (it != mWorlds.end())
            return it->second;

        return nullptr;
    }

    const ForeignWorld *ForeignStore<ForeignWorld>::find(ESM4::FormId worldId) const
    {
        std::map<ESM4::FormId, ForeignWorld*>::const_iterator it = mWorlds.find(worldId);
        if (it != mWorlds.end())
            return it->second;

        return nullptr;
    }

    // editorId parameter is assumed to be in lower case
    // FIXME: should maintain a map for a faster lookup?
    const ForeignWorld *ForeignStore<ForeignWorld>::find(const std::string& editorId) const
    {
        std::map<ESM4::FormId, ForeignWorld*>::const_iterator it = mWorlds.begin();
        for (;it != mWorlds.end(); ++it)
        {
            std::string lowerEditorId = Misc::StringUtils::lowerCase(it->second->mEditorId);
            if (lowerEditorId == editorId)
                return it->second;
        }

        return nullptr;
    }

    // FIXME: should maintain a map for a faster lookup?
    ESM4::FormId ForeignStore<ForeignWorld>::getFormId(const std::string& editorId) const
    {
        std::string id = Misc::StringUtils::lowerCase(editorId);

        std::map<ESM4::FormId, ForeignWorld*>::const_iterator it = mWorlds.begin();
        for (;it != mWorlds.end(); ++it)
        {
            std::string lowerEditorId = Misc::StringUtils::lowerCase(it->second->mEditorId);
            if (lowerEditorId == id)
                return it->first;
        }

        return 0;
    }

    // Need to load ForeignWorld as well as update ESM4::WorldGroup (or alternatively
    // incorporate some aspects of WorldGroup into ForeignWorld which is the current
    // implementation).
    //
    // Similarly, ForeignCell and ForeignRoad need to update ESM4::WorldGroup (or
    // ForiegnWorld).
    //
    // FIXME: try move semantics similar to OpenCS
    RecordId ForeignStore<MWWorld::ForeignWorld>::load(ESM::ESMReader &esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        reader.getRecordData();

        MWWorld::ForeignWorld *record = new MWWorld::ForeignWorld();
        bool isDeleted = false;

        record->load(esm, isDeleted);
#if 0
        std::pair<std::map<ESM4::FormId, MWWorld::ForeignWorld*>::iterator, bool> ret
            = mWorlds.insert(std::make_pair(record->mFormId, record));
        // Try to overwrite existing record
        // FIXME: should this be merged instead?
        if (!ret.second)
            ret.first->second = record;
#else
        std::map<ESM4::FormId, MWWorld::ForeignWorld*>::iterator lb = mWorlds.lower_bound(record->mFormId);
        if (lb != mWorlds.end() && !(mWorlds.key_comp()(record->mFormId, lb->first)))
        {
            // FIXME: how to merge? are there anything to merge or update?
        }
        else
            mWorlds.insert(lb, std::make_pair(record->mFormId, record));
#endif
        //std::cout << "World: " << ESM4::formIdToString(record->mFormId) << std::endl; // FIXME: debug

        return RecordId(ESM4::formIdToString(record->mFormId), ((record->mFlags & ESM4::Rec_Deleted) != 0));
    }

    ForeignStore<MWWorld::ForeignCell>::~ForeignStore()
    {
        std::map<ESM4::FormId, MWWorld::ForeignCell*>::iterator it = mCells.begin();
        for (; it != mCells.end(); ++it)
            delete it->second;
    }

    size_t ForeignStore<MWWorld::ForeignCell>::getSize() const
    {
        return mCells.size();
    }

    // In order to save the file contexts from each of the mods for a given cell, either the cell
    // object needs to be constructed or special maps of the context lists need to be
    // maintained.  Since we need to partially load the cell anyway (to get the EditorId and
    // grid) we go ahead and create the cell objects depite the memory usage.
    // (TODO: check actual usage)
    //
    // Unfortunately it is not possible to use the group header labels to get the cell's formId
    // or grid. The labels may not be reliable even if they could be used.  (see
    // http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format)
    //
    // All the cells (even external cells from any of the worldspaces) are indexed by the
    // cell's formId.  We maintain another map so that a cell can be retrieved by its editorId
    // without having to loop through all the cells.
    //
    // A map of cell grid to formId is maintained in the world objects (i.e. there can be
    // multiple cells with the same grid)
    //
    // FIXME: some old comments below - check if still correct
    // Need to load ForeignCell as well as update ESM4::WorldGroup and ESM4::CellGroup
    // (or alternatively incorporate some aspects of WorldGroup and CellGroup into ForeignWorld
    // and ForeignCell).
    //
    // Similarly, loading ForeignLand, foreignPathgrid and various references need to update
    // ESM4::CellGroup (or ForeignCell).
    //
    void ForeignStore<MWWorld::ForeignCell>::preload(ESM::ESMReader& esm, ForeignStore<ForeignWorld>& worlds)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        // save the context first
        ESM4::ReaderContext ctx = static_cast<ESM::ESM4Reader*>(&esm)->getESM4Context();
        // Need to save these because cell->preload() may read into cell child group (where the ref's are)
        const ESM4::GroupTypeHeader& grp = reader.grp();
        int32_t groupType = grp.type;
#ifdef TEST_GROUP_HEADER_LABELS
        ESM4::GroupLabel groupLabel = grp.label;
#endif

        reader.getRecordData();

        // check for deleted record? does it matter if it is the base or a mod?
        //std::uint32_t flags  = reader.hdr().record.flags; // currently unused
        if ((reader.hdr().record.flags & ESM4::Rec_Deleted) != 0)
        {
            std::cout << "some CELL deleted message" << std::endl; // FIXME
            return;
        }

        // FIXME: need to merge CELL records ??
        // Pathgrid.esp
        // cell->mCell->mFormId 0x0001BD1F ICMarketDistrict01 PGRD FormId 0x0001C2C7
        // cell->mCell->mFormId 0x0001BD21 ICMarketDistrict04 PGRD FormId 0x0001C2C8
        // cell->mCell->mFormId 0x00165F2C                    PGRD FormId 0x00175BCA
        // cell->mCell->mFormId 0x0001BD1E ICMarketDistrict02 PGRD FormId 0x0001C2C9
        // cell->mCell->mFormId 0x0001BD20 ICMarketDistrict03 PGRD FormId 0x0001C2CA

        MWWorld::ForeignCell *cell;

        ESM4::FormId formId = reader.hdr().record.id;
        reader.adjustFormId(formId); // ensure mod index is correct TODO: test

        std::map<ESM4::FormId, MWWorld::ForeignCell*>::iterator lb = mCells.lower_bound(formId);
        if (lb != mCells.end() && !(mCells.key_comp()(formId, lb->first)))
        {
            cell = lb->second;

            // load cell record again, overwriting the previous
            // TODO: assumed that none of the previous sub-records are kept
            cell->preload(reader);

            // update the context with adjusted formid - since we won't be re-loading the CELL
            // record after context restoration, ctx.currCell won't be correct
            ctx.currCell = cell->mCell->mFormId;

            //std::cout << "CELL replaced " << ESM4::formIdToString(formId) << std::endl; // FIXME: for testing

            if (cell->mHasChildren)
                cell->addFileContext(ctx);
        }
        else // cell none found
        {
            cell = new MWWorld::ForeignCell(); // deleted in dtor

            cell->preload(reader); // load cell record but no references

            ctx.currCell = cell->mCell->mFormId; // update the context with adjusted formid

            if (cell->mHasChildren)
                cell->addFileContext(ctx);

            mCells.insert(lb, std::make_pair(formId, cell));
        }

        // FIXME: cleanup the mess of logic below, may need to refactor using a function or two

        // verify group label and update maps
        if (groupType == ESM4::Grp_ExteriorSubCell) // exterior cell, has grid
        {
            ESM4::FormId worldId = reader.currWorld();
            ForeignWorld *world = worlds.getWorld(worldId);
            if (!world)
            {
                std::ostringstream msg;
                msg << "Cell's parent world formId " << ESM4::formIdToString(worldId) << " not found";
                throw std::runtime_error(msg.str());
            }

#ifdef TEST_GROUP_HEADER_LABELS
            // group grid   cell grid range
            //
            //        -7       -56 :-49
            //        -6       -48 :-41
            //        -5       -40 :-33
            //        -4       -32 :-25
            //        -3       -24 :-17
            //        -2       -16 : -9
            //        -1        -8 : -1
            //         0         0 :  7
            //         1         8 : 15
            //         2        16 : 23
            //         3        24 : 31
            //         4        32 : 39
            //         5        40 : 47
            //         6        48 : 55
            //         7        56 : 63
            //         8        64 : 71
            //
            // group label is sub block grid
            if ((int)((cell->mCell->mX < 0) ? (cell->mCell->mX-7)/8 : cell->mCell->mX/8) != groupLabel.grid[1] ||
                (int)((cell->mCell->mY < 0) ? (cell->mCell->mY-7)/8 : cell->mCell->mY/8) != groupLabel.grid[0])
                std::cout << "Cell grid mismatch, x " << std::dec << cell->mCell->mX << ", y " << cell->mCell->mY
                          << " label x " << groupLabel.grid[1] << ", y " << groupLabel.grid[0]
                          << std::endl; // FIXME: debug only
#endif

            // ignore if one already exists (note above we re-loaded the record when the same formid was found)
            world->updateCellGridMap(cell->mCell->mX, cell->mCell->mY, cell->mCell->mFormId);
            //world->updateCellGridMap(cell);
        }
        else if (groupType == ESM4::Grp_WorldChild) // exterior dummy cell
        {
            ESM4::FormId worldId = reader.currWorld();
            ForeignWorld *world = worlds.getWorld(worldId);
            if (!world)
            {
                std::ostringstream msg;
                msg << "Cell's parent world formId " << ESM4::formIdToString(worldId) << " not found";
                throw std::runtime_error(msg.str());
            }

#ifdef TEST_GROUP_HEADER_LABELS
            // group label is parent world's formId
            if (worldId != groupLabel.value)
                std::cout << "Cell parent formid mismatch, " << std::hex << worldId
                          << " label " << groupLabel.value << std::endl;
#endif

            // ignore if one already exists (note above we re-loaded the record when the same formid was found)
            //world->setDummyCellId(cell->mCell->mFormId);
            world->setDummyCell(cell);
        }
        else if (groupType == ESM4::Grp_InteriorSubCell) // interior cell
        {
            // group label is sub block number (not sure of its purpose?)
#ifdef TEST_GROUP_HEADER_LABELS
            std::string padding = "";
            padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
            std::cout << padding << "CELL preload: Grp_InteriorSubCell, formId "
                      << ESM4::formIdToString(cell->mCell->mFormId) << std::endl;
#endif
        }
        else
            throw std::runtime_error("Cell record found in an unexpected group");

        if (!cell->mCell->mEditorId.empty())
        {
            std::pair<std::map<std::string, ESM4::FormId>::iterator, bool> res
                = mEditorIdMap.insert(std::make_pair(Misc::StringUtils::lowerCase(cell->mCell->mEditorId),
                                      cell->mCell->mFormId));

            // ignore if one already exists, should have the same formid
            if (!res.second) // checking just in case
            {
                const ForeignCell *oldCell = find(Misc::StringUtils::lowerCase(cell->mCell->mEditorId));
                if (oldCell->mCell->mFormId != cell->mCell->mFormId) // FIXME
                    std::cout << "Cell with different formid has the same editor id" << std::endl; // AylidWorld (Knights.esp)
                    //throw std::runtime_error("Cell with different formid has the same editor id");
            }
        }

        mLastPreloadedCell = cell->mCell->mFormId; // FIXME for testing only, delete later
    }

    void ForeignStore<MWWorld::ForeignCell>::testPreload(ESM::ESMReader& esm)
    {
        //FIXME below is for testing only
        std::map<ESM4::FormId, MWWorld::ForeignCell*>::iterator it = mCells.find(mLastPreloadedCell);
        if (it != mCells.end())
            it->second->testPreload(esm);
        else
            std::cout << "CELL preload: cell not found" << std::endl;
    }

    void ForeignStore<MWWorld::ForeignCell>::loadVisibleDist(ESMStore& store, ESM::ESMReader& esm, CellStore *cell)
    {
        while(esm.hasMoreRecs())
        {
            ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

            reader.checkGroupStatus();
            if (reader.getContext().groupStack.back().first.type != ESM4::Grp_CellVisibleDistChild)
                return; // must have popped

            loadTes4Group(store, esm, cell);
            //listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
        }
    }

    // FIXME: this is the 3rd time the same code (almost) is being repeated, not even counting
    // OpenCS. Here, ESMStore and CellStore.  Need to move them out to something like ESMLoader.
    void ForeignStore<MWWorld::ForeignCell>::loadTes4Group(ESMStore& store, ESM::ESMReader& esm, CellStore *cell)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        reader.getRecordHeader();
        const ESM4::RecordHeader& hdr = reader.hdr();

        if (hdr.record.typeId != ESM4::REC_GRUP)
            return cell->loadTes4Record(store, reader);

        // should not happen, throw?
        std::cout << "ForeignStore<ForeignCell>::loadTes4Group unexpected group" << std::endl;
    }

    void ForeignStore<MWWorld::ForeignCell>::updateRefrEstimate(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        const ESM4::GroupTypeHeader grp = reader.grp();

        const ESM4::FormId currCell = reader.currCell();
        //std::uint64_t modId(reader.getContext().modIndex);
        //modId <<= 8;
        //modId |= currCell;
        std::map<ESM4::FormId, MWWorld::ForeignCell*>::iterator it = mCells.find(currCell);
        if (it == mCells.end())
            return;

        MWWorld::ForeignCell *cell = it->second;

        static int magic = 100; // FIXME: just a guess
        std::uint32_t estimate = grp.groupSize / magic;
        cell->setRefrEstimate(grp.type, estimate);
    }

    // FIXME: this is rather inefficient
    // - probably worth caching the current ForegnCell in ESMStore instead
    void ForeignStore<MWWorld::ForeignCell>::incrementRefrCount(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        const ESM4::FormId currCell = reader.currCell();
        //std::uint64_t modId(reader.getContext().modIndex);
        //modId <<= 8;
        //modId |= currCell;
        std::map<ESM4::FormId, MWWorld::ForeignCell*>::iterator it = mCells.find(currCell);
        if (it == mCells.end())
            return;

        MWWorld::ForeignCell* cell = it->second;

        cell->incrementRefrCount(reader.grp().type);
    }

    // FIXME: Is there a more efficient way than calling ForeignStore<ForeignWorld>::find() each time?
    RecordId ForeignStore<MWWorld::ForeignCell>::load(ESM::ESMReader& esm, ForeignStore<ForeignWorld>& worlds)
    {
        //ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader(); // currently unused

// FIXME: below was before preload was implemented
#if 0
        // FIXME: testing context save/restore only
        //ESM4::ReaderContext ctx = static_cast<ESM::ESM4Reader*>(&esm)->getESM4Context();
        //static_cast<ESM::ESM4Reader*>(&esm)->restoreESM4Context(ctx);

        reader.getRecordData();

        MWWorld::ForeignCell *record = new MWWorld::ForeignCell();
        bool isDeleted = false;

        record->load(esm, isDeleted);

        std::pair<std::map<ESM4::FormId, MWWorld::ForeignCell*>::iterator, bool> ret
            = mCells.insert(std::make_pair(record->mFormId, record));
        // Try to overwrite existing record
        // FIXME: should this be merged instead?
        if (!ret.second)
            ret.first->second = record;

        //std::cout << "Cell: " << ESM4::formIdToString(record->mFormId) << std::endl; // FIXME: debug

        if (record->mHasGrid)
        {
            ForeignWorld *world = worlds.find(record->mParent);

            if (world)
                world->updateCellGridMap(record->mX, record->mY, record->mFormId);

// FIXME: debug
            std::cout << "Exterior Cell, world: " << ESM4::formIdToString(record->mParent)
                      << ", x: " << record->mX << ", y: " << record->mY << std::endl;
        }
// FIXME: debug
        if (record->mIsInterior)
            std::cout << "Interior Cell, parent world: " << ESM4::formIdToString(record->mParent)
                      << ", editor id: " << record->mEditorId << std::endl;

        return RecordId(ESM4::formIdToString(record->mFormId), ((record->mFlags & ESM4::Rec_Deleted) != 0));
#endif
        return RecordId("", false);
    }

    // FIXME: how is this different to find(const std::string& name) ?
    const ForeignCell *ForeignStore<MWWorld::ForeignCell>::searchExtByName(const std::string &name) const
    {
        ForeignCell *cell = nullptr;

        std::map<ESM4::FormId, MWWorld::ForeignCell*>::const_iterator it = mCells.begin();
        for (;it != mCells.end(); ++it)
        {
            if (!it->second->mIsInterior && Misc::StringUtils::ciEqual(it->second->mCell->mEditorId, name))
            {
                // if there are more than one external cell with the same editor id, get the
                // cell that is the upper most and furtherst to the right
                if ( cell == 0 ||                                                 // the first match
                    ( it->second->mCell->mX > cell->mCell->mX ) ||                              // keep going right
                    ( it->second->mCell->mX == cell->mCell->mX && it->second->mCell->mY > cell->mCell->mY ) ) // keep going up
                {
                    cell = it->second;
                }
            }
        }

        // FIXME: debug only
        if (cell)
            std::cout << "cell: " << name << ", x: " << std::dec << cell->mCell->mX << ", y " << cell->mCell->mY << std::endl;

        return cell;
    }

    const ESM::Cell *ForeignStore<MWWorld::ForeignCell>::find(int x, int y) const
    {
        // FIXME: some dummy for now
        const ESM::Cell *ptr = nullptr;
        return ptr;
    }

    const MWWorld::ForeignCell *ForeignStore<MWWorld::ForeignCell>::find(const std::string& name) const
    {
        std::map<std::string, ESM4::FormId>::const_iterator it = mEditorIdMap.find(name);

        if (it != mEditorIdMap.end())
            return find(it->second);

        return nullptr;
    }

    // WARNING: the supplied formid needs to have the correct modindex
    const MWWorld::ForeignCell *ForeignStore<MWWorld::ForeignCell>::find(ESM4::FormId formId) const
    {
        std::map<ESM4::FormId, MWWorld::ForeignCell*>::const_iterator it = mCells.find(formId);
        if (it != mCells.end())
            return it->second;

        return nullptr;
    }

    ForeignStore<MWWorld::ForeignLand>::~ForeignStore()
    {
        std::map<ESM4::FormId, MWWorld::ForeignLand*>::iterator it = mLands.begin();
        for (; it != mLands.end(); ++it)
            delete it->second;
    }

    size_t ForeignStore<MWWorld::ForeignLand>::getSize() const
    {
        return mLands.size();
    }

    RecordId ForeignStore<MWWorld::ForeignLand>::load(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        ForeignId result = loadForeign(reader);

        std::string id = ESM4::formIdToString(result.mId);
        return RecordId(id, result.mIsDeleted); // NOTE: id is uppercase (not that it matters)
    }

    ForeignId ForeignStore<ForeignLand>::loadForeign(ESM4::Reader& reader)
    {
        MWWorld::ForeignLand *record = new ForeignLand();
        bool isDeleted = false; // not used

        record->load(reader, isDeleted);

        std::pair<std::map<ESM4::FormId, ForeignLand*>::iterator, bool> ret
            = mLands.insert(std::make_pair(record->mFormId, record));
        // Try to overwrite existing record
        // FIXME: should this be merged instead?
        if (!ret.second)
            ret.first->second = record;

        //std::cout << "Land: " << ESM4::formIdToString(record->mFormId) << std::endl; // FIXME: debug

        return ForeignId(record->mFormId, ((record->mFlags & ESM4::Rec_Deleted) != 0));
    }

    const ForeignLand *ForeignStore<ForeignLand>::find(ESM4::FormId formId) const
    {
        std::map<ESM4::FormId, ForeignLand*>::const_iterator it = mLands.find(formId);
        if (it != mLands.end())
            return it->second;

        return nullptr;
    }

    ForeignLand *ForeignStore<ForeignLand>::search(ESM4::FormId worldId, int x, int y) const
    {
        return nullptr; // FIXME
    }

    ForeignLand *ForeignStore<ForeignLand>::find(ESM4::FormId worldId, int x, int y) const
    {
        return nullptr; // FIXME
    }

    ForeignStore<ForeignDialogue>::~ForeignStore()
    {
        std::vector<ForeignDialogue*>::iterator it = mDialogues.begin();
        for (; it != mDialogues.end(); ++it)
            delete *it;
    }

    size_t ForeignStore<ForeignDialogue>::getSize() const
    {
        return mDialogues.size();
    }

    RecordId ForeignStore<ForeignDialogue>::load(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        ForeignId result = loadForeign(reader);

        std::string id = ESM4::formIdToString(result.mId);
        return RecordId(id, result.mIsDeleted); // NOTE: id is uppercase (not that it matters)
    }

    ForeignId ForeignStore<ForeignDialogue>::loadForeign(ESM4::Reader& reader)
    {
        // FIXME: unique_ptr?
        ForeignDialogue *record = new ForeignDialogue();
        record->load(reader);

        bool isDeleted = (record->mFlags & ESM4::Rec_Deleted) != 0;
#if 0
        std::pair<std::map<ESM4::FormId, ForeignDialogue*>::iterator, bool> ret
            = mDialogues.insert(std::make_pair(record->mFormId, record));

        // Trying to overwrite existing record
        // FIXME: should this be merged instead?
        if (!ret.second)
            ret.first->second = record;
#else
        std::size_t index = mDialogues.size();
        mDialogues.push_back(record);
        mFormIdMap[record->mFormId] = index;
        mTopicMap[record->mEditorId] = index;
#endif
        //std::cout << "Dialogue: " << ESM4::formIdToString(record->mFormId) << std::endl; // FIXME: debug

        return ForeignId(record->mFormId, isDeleted);
    }

    const ForeignDialogue *ForeignStore<ForeignDialogue>::find(ESM4::FormId formId) const
    {
#if 0
        std::map<ESM4::FormId, ForeignDialogue*>::const_iterator it = mDialogues.find(formId);
        if (it != mDialogues.end())
            return it->second;
#else
        std::map<ESM4::FormId, std::size_t>::const_iterator it = mFormIdMap.find(formId);
        if (it != mFormIdMap.end())
            return mDialogues[it->second];
#endif
        std::ostringstream msg;
        // FIXME: getRecordType() not implemented
        msg << /*T::getRecordType() <<*/ " '" << ESM4::formIdToString(formId) << "' not found";
        throw std::runtime_error(msg.str());
    }

    const ForeignDialogue *ForeignStore<ForeignDialogue>::search(ESM4::FormId formId) const
    {
#if 0
        std::map<ESM4::FormId, ForeignDialogue*>::const_iterator it = mDialogues.find(formId);
        if (it != mDialogues.end())
            return it->second;
#else
        std::map<ESM4::FormId, std::size_t>::const_iterator it = mFormIdMap.find(formId);
        if (it != mFormIdMap.end())
            return mDialogues[it->second];
#endif
        return nullptr;
    }

    const ForeignDialogue *ForeignStore<ForeignDialogue>::search(const std::string& topic) const
    {
        std::map<std::string, std::size_t>::const_iterator it = mTopicMap.find(topic);
        if (it != mTopicMap.end())
            return mDialogues[it->second];

        return nullptr;
    }

    ForeignStore<ESM4::Quest>::~ForeignStore()
    {
        std::vector<ESM4::Quest*>::iterator it = mQuests.begin();
        for (; it != mQuests.end(); ++it)
            delete *it;
    }

    size_t ForeignStore<ESM4::Quest>::getSize() const
    {
        return mQuests.size();
    }

    RecordId ForeignStore<ESM4::Quest>::load(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        ForeignId result = loadForeign(reader);

        std::string id = ESM4::formIdToString(result.mId);
        return RecordId(id, result.mIsDeleted); // NOTE: id is uppercase (not that it matters)
    }

    ForeignId ForeignStore<ESM4::Quest>::loadForeign(ESM4::Reader& reader)
    {
        // FIXME: unique_ptr?
        ESM4::Quest *record = new ESM4::Quest();
        record->load(reader);

        bool isDeleted = (record->mFlags & ESM4::Rec_Deleted) != 0;
        std::size_t index = mQuests.size();
        mQuests.push_back(record);

        mFormIdMap[record->mFormId] = index;
        mTopicMap[record->mEditorId] = index;

        for (std::size_t i = 0; i < record->mTargetConditions.size(); ++i)
        {
            // FIXME: there are *lots* of function types other than GetIsID
            if (record->mTargetConditions[i].functionIndex == ESM4::FUN_GetIsID)
            {
                mConditionMap[record->mTargetConditions[i].param1] = index;
            }
        }

        return ForeignId(record->mFormId, isDeleted);
    }

    std::vector<ESM4::Quest*>::const_iterator ForeignStore<ESM4::Quest>::begin() const
    {
        return mQuests.begin();
    }

    std::vector<ESM4::Quest*>::const_iterator ForeignStore<ESM4::Quest>::end() const
    {
        return mQuests.end();
    }

    const ESM4::Quest *ForeignStore<ESM4::Quest>::find(ESM4::FormId formId) const
    {
        const ESM4::Quest *quest = search(formId);
        if (quest)
            return quest;

        std::ostringstream msg;
        // FIXME: getRecordType() not implemented
        msg << /*T::getRecordType() <<*/ " '" << ESM4::formIdToString(formId) << "' not found";
        throw std::runtime_error(msg.str());
    }

    const ESM4::Quest *ForeignStore<ESM4::Quest>::search(ESM4::FormId formId) const
    {
        std::map<ESM4::FormId, std::size_t>::const_iterator it = mFormIdMap.find(formId);
        if (it != mFormIdMap.end())
            return mQuests[it->second];

        return nullptr;
    }

    const ESM4::Quest *ForeignStore<ESM4::Quest>::searchCondition(ESM4::FormId formId) const
    {
        std::map<ESM4::FormId, std::size_t>::const_iterator it = mConditionMap.find(formId);
        if (it != mConditionMap.end())
            return mQuests[it->second];

        return nullptr;
    }

    const ESM4::Quest *ForeignStore<ESM4::Quest>::search(const std::string& topic) const
    {
        std::map<std::string, std::size_t>::const_iterator it = mTopicMap.find(topic);
        if (it != mTopicMap.end())
            return mQuests[it->second];

        return nullptr;
    }
}

//template class MWWorld::ForeignStore<MWWorld::ForeignWorld>;
//template class MWWorld::ForeignStore<MWWorld::ForeignCell>;
//template class MWWorld::ForeignStore<MWWorld::ForeignLand>;
//
template class MWWorld::ForeignStore<ESM4::GlobalVariable>;
template class MWWorld::ForeignStore<ESM4::Hair>;
template class MWWorld::ForeignStore<ESM4::Eyes>;
template class MWWorld::ForeignStore<ESM4::Race>;
template class MWWorld::ForeignStore<ESM4::ActorCharacter>;
template class MWWorld::ForeignStore<ESM4::ActorCreature>;
template class MWWorld::ForeignStore<ESM4::LandTexture>;
template class MWWorld::ForeignStore<ESM4::Script>;
template class MWWorld::ForeignStore<ESM4::Dialogue>;
template class MWWorld::ForeignStore<ESM4::DialogInfo>;
template class MWWorld::ForeignStore<ESM4::Quest>;
template class MWWorld::ForeignStore<ESM4::AIPackage>;
template class MWWorld::ForeignStore<ESM4::Pathgrid>;
template class MWWorld::ForeignStore<ESM4::BodyPartData>;
template class MWWorld::ForeignStore<ESM4::HeadPart>;
template class MWWorld::ForeignStore<ESM4::LightingTemplate>;
template class MWWorld::ForeignStore<ESM4::Music>;
template class MWWorld::ForeignStore<ESM4::MediaLocationController>;
template class MWWorld::ForeignStore<ESM4::MediaSet>;
template class MWWorld::ForeignStore<ESM4::DefaultObj>;
template class MWWorld::ForeignStore<ESM4::Region>;
template class MWWorld::ForeignStore<ESM4::PlacedGrenade>;
template class MWWorld::ForeignStore<ESM4::SoundReference>;
// Referenceables
template class MWWorld::ForeignStore<ESM4::Sound>;
template class MWWorld::ForeignStore<ESM4::Activator>;
template class MWWorld::ForeignStore<ESM4::Apparatus>;
template class MWWorld::ForeignStore<ESM4::Armor>;
template class MWWorld::ForeignStore<ESM4::Book>;
template class MWWorld::ForeignStore<ESM4::Clothing>;
template class MWWorld::ForeignStore<ESM4::Container>;
template class MWWorld::ForeignStore<ESM4::Door>;
template class MWWorld::ForeignStore<ESM4::Ingredient>;
template class MWWorld::ForeignStore<ESM4::Light>;
template class MWWorld::ForeignStore<ESM4::MiscItem>;
template class MWWorld::ForeignStore<ESM4::Static>;
template class MWWorld::ForeignStore<ESM4::Grass>;
template class MWWorld::ForeignStore<ESM4::Tree>;
template class MWWorld::ForeignStore<ESM4::Flora>;
template class MWWorld::ForeignStore<ESM4::Furniture>;
template class MWWorld::ForeignStore<ESM4::Weapon>;
template class MWWorld::ForeignStore<ESM4::Ammunition>;
template class MWWorld::ForeignStore<ESM4::Npc>;
template class MWWorld::ForeignStore<ESM4::Creature>;
template class MWWorld::ForeignStore<ESM4::LevelledCreature>;
template class MWWorld::ForeignStore<ESM4::SoulGem>;
template class MWWorld::ForeignStore<ESM4::Key>;
template class MWWorld::ForeignStore<ESM4::Potion>;
template class MWWorld::ForeignStore<ESM4::SubSpace>;
template class MWWorld::ForeignStore<ESM4::SigilStone>;
template class MWWorld::ForeignStore<ESM4::LevelledItem>;
template class MWWorld::ForeignStore<ESM4::LevelledNpc>;
template class MWWorld::ForeignStore<ESM4::IdleMarker>;
template class MWWorld::ForeignStore<ESM4::MovableStatic>;
template class MWWorld::ForeignStore<ESM4::TextureSet>;
template class MWWorld::ForeignStore<ESM4::Scroll>;
template class MWWorld::ForeignStore<ESM4::ArmorAddon>;
template class MWWorld::ForeignStore<ESM4::Terminal>;
template class MWWorld::ForeignStore<ESM4::TalkingActivator>;
template class MWWorld::ForeignStore<ESM4::Note>;
template class MWWorld::ForeignStore<ESM4::AcousticSpace>;
template class MWWorld::ForeignStore<ESM4::ItemMod>;
template class MWWorld::ForeignStore<ESM4::PlaceableWater>;
template class MWWorld::ForeignStore<ESM4::StaticCollection>;
template class MWWorld::ForeignStore<ESM4::AnimObject>;
template class MWWorld::ForeignStore<ESM4::Outfit>;
