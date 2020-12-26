#include "cellstore.hpp"

#include <iostream>
#include <algorithm>

#include <extern/esm4/cell.hpp>
#include <extern/esm4/refr.hpp>
#include <extern/esm4/achr.hpp>
#include <extern/esm4/acre.hpp>
#include <extern/esm4/pgrd.hpp>

#include <components/esm/cellstate.hpp>
#include <components/esm/cellid.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/objectstate.hpp>
#include <components/esm/containerstate.hpp>
#include <components/esm/npcstate.hpp>
#include <components/esm/creaturestate.hpp>
#include <components/esm/fogstate.hpp>
#include <components/esm/creaturelevliststate.hpp>
#include <components/esm/doorstate.hpp>
#include <components/esm/esm4reader.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "ptr.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "containerstore.hpp"

namespace
{
    template<typename T>
    MWWorld::Ptr searchInContainerList (MWWorld::CellRefList<T>& containerList, const std::string& id)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (containerList.mList.begin());
             iter!=containerList.mList.end(); ++iter)
        {
            MWWorld::Ptr container (&*iter, 0);

            MWWorld::Ptr ptr =
                container.getClass().getContainerStore (container).search (id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return MWWorld::Ptr();
    }

    template<typename T>
    MWWorld::Ptr searchViaActorId (MWWorld::CellRefList<T>& actorList, int actorId,
        MWWorld::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (actorList.mList.begin());
             iter!=actorList.mList.end(); ++iter)
        {
            MWWorld::Ptr actor (&*iter, cell);

            if (actor.getClass().getCreatureStats (actor).matchesActorId (actorId) && actor.getRefData().getCount() > 0)
                return actor;
        }

        return MWWorld::Ptr();
    }
#if 0
    // FIXME: can this be more efficient?
    template<typename T>
    MWWorld::Ptr searchViaActorId (MWWorld::CellRefList<T>& actorList, int actorId,
        MWWorld::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (actorList.mList.begin());
             iter!=actorList.mList.end(); ++iter)
        {
            MWWorld::Ptr actor (&*iter, cell);

            if (actor.getClass().getCreatureStats (actor).matchesActorId (actorId) && actor.getRefData().getCount() > 0)
                return actor;
        }

        return MWWorld::Ptr();
    }
#endif
    template<typename RecordType, typename T>
    void writeReferenceCollection (ESM::ESMWriter& writer,
        const MWWorld::CellRefList<T>& collection)
    {
        if (!collection.mList.empty())
        {
            // references
            for (typename MWWorld::CellRefList<T>::List::const_iterator
                iter (collection.mList.begin());
                iter!=collection.mList.end(); ++iter)
            {
                if (!iter->mData.hasChanged() && !iter->mRef.hasChanged() && iter->mRef.hasContentFile())
                {
                    // Reference that came from a content file and has not been changed -> ignore
                    continue;
                }
                if (iter->mData.getCount()==0 && !iter->mRef.hasContentFile())
                {
                    // Deleted reference that did not come from a content file -> ignore
                    continue;
                }

                RecordType state;
                iter->save (state);

                // recordId currently unused
                writer.writeHNT ("OBJE", collection.mList.front().mBase->sRecordId);

                state.save (writer);
            }
        }
    }

    template<typename RecordType, typename T>
    void readReferenceCollection (ESM::ESMReader& reader,
        MWWorld::CellRefList<T>& collection, const ESM::CellRef& cref, const std::map<int, int>& contentFileMap)
    {
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        RecordType state;
        state.mRef = cref;
        state.load(reader);

        // If the reference came from a content file, make sure this content file is loaded
        if (state.mRef.mRefNum.hasContentFile())
        {
            std::map<int, int>::const_iterator iter =
                contentFileMap.find (state.mRef.mRefNum.mContentFile);

            if (iter==contentFileMap.end())
                return; // content file has been removed -> skip

            state.mRef.mRefNum.mContentFile = iter->second;
        }

        if (!MWWorld::LiveCellRef<T>::checkState (state))
            return; // not valid anymore with current content files -> skip

        const T *record = esmStore.get<T>().search (state.mRef.mRefID);

        if (!record)
            return;

        if (state.mRef.mRefNum.hasContentFile())
        {
            for (typename MWWorld::CellRefList<T>::List::iterator iter (collection.mList.begin());
                iter!=collection.mList.end(); ++iter)
                if (iter->mRef.getRefNum()==state.mRef.mRefNum)
                {
                    // overwrite existing reference
                    iter->load (state);
                    return;
                }
        }

        // new reference
        MWWorld::LiveCellRef<T> ref (record);
        ref.load (state);
        collection.mList.push_back (ref);
    }
}

namespace MWWorld
{
    template <typename X>
    void CellRefList<X>::load(ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore)
    {
        const MWWorld::Store<X> &store = esmStore.get<X>();

        if (const X *ptr = store.search (ref.mRefID))
        {
            typename std::list<LiveRef>::iterator iter =
                std::find(mList.begin(), mList.end(), ref.mRefNum);

            LiveRef liveCellRef (ref, ptr);

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mList.end())
                *iter = liveCellRef;
            else
                mList.push_back (liveCellRef);
        }
        else
        {
            std::cerr
                << "Error: could not resolve cell reference " << ref.mRefID
                << " (dropping reference)" << std::endl;
        }
    }

    template <typename X>
    void CellRefList<X>::load(ESM4::Reference &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool mapGrid)
    {
        const MWWorld::ForeignStore<X> &store = esmStore.getForeign<X>();

        if (const X *base = store.search(ref.mBaseObj))
        {
            FormIdMap::const_iterator iter = mFormIdMap.find(ref.mFormId);

            LiveRef liveCellRef (ref, base); // ref is the reference, base is the base object

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mFormIdMap.end()) // exists, replace
            {
                // there are a couple of options here (choosing option 1 for now):
                // 1. actually replace, but to do that will need to search again
                // 2. mark the record as replaced by setting ref.mData.disable() and also
                //    setting the formid in ref.mRef to be 0
                std::cout << "REFR " << ESM4::formIdToString(ref.mFormId) << " replaced" << std::endl; // FIXME

                typename std::list<LiveRef>::iterator listIter =
                    std::find(mList.begin(), mList.end(), ref);

                if (listIter != mList.end())
                    *listIter = liveCellRef;
                else
                    throw std::runtime_error ("CellStore: could not find "+ESM4::formIdToString(ref.mFormId));

                // update index map with a new pointer
                mFormIdMap[ref.mFormId] = &*listIter;
            }
            else
            {
                mList.push_back(liveCellRef); // insert

                //std::size_t index = mList.size() -1;
                LiveCellRefBase *refPtr = &mList.back();
                mFormIdMap[ref.mFormId] = refPtr;//index;

                // TODO: skip GridMap for non-dummy cells? need a new variable passed in from CellStore
                if (mapGrid)
                {
                    const int cellSize = 4096; // FIXME: hard coded cell size

                    std::int32_t x = std::int32_t(ref.mPlacement.pos.x / cellSize);
                    std::int32_t y = std::int32_t(ref.mPlacement.pos.y / cellSize);
                    std::pair<std::int32_t, std::int32_t> grid(x, y);

                    GridMap::iterator lb = mGridMap.lower_bound(grid);

                    if (lb != mGridMap.end() && !(mGridMap.key_comp()(grid, lb->first)))
                        lb->second.push_back(ref.mFormId); // found, add
                    else // none found, insert new grid
                        mGridMap.insert(lb, std::make_pair(grid, std::vector<ESM4::FormId> { ref.mFormId }));
                }

                //std::cout << "CellRefList::load "
                    //<< ESM4::formIdToString(mList.back().mBase->mFormId) << std::endl; // FIXME: debug only

// FIXME: testing audio markers
#if 0
                if (ref.mAudioLocation)
                    std::cout << "audio loc " << ref.mEditorId << " "
                        << ref.mPosition.pos.x/4096 << "," << ref.mPosition.pos.y/4096 << " "
                    << mList.back().mBase->mEditorId << std::endl; // FIXME: debug only
#endif
            }
        }
        else
        {
            std::cerr
                << "Error: could not resolve foreign cell reference "
                << ((ref.mEditorId != "") ? ref.mEditorId : ESM4::formIdToString(ref.mFormId))
                << " (dropping reference)" << std::endl;
        }
    }

    template <typename X>
    void CellRefList<X>::load(ESM4::ActorCreature &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy)
    {
        const MWWorld::ForeignStore<X> &store = esmStore.getForeign<X>();

        if (const X *base = store.search(ref.mBaseObj))
        {
            FormIdMap::const_iterator iter = mFormIdMap.find(ref.mFormId);

            LiveRef liveCellRef (ref, base); // ref is the reference, base is the base object

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mFormIdMap.end()) // exists, replace
            {
                std::cout << "crea replace" << std::endl; // FIXME

                typename std::list<LiveRef>::iterator listIter =
                    std::find(mList.begin(), mList.end(), ref);

                if (listIter != mList.end())
                    *listIter = liveCellRef;
                else
                    throw std::runtime_error ("CellStore: could not find "+ESM4::formIdToString(ref.mFormId));

                // update index map with a new pointer
                mFormIdMap[ref.mFormId] = &*listIter;
            }
            else
            {
                mList.push_back(liveCellRef); // insert

                //std::size_t index = mList.size() -1;
                LiveCellRefBase *refPtr = &mList.back();
                mFormIdMap[ref.mFormId] = refPtr;//index;

                // TODO: skip GridMap for non-dummy cells? need a new variable passed in from CellStore
                if (dummy)
                {
                    const int cellSize = 4096; // FIXME: hard coded cell size

                    std::int32_t x = std::int32_t(ref.mPlacement.pos.x / cellSize);
                    std::int32_t y = std::int32_t(ref.mPlacement.pos.y / cellSize);
                    std::pair<std::int32_t, std::int32_t> grid(x, y);

                    GridMap::iterator lb = mGridMap.lower_bound(grid);

                    if (lb != mGridMap.end() && !(mGridMap.key_comp()(grid, lb->first)))
                        lb->second.push_back(ref.mFormId); // found, add
                    else // none found, insert new grid
                        mGridMap.insert(lb, std::make_pair(grid, std::vector<ESM4::FormId> { ref.mFormId }));
                }

                //std::cout << "CellRefList::load "
                    //<< ESM4::formIdToString(mList.back().mBase->mFormId) << std::endl; // FIXME: debug only
            }
        }
        else
        {
            std::cerr
                << "Error: could not resolve foreign cell reference "
                << ((ref.mEditorId != "") ? ref.mEditorId : ESM4::formIdToString(ref.mFormId))
                << " (dropping reference)" << std::endl;
        }
    }

    template <typename X>
    void CellRefList<X>::load(ESM4::ActorCharacter &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy)
    {
        const MWWorld::ForeignStore<X> &store = esmStore.getForeign<X>();

        if (const X *base = store.search(ref.mBaseObj))
        {
            FormIdMap::const_iterator iter = mFormIdMap.find(ref.mFormId);
#if 0
            for (size_t i = 0; i < ptr->mInventory.size(); ++i)
            {
                uint32_t type = esmStore.find(ptr->mInventory[i].item);
                if (type == ESM4::REC_BOOK)
                {
                    const ESM4::Book* note = esmStore.getForeign<ESM4::Book>().search(ptr->mInventory[i].item);
                    std::cout << ptr->mEditorId << " " << note->mEditorId << std::endl;
                }
                else
                    std::cout << ESM4::printName(type) << std::endl;
            }
#endif

            LiveRef liveCellRef (ref, base); // ref is the reference, base is the base object

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mFormIdMap.end()) // exists, replace
            {
                std::cout << "npc replace" << std::endl; // FIXME

                typename std::list<LiveRef>::iterator listIter =
                    std::find(mList.begin(), mList.end(), ref);

                if (listIter != mList.end())
                    *listIter = liveCellRef;
                else
                    throw std::runtime_error ("CellStore: could not find "+ESM4::formIdToString(ref.mFormId));

                // update index map with a new pointer
                mFormIdMap[ref.mFormId] = &*listIter;
            }
            else
            {
                mList.push_back(liveCellRef); // insert

                //std::size_t index = mList.size() -1;
                LiveCellRefBase *refPtr = &mList.back();
                mFormIdMap[ref.mFormId] = refPtr;//index;

                // TODO: skip GridMap for non-dummy cells? need a new variable passed in from CellStore
                if (dummy)
                {
                    const int cellSize = 4096; // FIXME: hard coded cell size

                    std::int32_t x = std::int32_t(ref.mPlacement.pos.x / cellSize);
                    std::int32_t y = std::int32_t(ref.mPlacement.pos.y / cellSize);
                    std::pair<std::int32_t, std::int32_t> grid(x, y);

                    GridMap::iterator lb = mGridMap.lower_bound(grid);

                    if (lb != mGridMap.end() && !(mGridMap.key_comp()(grid, lb->first)))
                        lb->second.push_back(ref.mFormId); // found, add
                    else // none found, insert new grid
                        mGridMap.insert(lb, std::make_pair(grid, std::vector<ESM4::FormId> { ref.mFormId }));
                }

                //std::cout << "CellRefList::load "
                    //<< ESM4::formIdToString(mList.back().mBase->mFormId) << std::endl; // FIXME: debug only
            }
        }
        else
        {
            std::cerr
                << "Error: could not resolve foreign cell reference "
                << ((ref.mEditorId != "") ? ref.mEditorId : ESM4::formIdToString(ref.mFormId))
                << " (dropping reference)" << std::endl;
        }
    }

    template<typename X> bool operator==(const LiveCellRef<X>& ref, int pRefnum)
    {
        return (ref.mRef.mRefnum == pRefnum);
    }

    CellStore::CellStore (const ESM::Cell *cell, bool isForeignCell, bool isDummyCell)
      : mCell (cell), mState (State_Unloaded), mHasState (false),
        mIsForeignCell(isForeignCell), mIsDummyCell(isDummyCell), mForeignLand(0),
        mLastRespawn(0,0)
    {
        if (!mIsForeignCell)
            mWaterLevel = cell->mWater;
        else if (mCell)
        {
            // use the one from world, since the cell water height is for rivers, ponds, etc
            //mWaterLevel = static_cast<const ForeignCell*>(mCell)->mWaterHeight; // Non-ocean water-height
            const ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ForeignWorld* world
                = store.getForeign<ForeignWorld>().find(static_cast<const ForeignCell*>(mCell)->mCell->mParent);
            if (world)
                mWaterLevel = world->mWaterLevel;
            else
                mWaterLevel = 0.f; // FIXME: throw?
        }
        else
            mWaterLevel = 0.f;

        mStores[ESM4::REC_SOUN] = &mSounds;
        mStores[ESM4::REC_ACTI] = &mForeignActivators;
        mStores[ESM4::REC_APPA] = &mForeignApparatus;
        mStores[ESM4::REC_ARMO] = &mForeignArmors;
        mStores[ESM4::REC_BOOK] = &mForeignBooks;
        mStores[ESM4::REC_CLOT] = &mForeignClothes;
        mStores[ESM4::REC_CONT] = &mForeignContainers;
        mStores[ESM4::REC_DOOR] = &mForeignDoors;
        mStores[ESM4::REC_INGR] = &mForeignIngredients;
        mStores[ESM4::REC_LIGH] = &mForeignLights;
        mStores[ESM4::REC_MISC] = &mForeignMiscItems;
        mStores[ESM4::REC_STAT] = &mForeignStatics;
        mStores[ESM4::REC_GRAS] = &mForeignGrasses;
        mStores[ESM4::REC_TREE] = &mForeignTrees;
        mStores[ESM4::REC_FLOR] = &mForeignFloras;
        mStores[ESM4::REC_FURN] = &mForeignFurnitures;
        mStores[ESM4::REC_WEAP] = &mForeignWeapons;
        mStores[ESM4::REC_AMMO] = &mAmmunitions;
        mStores[ESM4::REC_NPC_] = &mForeignNpcs;
        mStores[ESM4::REC_CREA] = &mForeignCreatures;
        mStores[ESM4::REC_LVLC] = &mLevelledCreatures;
        mStores[ESM4::REC_IDLM] = &mIdleMarkers;
        mStores[ESM4::REC_SLGM] = &mSoulGems;
        mStores[ESM4::REC_KEYM] = &mForeignKeys;
        mStores[ESM4::REC_ALCH] = &mForeignPotions;
        mStores[ESM4::REC_SBSP] = &mSubSpaces;
        mStores[ESM4::REC_SGST] = &mSigilStones;
        mStores[ESM4::REC_LVLI] = &mLevelledItems;
        mStores[ESM4::REC_LVLN] = &mLevelledNpcs;
        mStores[ESM4::REC_MSTT] = &mMovableStatics;
      //mStores[ESM4::REC_TXST] = &mTextureSets;
      //mStores[ESM4::REC_SCRL] = &mForeignScrolls;
      //mStores[ESM4::REC_ARMA] = &mArmorAddons;
        mStores[ESM4::REC_ASPC] = &mAcousticSpaces;
        mStores[ESM4::REC_TERM] = &mTerminals;
        mStores[ESM4::REC_TACT] = &mTalkingActivators;
        mStores[ESM4::REC_NOTE] = &mNotes;
      //mStores[ESM4::REC_IMOD] = &mItemMods;
        mStores[ESM4::REC_PWAT] = &mPlaceableWaters;
        mStores[ESM4::REC_SCOL] = &mStaticCollections;
      //mStores[ESM4::REC_CCRD] = &mCaravanCard;
      //mStores[ESM4::REC_CMNY] = &mCaravanMoney;
    }

    CellStore::CellStore (CellStore&& other)
    {
        *this = std::move(other);
    }

    CellStore& CellStore::operator=(CellStore&& other)
    {
        if (this != &other)
        {
            mCell = other.mCell;
            mState = other.mState;
            mHasState = other.mHasState;
            mWaterLevel = other.mWaterLevel;
            mIsForeignCell = other.mIsForeignCell;
            mIsDummyCell = other.mIsDummyCell;
            mForeignLand = other.mForeignLand;
            mAudioLocation = other.mAudioLocation;
            mLastRespawn = other.mLastRespawn;

            mFogState = std::move(other.mFogState);

            mIds = std::move(other.mIds);
            // FIXME: currently unused because getPtr() only uses hasId()
          //mForeignIds = std::move(other.mForeignIds);

            mActivators = std::move(other.mActivators);
            mPotions = std::move(other.mPotions);
            mAppas = std::move(other.mAppas);
            mArmors = std::move(other.mArmors);
            mBooks = std::move(other.mBooks);
            mClothes = std::move(other.mClothes);
            mContainers = std::move(other.mContainers);
            mCreatures = std::move(other.mCreatures);
            mDoors = std::move(other.mDoors);
            mIngreds = std::move(other.mIngreds);
            mCreatureLists = std::move(other.mCreatureLists);
            mItemLists = std::move(other.mItemLists);
            mLights = std::move(other.mLights);
            mLockpicks = std::move(other.mLockpicks);
            mMiscItems = std::move(other.mMiscItems);
            mNpcs = std::move(other.mNpcs);
            mProbes = std::move(other.mProbes);
            mRepairs = std::move(other.mRepairs);
            mStatics = std::move(other.mStatics);
            mWeapons = std::move(other.mWeapons);
            //
            mSounds = std::move(other.mSounds);
            mForeignActivators = std::move(other.mForeignActivators);
            mForeignApparatus = std::move(other.mForeignApparatus);
            mForeignArmors = std::move(other.mForeignArmors);
            mForeignBooks = std::move(other.mForeignBooks);
            mForeignClothes = std::move(other.mForeignClothes);
            mForeignContainers = std::move(other.mForeignContainers);
            mForeignDoors = std::move(other.mForeignDoors);
            mForeignIngredients = std::move(other.mForeignIngredients);
            mForeignLights = std::move(other.mForeignLights);
            mForeignMiscItems = std::move(other.mForeignMiscItems);
            mForeignStatics = std::move(other.mForeignStatics);
            mForeignGrasses = std::move(other.mForeignGrasses);
            mForeignTrees = std::move(other.mForeignTrees);
            mForeignFloras = std::move(other.mForeignFloras);
            mForeignFurnitures = std::move(other.mForeignFurnitures);
            mForeignWeapons = std::move(other.mForeignWeapons);
            mAmmunitions = std::move(other.mAmmunitions);
            mForeignNpcs = std::move(other.mForeignNpcs);
            mForeignCreatures = std::move(other.mForeignCreatures);
            mLevelledCreatures = std::move(other.mLevelledCreatures);
            mIdleMarkers = std::move(other.mIdleMarkers);
            mSoulGems = std::move(other.mSoulGems);
            mForeignKeys = std::move(other.mForeignKeys);
            mForeignPotions = std::move(other.mForeignPotions);
            mSubSpaces = std::move(other.mSubSpaces);
            mSigilStones = std::move(other.mSigilStones);
            mLevelledItems = std::move(other.mLevelledItems);
            mLevelledNpcs = std::move(other.mLevelledNpcs);
            mMovableStatics = std::move(other.mMovableStatics);
            mAcousticSpaces = std::move(other.mAcousticSpaces);
            mTerminals = std::move(other.mTerminals);
            mTalkingActivators = std::move(other.mTalkingActivators);
            mNotes = std::move(other.mNotes);
            mPlaceableWaters = std::move(other.mPlaceableWaters);
            mStaticCollections = std::move(other.mStaticCollections);

            mStores = std::move(other.mStores);
            mStoreTypes = std::move(other.mStoreTypes);
            mSceneNodeMap = std::move(other.mSceneNodeMap);

            mForeignPathgrids = std::move(other.mForeignPathgrids);
            mPathgrid = std::move(other.mPathgrid);
            mPathgridGraph = std::move(other.mPathgridGraph);

            mStores[ESM4::REC_SOUN] = &mSounds;
            mStores[ESM4::REC_ACTI] = &mForeignActivators;
            mStores[ESM4::REC_APPA] = &mForeignApparatus;
            mStores[ESM4::REC_ARMO] = &mForeignArmors;
            mStores[ESM4::REC_BOOK] = &mForeignBooks;
            mStores[ESM4::REC_CLOT] = &mForeignClothes;
            mStores[ESM4::REC_CONT] = &mForeignContainers;
            mStores[ESM4::REC_DOOR] = &mForeignDoors;
            mStores[ESM4::REC_INGR] = &mForeignIngredients;
            mStores[ESM4::REC_LIGH] = &mForeignLights;
            mStores[ESM4::REC_MISC] = &mForeignMiscItems;
            mStores[ESM4::REC_STAT] = &mForeignStatics;
            mStores[ESM4::REC_GRAS] = &mForeignGrasses;
            mStores[ESM4::REC_TREE] = &mForeignTrees;
            mStores[ESM4::REC_FLOR] = &mForeignFloras;
            mStores[ESM4::REC_FURN] = &mForeignFurnitures;
            mStores[ESM4::REC_WEAP] = &mForeignWeapons;
            mStores[ESM4::REC_AMMO] = &mAmmunitions;
            mStores[ESM4::REC_NPC_] = &mForeignNpcs;
            mStores[ESM4::REC_CREA] = &mForeignCreatures;
            mStores[ESM4::REC_LVLC] = &mLevelledCreatures;
            mStores[ESM4::REC_IDLM] = &mIdleMarkers;
            mStores[ESM4::REC_SLGM] = &mSoulGems;
            mStores[ESM4::REC_KEYM] = &mForeignKeys;
            mStores[ESM4::REC_ALCH] = &mForeignPotions;
            mStores[ESM4::REC_SBSP] = &mSubSpaces;
            mStores[ESM4::REC_SGST] = &mSigilStones;
            mStores[ESM4::REC_LVLI] = &mLevelledItems;
            mStores[ESM4::REC_LVLN] = &mLevelledNpcs;
            mStores[ESM4::REC_MSTT] = &mMovableStatics;
          //mStores[ESM4::REC_TXST] = &mTextureSets;
          //mStores[ESM4::REC_SCRL] = &mForeignScrolls;
          //mStores[ESM4::REC_ARMA] = &mArmorAddons;
            mStores[ESM4::REC_ASPC] = &mAcousticSpaces;
            mStores[ESM4::REC_TERM] = &mTerminals;
            mStores[ESM4::REC_TACT] = &mTalkingActivators;
            mStores[ESM4::REC_NOTE] = &mNotes;
          //mStores[ESM4::REC_IMOD] = &mItemMods;
            mStores[ESM4::REC_PWAT] = &mPlaceableWaters;
            mStores[ESM4::REC_SCOL] = &mStaticCollections;
          //mStores[ESM4::REC_CCRD] = &mCaravanCard;
          //mStores[ESM4::REC_CMNY] = &mCaravanMoney;
        }

        return *this;
    }

    const ESM::Cell *CellStore::getCell() const
    {
        return mCell;
    }

    CellStore::State CellStore::getState() const
    {
        return mState;
    }

    bool CellStore::hasState() const
    {
        return mHasState;
    }

    // this is only used by MWWorld::Cells::getPtr() (and a few places in OpenCS) but:
    //
    // Cells::getPtr() itself is called by World::searchPtr(const std::string& name, bool activeOnly)
    // in return used by MechanicsManager::isAllowedToUse(), InterpreterContext::getReferenceImp(),
    // MWScript::ExplicitRef::operator() and some AI classes/methods.
    //
    // TODO: probably worth making it efficient, but how to make the caller use formid?
    bool CellStore::hasId (const std::string& id) const
    {
        if (mState==State_Unloaded)
            return false;

        if (mState==State_Preloaded)
            return std::binary_search (mIds.begin(), mIds.end(), id);

        /// \todo address const-issues
        return const_cast<CellStore *> (this)->search (id).isEmpty();
    }

    int CellStore::getStoreType(ESM4::FormId formId) const
    {
        std::map<ESM4::FormId, int>::const_iterator it = mStoreTypes.find(formId);
        if (it == mStoreTypes.end())
            return 0;
        else
            return it->second;
    }

    // FIXME: currently unused because getPtr() only uses hasId()
    bool CellStore::hasFormId(ESM4::FormId formId) const
    {
        if (mState == State_Unloaded)
            return false;

        // FIXME: for binary_search to work properly mForeignIds needs to be sorted first
        // (note that mIds does get sorted in listRefs())
        if (mState == State_Preloaded)
            return std::binary_search(mForeignIds.begin(), mForeignIds.end(), formId);

        // FIXME: why is this done?  shouldn't they all have an entry in mFormIds?  is it for
        // dynamically created ones? if so why not create a new list of dynamic ids?
        return const_cast<CellStore *> (this)->search(formId).isEmpty();
    }

    Ptr CellStore::search(ESM4::FormId formId)
    {
        bool oldState = mHasState;

        mHasState = true;

        int storeType = getStoreType(formId);

        std::map<int, CellRefStoreBase*>::const_iterator iter = mStores.find(storeType);
        if (iter != mStores.end())
        {
            LiveCellRefBase *liveRef = iter->second->find(formId);

            return Ptr(liveRef, this);
        }

        mHasState = oldState;

        return Ptr();
    }

    Ptr CellStore::searchViaEditorId(const std::string& editorId)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (LiveCellRefBase *ref = mForeignActivators.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignPotions.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignApparatus.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignArmors.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignBooks.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignClothes.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignContainers.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignCreatures.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignDoors.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignIngredients.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mLevelledCreatures.find (editorId))
            return Ptr (ref, this);

        //if (LiveCellRefBase *ref = mLevelledItems.find (editorId))
            //return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignLights.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignKeys.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignMiscItems.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignNpcs.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignFurnitures.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignFloras.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mSoulGems.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mSigilStones.find (editorId))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mForeignWeapons.find (editorId))
            return Ptr (ref, this);

        // needed for script parsing
        if (LiveCellRefBase *ref = mForeignStatics.find (editorId))
            return Ptr (ref, this);

        mHasState = oldState;

        return Ptr();
    }

    Ptr CellStore::search(const std::string& id)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (LiveCellRefBase *ref = mActivators.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mPotions.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mAppas.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mArmors.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mBooks.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mClothes.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mContainers.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mCreatures.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mDoors.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mIngreds.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mCreatureLists.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mItemLists.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mLights.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mLockpicks.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mMiscItems.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mNpcs.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mProbes.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mRepairs.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mStatics.find (id))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mWeapons.find (id))
            return Ptr (ref, this);

        mHasState = oldState;

        return Ptr();
    }

    void CellStore::removeLookupKeys (const std::string& handle, ESM4::FormId formId)
    {
        std::unordered_map<std::string, ESM4::FormId>::iterator handleIter = mSceneNodeMap.find(handle);
        if (handleIter != mSceneNodeMap.end())
            mSceneNodeMap.erase(handleIter);

        // TODO: don't remove the formid from mStoreTypes
        //std::map<ESM4::FormId, int>::iterator typeIter = mStoreTypes.find(formId);
        //if (typeIter != mStoreTypes.end())
            //mStoreTypes.erase(typeIter);

        // TODO: mark it with 0xFFFFFFFF instead?
      //std::vector<ESM4::FormId>::iteroator idIter = mForeignIds.find(formId);
      //if (idIter != mForeignIds.end())
      //    mForeignIds.erase(idIter);
    }

    void CellStore::updateHandleMap (const std::string& handle, ESM4::FormId formId)
    {
        if (handle != "")
        {
            mSceneNodeMap[handle] = formId; // for searchViaHandle()
            //std::cout << "adding handle" << std::endl;
        }
    }

    void CellStore::updateLookupMaps (ESM4::FormId formId, LiveCellRefBase *ref, int storeType)
    {
        // FIXME: currently unused because getPtr() only uses hasId()
        //mForeignIds.push_back(formId); // for hasFormId()

        // storeType -1 is called from Scene::InsertFunctor
        // (for these we should have the formids already from load)
        if (storeType != -1)
            mStoreTypes[formId] = storeType;

        if (ref && ref->mData.getBaseNode())
            mSceneNodeMap[ref->mData.getHandle()] = formId; // for searchViaHandle()
    }

    Ptr CellStore::searchViaHandle (const std::string& handle)
    {
        bool oldState = mHasState;

        mHasState = true;

        // mSceneNodeMap is populated in forEachImpForeign() and updateHandleMap()
        std::unordered_map<std::string, ESM4::FormId>::const_iterator handleIter = mSceneNodeMap.find(handle);
        if (handleIter != mSceneNodeMap.end())
        {
            ESM4::FormId formId = handleIter->second;

            int storeType = getStoreType(formId);
            //std::cout << "type " << ESM4::printName(storeType) << std::endl;

            std::map<int, CellRefStoreBase*>::const_iterator iter = mStores.find(storeType);
            if (iter == mStores.end())
            {
                mHasState = oldState;
                throw std::runtime_error ("CellStore: store not found for "+ESM4::printName(storeType));
            }

            LiveCellRefBase *liveRef = iter->second->find(formId);
            if (liveRef)
                return Ptr(liveRef, this);
        }
        //else if (mIsForeignCell)
            //std::cout << "handle not found" << std::endl; // FIXME: why does this happen?

        if (LiveCellRefBase *ref = mActivators.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mPotions.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mAppas.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mArmors.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mBooks.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mClothes.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mContainers.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mCreatures.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mDoors.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mIngreds.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mCreatureLists.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mItemLists.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mLights.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mLockpicks.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mMiscItems.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mNpcs.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mProbes.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mRepairs.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mStatics.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRefBase *ref = mWeapons.searchViaHandle (handle))
            return Ptr (ref, this);

        mHasState = oldState;

        return Ptr();
    }

    Ptr CellStore::searchViaActorId (int id)
    {
        if (Ptr ptr = ::searchViaActorId (mForeignNpcs, id, this))
            return ptr;

        if (Ptr ptr = ::searchViaActorId (mForeignCreatures, id, this))
            return ptr;

        if (Ptr ptr = ::searchViaActorId (mNpcs, id, this))
            return ptr;

        if (Ptr ptr = ::searchViaActorId (mCreatures, id, this))
            return ptr;

        return Ptr();
    }

    float CellStore::getWaterLevel() const
    {
        return mWaterLevel;
    }

    void CellStore::setWaterLevel (float level)
    {
        mWaterLevel = level;
        mHasState = true;
    }

    int CellStore::count() const
    {
        return int(
            mActivators.mList.size()
            + mPotions.mList.size()
            + mAppas.mList.size()
            + mArmors.mList.size()
            + mBooks.mList.size()
            + mClothes.mList.size()
            + mContainers.mList.size()
            + mDoors.mList.size()
            + mIngreds.mList.size()
            + mCreatureLists.mList.size()
            + mItemLists.mList.size()
            + mLights.mList.size()
            + mLockpicks.mList.size()
            + mMiscItems.mList.size()
            + mProbes.mList.size()
            + mRepairs.mList.size()
            + mStatics.mList.size()
            + mWeapons.mList.size()
            + mCreatures.mList.size()
            + mNpcs.mList.size()
            //
            + mSounds.mList.size()
            + mForeignActivators.mList.size()
            + mForeignApparatus.mList.size()
            + mForeignArmors.mList.size()
            + mForeignBooks.mList.size()
            + mForeignClothes.mList.size()
            + mForeignContainers.mList.size()
            + mForeignDoors.mList.size()
            + mForeignIngredients.mList.size()
            + mForeignLights.mList.size()
            + mForeignMiscItems.mList.size()
            + mForeignStatics.mList.size()
            + mForeignGrasses.mList.size()
            + mForeignTrees.mList.size()
            + mForeignFloras.mList.size()
            + mForeignFurnitures.mList.size()
            + mForeignWeapons.mList.size()
            + mAmmunitions.mList.size()
            + mForeignNpcs.mList.size()
            + mForeignCreatures.mList.size()
            + mLevelledCreatures.mList.size()
            + mForeignKeys.mList.size()
            + mForeignPotions.mList.size()
            + mSubSpaces.mList.size()
            + mLevelledItems.mList.size());
          // TERM, TACT, etc
    }

    int CellStore::getRefrEstimate(std::int32_t groupType) const
    {
        assert(mIsForeignCell);
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        return (int)cell->getRefrEstimate(groupType);
    }

    int CellStore::getPersistentRefrCount() const
    {
        assert(mIsForeignCell);
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        return (int)cell->getPersistentRefrCount();
    }

    void CellStore::load (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        if (!mIsForeignCell)
        {
            if (mState!=State_Loaded)
            {
                if (mState==State_Preloaded)
                    mIds.clear();

                loadRefs (store, esm);

                mState = State_Loaded;

                // TODO: the pathgrid graph only needs to be loaded for active cells, so move this somewhere else.
                // In a simple test, loading the graph for all cells in MW + expansions took 200 ms
                mPathgridGraph.load(this);
            }
        }
        else // mForeignCell
        {
            if (mState != State_Loaded)
            {
                // FIXME: currently unused because getPtr() only uses hasId()
              //if (mState == State_Preloaded)
              //    mForeignIds.clear();

                loadForeignRefs(store, esm);

                mState = State_Loaded;

                // Foreign::TerrainStorage::getLand() calls MWWorld::Cells::getWorldCell()
                // which could mean a lot of unnecessary loading - build PathgridGraph later
                // (but then again the cell would be empty and the overhead won't be much anyway?)
            }
        }
    }

    void CellStore::preload (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        if (mCell && !mIsForeignCell)
        {
            if (mState==State_Unloaded)
            {
                listRefs (store, esm);

                mState = State_Preloaded;
            }
        }
    }

    void CellStore::listRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (*esm[0][index], (int)i); // FIXME: hardcoded 0 means TES3

            ESM::CellRef ref;

            // Get each reference in turn
            bool deleted = false;
            while (mCell->getNextRef (*esm[0][index], ref, deleted)) // FIXME hardcoded 0 means TES3
            {
                if (deleted)
                    continue;

                // Don't list reference if it was moved to a different cell.
                ESM::MovedCellRefTracker::const_iterator iter =
                    std::find(mCell->mMovedRefs.begin(), mCell->mMovedRefs.end(), ref.mRefNum);
                if (iter != mCell->mMovedRefs.end()) {
                    continue;
                }

                mIds.push_back (Misc::StringUtils::lowerCase (ref.mRefID));
            }
        }

        // List moved references, from separately tracked list.
        for (ESM::CellRefTracker::const_iterator it = mCell->mLeasedRefs.begin(); it != mCell->mLeasedRefs.end(); ++it)
        {
            const ESM::CellRef &ref = *it;

            mIds.push_back(Misc::StringUtils::lowerCase(ref.mRefID));
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (*esm[0][index], (int)i); // FIXME: hardcoded 0 means TES3

            ESM::CellRef ref;
            ref.mRefNum.mContentFile = ESM::RefNum::RefNum_NoContentFile;

            // Get each reference in turn
            bool deleted = false;
            while(mCell->getNextRef(*esm[0][index], ref, deleted)) // FIXME: 0 means TES3
            {
                // Don't load reference if it was moved to a different cell.
                ESM::MovedCellRefTracker::const_iterator iter =
                    std::find(mCell->mMovedRefs.begin(), mCell->mMovedRefs.end(), ref.mRefNum);
                if (iter != mCell->mMovedRefs.end()) {
                    continue;
                }

                loadRef (ref, deleted, store);
            }
        }

        // Load moved references, from separately tracked list.
        for (ESM::CellRefTracker::const_iterator it = mCell->mLeasedRefs.begin();
                it != mCell->mLeasedRefs.end(); ++it)
        {
            ESM::CellRef &ref = const_cast<ESM::CellRef&>(*it);

            loadRef (ref, false, store);
        }
    }

    // FIXME: this can be optimised by checking if a cell has any refs - see ForeignCell::mHasChildren
    void CellStore::loadForeignRefs(const MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& esm)
    {
        assert(mIsForeignCell);
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        if (!cell)
            return; // FIXME: probably spelling error from console

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < cell->mModList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int modIndex = cell->mModList.at(i).modIndex >> 24; // FIXME: Morroblivion
            // HACK: Should find another way...  Anyway, TES4 has 20 byte header, TES5 24.
            int modType = (cell->mModList.at(i).recHeaderSize == 20) ? 1 : 2; // FIXME: hard coded 1 and 2
            ESM::ESM4Reader *esm4 = static_cast<ESM::ESM4Reader*>(esm[modType][modIndex]);
            esm4->restoreCellChildrenContext(cell->mModList.at(i));
            // FIXME: need a way to load the cell children (just the refs?)
            //
            // 1. skip cell itself (should have been preloaded)
            // 2. load cell child group
            //std::cout << "file " << cell->mModList.at(i).filename << std::endl;

            // hasMoreRecs() here depends on the hack in restoreCellChildrenContext()
            while(esm[modType][modIndex]->hasMoreRecs())
            {
                ESM4::Reader& reader = esm4->reader();
                reader.checkGroupStatus();

                loadTes4Group(store, *esm[modType][modIndex]);
            }

            const ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ForeignWorld *world
                = store.getForeign<ForeignWorld>().find(static_cast<const ForeignCell*>(mCell)->mCell->mParent);
            if (!world)
                continue;

            //if (world->mMapFile == "") // don't use parent world's land for FO3/FONV
            //if ((world->mParentUseFlags & 0x01/*use land data*/) == 0) // FIXME: doesn't work for TES4
            if ((world->mWorldFlags & 0x10/*no landscape*/) == 0)
                continue;

            if (world->mParent != 0) // ok, use parent world land but do I have a parent world?
            {
                CellStore * parentCell
                    = MWBase::Environment::get().getWorld()->getWorldCell(world->mParent,
                        mCell->getGridX(),
                        mCell->getGridY());

                if (parentCell)
                    mForeignLand = parentCell->getForeignLandId();
            }
        }
    }

    void CellStore::loadTes4Group (const MWWorld::ESMStore& store, ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        reader.getRecordHeader();
        const ESM4::RecordHeader& hdr = reader.hdr();

        if (hdr.record.typeId != ESM4::REC_GRUP)
            return loadTes4Record(store, reader);

        switch (hdr.group.type)
        {
            case ESM4::Grp_CellChild: // World dummy cell
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellTemporaryChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                reader.adjustGRUPFormId();  // not needed or even shouldn't be done? (only labels anyway)
                reader.saveGroupStatus();
                if (!esm.hasMoreRecs())
                    return; // may have been an empty group followed by EOF

                loadTes4Group(store, esm);

                break;
            }
#if 0 // FIXME: temporary testing only
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                //if (hdr.group.type == ESM4::Grp_CellPersistentChild)
                    std::cout << ESM4::printLabel(reader.hdr().group.label, reader.hdr().group.type) << std::endl;
                reader.skipGroup();
                break;
            }
#endif
            case ESM4::Grp_RecordType:
            case ESM4::Grp_WorldChild:
            case ESM4::Grp_TopicChild:
            case ESM4::Grp_ExteriorCell:
            case ESM4::Grp_ExteriorSubCell:
            case ESM4::Grp_InteriorCell:
            case ESM4::Grp_InteriorSubCell:
            default:
                std::cout << "unexpected group..." << std::endl; // FIXME
                reader.skipGroup();
                break;
        }

        return;
    }

    void CellStore::loadTes4Record (const MWWorld::ESMStore& store, ESM4::Reader& reader)
    {
        // Assumes that the reader has just read the record header only.
        //ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        const ESM4::RecordHeader& hdr = reader.hdr();

        switch (hdr.record.typeId)
        {
            case ESM4::REC_REFR:
            {
                // FIXME: testing WhiteRun
                if ((reader.hdr().record.flags & ESM4::Rec_Disabled) != 0)
                {
#if 1
                    reader.skipRecordData();
#else
                    reader.getRecordData();
                    ESM4::Reference record;
                    record.load(reader);
                    std::cout << "disabled " << record.mEditorId << std::endl;

                    if (record.mEditorId == "CWSiegeWhiterunFireEnblerMrk")
                        std::cout << std::hex << record.mFormId << std::endl;

                    if (record.mBaseObj == 0x00100d2f)
                        std::cout << "? " << record.mEditorId << std::endl;
                    switch (store.find(record.mEsp.parent))
                    {
                        case ESM4::REC_REFR: std::cout << "parent refr " << std::endl; break;
                        default: std::cout << std::hex << store.find(record.mEsp.parent) << std::endl; break;
                    }
#endif
                    break;
                }

                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::Reference record;
                record.load(reader);
                //if (!record.mEditorId.empty())
                    //std::cout << "REFR: " << record.mEditorId << std::endl; // FIXME

#if 0
                //if (reader.grp().type == ESM4::Grp_CellPersistentChild)
                if (record.mGroupType == ESM4::Grp_CellPersistentChild)
                    //if (record.mIsMapMarker)
                    std::cout << "Persistent REFR: " << record.mEditorId << " 0x"
                        << ESM4::formIdToString(record.mFormId) << std::endl;
#endif

                // currently unused
                //const int cellSize = 4096;
                //int newX = static_cast<int>(std::floor(record.mPlacement.pos.x / cellSize));
                //int newY = static_cast<int>(std::floor(record.mPlacement.pos.y / cellSize));




                // FIXME: for temp testing
                //if (newX < (5-16) || newX > (5+16) || newY < (12-16) || newY > (12+16))
                    //break;





                if (record.mEsp.parent != 0)
                {
                    int res = store.getRecordType(record.mEsp.parent);
                    if (res == ESM4::REC_REFR)
                        std::cout << "flags " << std::hex << record.mEsp.flags << std::endl;

                    if ((record.mEsp.parent == 0x000376e0 || record.mEsp.parent == 0x000fc90b ||
                         record.mEsp.parent == 0x00100d2c || record.mEsp.parent == 0x00100d98)
                            && ((record.mEsp.flags & 0x01) == 0))
                    {
                        std::cout << "CWSiegeWhiterunFireEnablerMrk" << std::endl;
                        std::cout << "CWWhiterunIntEnableOnly" << std::endl;
                        std::cout << "CWSolClutterToggle" << std::endl;
                        std::cout << "CWSolClutterEnableForever" << std::endl;
                        std::cout << std::hex << record.mEsp.flags << std::endl;
                        break;
                    }
                }
#if 0
                if ((record.mFlags & ESM4::Rec_VisDistant) != 0 && reader.getContext().groupStack.back().first.type != ESM4::Grp_CellVisibleDistChild)
                {
                    std::string padding = ""; // FIXME: debugging only
                    padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                    std::cout << padding << "CellStore REFR " << record.mEditorId << " "
                              << ESM4::formIdToString(record.mFormId) << " visible dist" << std::endl;
                }

                if ((record.mFlags & ESM4::Rec_VisDistant) == 0 && reader.getContext().groupStack.back().first.type == ESM4::Grp_CellVisibleDistChild)
                {
                    std::string padding = ""; // FIXME: debugging only
                    padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                    std::cout << padding << "CellStore REFR " << record.mEditorId << " "
                              << ESM4::formIdToString(record.mFormId) << " NOT visible dist" << std::endl;
                }
#endif
                // unfortunately at this point most of the worlds are not yet loaded and
                // hence nothing will be found - try interior only
                //if (record.mBaseObj == 0x23) // AudioMarker
                //if (record.mAudioLocation)
                if (record.mAudioLocation && !getCell()->isExterior())
                {
                    ESM4::FormId worldId
                        = static_cast<const MWWorld::ForeignCell*>(getCell())->mCell->mParent;
                    const ForeignWorld *world = store.getForeign<ForeignWorld>().find(worldId);
                    if (world)
                    {
                        std::string worldName = world->mEditorId;
                        std::cout << "REFR world audio marker " << worldName << std::endl;
                    }
                    else // shouldn't be any interior dummy cells
                        //std::cout << "REFR audio marker " << (isDummyCell() ? " dummy cell" : "") << std::endl;
                        std::cout << "REFR " << ESM4::formIdToString(record.mFormId) << " audio marker "
                                  << /*15d026*/ ESM4::formIdToString(record.mAudioLocation) << std::endl;

                    mAudioLocation = record.mAudioLocation; // will be processed later when cell becomes active
                }

                //if (record.mBaseObj == 0x0016B46E) // TACT in cell ULCasino
                     //std::cout << "TACT" << std::endl;

                // FIXME: replace REFR even if the baseObj is missing?
                if (!record.mBaseObj)
                    break;

                // FIXME: for vis dist testing (see scene.cpp)
                bool mapGrid = mIsDummyCell || true;

                switch (store.getRecordType(record.mBaseObj))
                {
                    case ESM4::REC_SOUN: mSounds.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_SOUN; break;
#if 1
                    case ESM4::REC_ACTI: mForeignActivators.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_ACTI; break;
#else
                    case ESM4::REC_ACTI):
                    {
                        const MWWorld::ForeignStore<ESM4::Activator>& actiStore
                            = store.getForeign<ESM4::Activator>();
                        const ESM4::Activator *acti = actiStore.search(record.mBaseObj);
                        if (acti)
                            std::cout << "activator " << acti->mEditorId << std::endl;

                        mForeignActivators.load(record, deleted, store);
                        break;
                    }
#endif
                    case ESM4::REC_APPA: mForeignApparatus.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_APPA; break;
                    case ESM4::REC_ARMO: mForeignArmors.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_ARMO; break;
                    case ESM4::REC_BOOK: mForeignBooks.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_BOOK; break;
                    case ESM4::REC_CLOT: mForeignClothes.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_CLOT; break;
                    case ESM4::REC_CONT: mForeignContainers.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_CONT; break;
                    case ESM4::REC_DOOR:
                    {
                        mForeignDoors.load(record, deleted, store, mapGrid);
                        store.setDoorCell(record.mFormId, reader.currCell());
                        mStoreTypes[record.mFormId] = ESM4::REC_DOOR;
                        break;
                    }
                    case ESM4::REC_INGR: mForeignIngredients.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_INGR; break;
                    case ESM4::REC_LIGH: mForeignLights.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_LIGH; break;
                    case ESM4::REC_MISC: mForeignMiscItems.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_MISC; break;
                    case ESM4::REC_STAT:
                    {
                        mForeignStatics.load(record, deleted, store, mapGrid);
                        mStoreTypes[record.mFormId] = ESM4::REC_STAT;
#if 0
                        if (record.mEditorId == "DoorMarker") // FIXME
                             std::cout << "DoorMarker: " << " 0x"
                                 << ESM4::formIdToString(record.mFormId) << std::endl;
#endif
#if 0
                        //if (reader.getContext().currWorld == 0x0001D0BC &&
                            //reader.getContext().groupStack.back().first.type == ESM4::Grp_CellVisibleDistChild)
                        if (reader.getContext().currWorld == 0x0000003C &&
                            reader.getContext().groupStack.back().first.type == ESM4::Grp_CellPersistentChild)
                        {
                            std::string padding = ""; // FIXME: debugging only
                            padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                            std::cout << padding << "CellStore REFR " << record.mEditorId << " "
                                  //<< ESM4::formIdToString(reader.getContext().currCell) << " visible dist "
                                  << ESM4::formIdToString(reader.getContext().currCell) << " persistent "
                                  << ESM4::formIdToString(record.mBaseObj) << std::endl;
                        }
#endif
                        break;
                    }
                    case ESM4::REC_GRAS: mForeignGrasses.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_GRAS; break;
                    //case ESM4::REC_TREE: mForeignTrees.load(record, deleted, store, mapGrid);
                                         //mStoreTypes[record.mFormId] = ESM4::REC_TREE; break;
                    case ESM4::REC_FLOR: mForeignFloras.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_FLOR; break;
                    case ESM4::REC_FURN: mForeignFurnitures.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_FURN; break;
                    case ESM4::REC_WEAP: mForeignWeapons.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_WEAP; break;
                    case ESM4::REC_AMMO: mAmmunitions.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_AMMO; break;
                    case ESM4::REC_LVLC:
                    {
                                         mLevelledCreatures.load(record, deleted, store, mapGrid); // only TES4
                                         mStoreTypes[record.mFormId] = ESM4::REC_LVLC; break;
                    }
                    case ESM4::REC_SLGM: mSoulGems.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_SLGM; break;
                    case ESM4::REC_KEYM: mForeignKeys.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_KEYM; break;
                    case ESM4::REC_ALCH: mForeignPotions.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_ALCH; break;
                    case ESM4::REC_SBSP: mSubSpaces.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_SBSP; break;
                    case ESM4::REC_SGST: mSigilStones.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_SGST; break;
                    case ESM4::REC_LVLI: mLevelledItems.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_LVLI; break;
                    case ESM4::REC_TERM: mTerminals.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_TERM; break;
                    case ESM4::REC_TACT: mTalkingActivators.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_TACT; break;
                    case ESM4::REC_NOTE: mNotes.load(record, deleted, store, mapGrid);
                                         mStoreTypes[record.mFormId] = ESM4::REC_NOTE; break;
                    case ESM4::REC_LVLN: // Leveled NPC
                    {
                                         mLevelledNpcs.load(record, deleted, store, mapGrid); // never occurs?
                                         mStoreTypes[record.mFormId] = ESM4::REC_LVLN; break;
                    }
                    case ESM4::REC_TREE: // FIXME
                    case ESM4::REC_IDLM: // Idle Marker
                    case ESM4::REC_MSTT: // Movable Static
                    case ESM4::REC_TXST: // Texture Set
                    case ESM4::REC_SCRL: // Scroll
                    case ESM4::REC_ARMA: // Armor Addon
                    case ESM4::REC_ASPC: // Acoustic Space
                    case ESM4::REC_IMOD: // Item Mod
                    case ESM4::REC_PWAT: // Placeable Water
                    case ESM4::REC_SCOL: // Static Collection
                         break;

                    // FO3 adds ASPC, IDLM, ARMA, MSTT, NOTE, PWAT, SCOL, TACT, TERM and TXST
                    // FONV adds CCRD, IMOD and CMNY
                    case 0: std::cerr << "Cell refr " + ESM4::formIdToString(record.mFormId) +" "+ ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr << "WARNING: Ignoring TES4 reference '"
                                  << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }

// FIXME: testing only
#if 0
                if (record.mEsp.parent != 0)
                {
                    uint32_t baseType = store.find(record.mBaseObj);
                    std::cout << "base type "
                              << ESM4::printName(((baseType & 0xffffff00)>>8)|((baseType & 0xff)<<24))
                              << " parent formid " << ESM4::formIdToString(record.mEsp.parent) << std::endl;
                    // parent type appears to be REFR
                }
#endif
                break;
            }
            case ESM4::REC_ACHR:
            {
                if (0)//(reader.hdr().record.flags & ESM4::Rec_Disabled) != 0) // FIXME: temp testing
                {
                    reader.skipRecordData();
                    break;
                }

                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::ActorCharacter record;
                record.load(reader);
                //if (!record.mEditorId.empty())
                    //std::cout << ESM4::printName(hdr.record.typeId) << ": " << record.mEditorId << std::endl; // FIXME

                switch (store.getRecordType(record.mBaseObj))
                {
                    case ESM4::REC_NPC_:
                    {
#if 0
                        // this might be a levelled actor (e.g. FO3) - check if the model is empty
                        const ESM4::Npc* npc = store.getForeign<ESM4::Npc>().search(record.mBaseObj);
                        //
                        //if ((npc->mBaseConfig.fo3.flags & 0x1) == 0) // FIXME: temp testing
                            //std::cout << npc->mEditorId << " male" << std::endl;
                        //
                        if (npc && (/*npc->mModel.empty() || */npc->mModel == "marker_creature.nif"))
                            for (size_t i = 0; i < npc->mInventory.size(); ++i)
                            {
                                uint32_t type = store.find(npc->mInventory[i].item);
                                if (type == ESM4::REC_BOOK)
                              //if (type == ESM4::REC_NOTE)
                                {
                        const ESM4::Book* note = store.getForeign<ESM4::Book>().search(npc->mInventory[i].item);
                        std::cout << npc->mEditorId << " " << note->mEditorId << std::endl;
                                }
                                else
                                    std::cout << npc->mEditorId << " " << ESM4::printName(type) << std::endl;
                            }
                        if (npc && npc->mBaseTemplate != 0
                                && (npc->mModel.empty() || npc->mModel == "marker_creature.nif"))
                        {
#if 0
            if (npc->mBaseTemplate != 0)
            {
                // has baseconfig, inventory, hair colour, hair length, race, class; all else 0
                const ESM4::Race* race = store.getForeign<ESM4::Race>().search(npc->mRace);
                const ESM4::Npc& newNpc = *npc;
                std::vector<const ESM4::Npc*> sts;
                sts.push_back(npc);
                bool isFemale = (npc->mBaseConfig.fo3.flags & 0x000001) != 0;
                //if (isFemale)
                std::cout << "ori " << newNpc.mEditorId << (isFemale? " female" : "") << std::endl;
                std::cout << race->mEditorId << std::endl;
                std::cout << ESM4::formIdToString(newNpc.mClass) << std::endl;
            }
#endif
                            ESM4::FormId id = npc->mBaseTemplate;
                            uint32_t type = store.find(id);
                            bool found = false;

                            while (!found) {
                                if (type == ESM4::REC_NPC_)
                                {
                                    // FIXME: for TES5 need to check male/female model
                                    record.mBaseObj = id;
                                    found = true;
#if 0 // 32 64 23 209, inv 2
            //bool isFemale = (record.mBaseConfig.fo3.flags & 0x000001) != 0;
            //std::cout << record.mEditorId << (isFemale? " female" : " male") << std::endl;
            const ESM4::Npc* newNpc = store.getForeign<ESM4::Npc>().search(id);
            const ESM4::Race* race = store.getForeign<ESM4::Race>().search(newNpc->mRace);
            std::vector<const ESM4::Npc*> sts;
            sts.push_back(newNpc);
            bool isFemale = (newNpc->mBaseConfig.fo3.flags & 0x000001) != 0;
            std::cout << newNpc->mEditorId << (isFemale? " female" : "") << std::endl;
            std::cout << race->mEditorId << std::endl;
            std::cout << ESM4::formIdToString(newNpc->mClass) << std::endl;
                            for (size_t i = 0; i < newNpc->mInventory.size(); ++i)
                            {
                                uint32_t type = store.find(newNpc->mInventory[i].item);
                                if (type == ESM4::REC_BOOK)
                              //if (type == ESM4::REC_NOTE)
                                {
                        const ESM4::Book* note = store.getForeign<ESM4::Book>().search(newNpc->mInventory[i].item);
                        std::cout << newNpc->mEditorId << " " << note->mEditorId << std::endl;
                                }
                                else
                                    std::cout << newNpc->mEditorId << " " << ESM4::printName(type) << std::endl;
                            }
#endif
                                }
                                else if (type == ESM4::REC_LVLN)
                                {
                                    const ESM4::LevelledNpc* lvlActor
                                        = store.getForeign<ESM4::LevelledNpc>().search(id);

                                    if (lvlActor && lvlActor->mLvlObject.size() != 0)
                                    {
                                        // FIXME: this should be based on player's level, etc
                                        id = lvlActor->mLvlObject[0].item;
                                        type = store.find(id);
                                    }
                                    else
                                        throw std::runtime_error ("levelled actor not found!");
                                }
                            }
                        }

#endif
#if 1
                        mForeignNpcs.load(record, deleted, store, mIsDummyCell);
                        mStoreTypes[record.mFormId] = ESM4::REC_NPC_;
#else
        const MWWorld::ForeignStore<ESM4::Npc> &store = esmStore.getForeign<ESM4::Npc>();

        if (const ESM4::Npc *ptr = store.search (record.mBaseObj))
        {
            // see operator== in livecellref.hpp, check for matching formId
            typename std::list<LiveRef>::iterator iter =
                std::find(mList.begin(), mList.end(), ref);

            LiveRef liveCellRef (ref, ptr); // ref is the reference, ptr is the base object

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mList.end())
                *iter = liveCellRef;
            else
            {
                mList.push_back (liveCellRef);
                //std::cout << "CellRefList::load "
                    //<< ESM4::formIdToString(mList.back().mBase->mFormId) << std::endl; // FIXME: debug only
            }
        }
#endif
                        break;
                    }

                    case 0: std::cerr << "Cell achr " + ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr
                            << "WARNING: Ignoring ACHR '" << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }
                break;
            }
            case ESM4::REC_ACRE: // Oblivion only?
            {
                if (0)//(reader.hdr().record.flags & ESM4::Rec_Disabled) != 0) // FIXME: temp testing
                {
                    reader.skipRecordData();
                    break;
                }

                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::ActorCreature record;
                record.load(reader);
                //if (!record.mEditorId.empty())
                    //std::cout << ESM4::printName(hdr.record.typeId) << ": " << record.mEditorId << std::endl; // FIXME

                switch (store.getRecordType(record.mBaseObj))
                {
                    case ESM4::REC_CREA:
                    {
#if 0
                        // this might be a levelled creature (e.g. FO3) - check if the model is empty
                        const ESM4::Creature* crea = store.getForeign<ESM4::Creature>().search(record.mBaseObj);
                        if (crea && crea->mBaseTemplate != 0
                                 && (crea->mModel.empty() || crea->mModel == "marker_creature.nif"))
                        {
                            ESM4::FormId id = crea->mBaseTemplate;
                            uint32_t type = store.find(id);
                            bool found = false;

                            while (!found) {
                                if (type == ESM4::REC_CREA)
                                {
                                    record.mBaseObj = id;
                                    found = true;
                                }
                                else if (type == ESM4::REC_LVLC)
                                {
                                    const ESM4::LevelledCreature* lvlCrea
                                        = store.getForeign<ESM4::LevelledCreature>().search(id);

                                    if (lvlCrea && lvlCrea->mLvlObject.size() != 0)
                                    {
                                        // FIXME: this should be based on player's level, etc
                                        id = lvlCrea->mLvlObject[0].item;
                                        type = store.find(id);
                                    }
                                    else
                                        throw std::runtime_error ("levelled creature not found!");
                                }
                            }
                        }

#else
                        mForeignCreatures.load(record, deleted, store, mIsDummyCell);
                        mStoreTypes[record.mFormId] = ESM4::REC_CREA;
#endif
                        break;
                    }

                    case 0: std::cerr << "Cell acre " + ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr
                            << "WARNING: Ignoring ACRE '" << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }
                break;
            }
            case ESM4::REC_LAND: // Cell temporary child group for exterior cells
            {

                // Can't store land record in CellStore if we want to keep it around rather
                // than loading it each time like references.  But then, how do we keep it
                // growing too large?  Need some way of keeping track of "recently accessed"
                // cells.
                //
                // The other issue is that of const correctness. We only get access to a const
                // pointer or reference to the ESMStore...
                //
                // HACK: Workaround by going through a method in World
                //
                //ESM4::FormId worldId = mCell->mCell->mParent;
                //ESM4::FormId cellId  = mCell->mCell->mFormId;

                // Save context just in case
                //ESM4::ReaderContext ctx = static_cast<ESM::ESM4Reader*>(&esm)->getESM4Context();

                // Load land, note may not be used
                reader.getRecordData();
                mForeignLand = MWBase::Environment::get().getWorld()->loadForeignLand(reader);

                // load parent world's land; but that triggers Tamriel to be loaded, is there another way?
#if 0
                const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);
                const ForeignWorld *world = store.getForeign<ForeignWorld>().find(cell->mCell->mParent);

                if (world && world->mParent != 0)
                {
                    CellStore * parentCell
                        = MWBase::Environment::get().getWorld()->getWorldCell(world->mParent,
                            mCell->getGridX(),
                            mCell->getGridY());

                    if (parentCell)
                        mForeignLand = parentCell->getForeignLandId();
                }
#endif
                break;
            }
            case ESM4::REC_PGRD: // TES4 only
            {
                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                ESM4::FormId cellId = static_cast<const MWWorld::ForeignCell*>(mCell)->mCell->mFormId;

                // SEtestGateRoom in Oblivion.esm has broken PGRD sub-record
                if (deleted || cellId == 0x000132b1)
                {
                    reader.skipRecordData();
                }
                else
                {
                    reader.getRecordData();
                    mForeignPathgrids.loadForeign(reader);

                    mPathgridGraph.load(this);
                    buildTES3Pathgrid(); // FIXME: just a workaround
                }

                break;
            }
            case ESM4::REC_ROAD: // TES4 only
            case ESM4::REC_NAVM:
            {
                //std::string padding = "";
                //padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                //std::cout << padding << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
                reader.skipRecordData();
                break;
            }
            case ESM4::REC_PGRE: // FO3/FONV
            {
#if 0
                reader.getRecordData();
                ESM4::PlacedGrenade record;
                record.load(reader);
                std::string padding = "";
                padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                std::cout << padding << ESM4::printName(hdr.record.typeId) << " " << record.mEditorId << std::endl;
#else
                //std::string padding = "";
                //padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                //std::cout << padding << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
                reader.skipRecordData();
#endif
                break;
            }
            default:
            {
                std::cout << "CellStore unsupported TES4 record type: " + ESM4::printName(hdr.record.typeId) << std::endl;
                reader.skipRecordData();
            }
        }

        return;
    }

    void CellStore::setLoadedState()
    {
        if (mState!=State_Loaded)
        {
            // first create a grid index



            mState = State_Loaded;
        }
        else
            throw std::runtime_error ("CellStore: unexpected call to setLoadedState");
    }

    // used by MWRender::Debugging
    const ESM4::Pathgrid *CellStore::getTES4Pathgrid() const
    {
        // NOTE: cellId is not what we want - we need the Pathgrid FormId
        ESM4::FormId cellId = static_cast<const MWWorld::ForeignCell*>(mCell)->mCell->mFormId;

        // FIXME: can there be more than one? if not why do we need a ForeignStore?
        // Is there a way to store these in ESMStore? (we only get a const reference)
        std::vector<ESM4::FormId> list;
        mForeignPathgrids.listForeignIdentifier(list);
        if (list.size())
        {
            //std::cout << "CELL formId " << ESM4::formIdToString(cellId) << std::endl;
            //std::cout << "PGRD formId " << ESM4::formIdToString(list[0]) << std::endl;
            if (list.size() > 1)
                std::cout << cellId << " cellid Pathgrid more than one " << list.size() << std::endl;

            return mForeignPathgrids.search(list[0]);
        }
        else
            return nullptr;
    }

    // used by MWMechanics::PathgridGraph
    const ESM::Pathgrid *CellStore::getTES3Pathgrid() const
    {
        return &mPathgrid;
    }

    // FIXME: quick workaround to avoid rewriting code, however it is likely that new logic for
    //        connecting to roads and other external cells will be required eventually
    void CellStore::buildTES3Pathgrid()
    {
        std::vector<ESM4::FormId> list;
        mForeignPathgrids.listForeignIdentifier(list);
        if (!list.size())
            return;

        const ESM4::Pathgrid *pathgrid = mForeignPathgrids.search(list[0]);

        mPathgrid.mData.mX = static_cast<const MWWorld::ForeignCell*>(mCell)->getGridX();
        mPathgrid.mData.mY = static_cast<const MWWorld::ForeignCell*>(mCell)->getGridY();
        mPathgrid.mData.mS2 = pathgrid->mData;

        mPathgrid.mPoints.resize(pathgrid->mNodes.size());
        for (std::size_t i = 0; i < pathgrid->mNodes.size(); ++i)
        {
            mPathgrid.mPoints.at(i).mX = pathgrid->mNodes[i].x;
            mPathgrid.mPoints.at(i).mY = pathgrid->mNodes[i].y;
            mPathgrid.mPoints.at(i).mZ = pathgrid->mNodes[i].z;
            mPathgrid.mPoints.at(i).mAutogenerated = pathgrid->mNodes[i].priority;
            mPathgrid.mPoints.at(i).mConnectionNum = pathgrid->mNodes[i].numLinks;
        }

        mPathgrid.mEdges.resize(pathgrid->mLinks.size());
        for (std::size_t i = 0; i < pathgrid->mLinks.size(); ++i)
        {
            mPathgrid.mEdges.at(i).mV0 = pathgrid->mLinks[i].startNode;
            mPathgrid.mEdges.at(i).mV1 = pathgrid->mLinks[i].endNode;
        }
    }

    bool CellStore::isExterior() const
    {
        return mCell->isExterior();
    }

    Ptr CellStore::searchInContainer (const std::string& id)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (Ptr ptr = searchInContainerList (mContainers, id))
            return ptr;

        if (Ptr ptr = searchInContainerList (mCreatures, id))
            return ptr;

        if (Ptr ptr = searchInContainerList (mNpcs, id))
            return ptr;

        mHasState = oldState;

        return Ptr();
    }

    void CellStore::loadRef (ESM::CellRef& ref, bool deleted, const ESMStore& store)
    {
        Misc::StringUtils::lowerCaseInPlace (ref.mRefID);

        switch (store.find (ref.mRefID))
        {
            case ESM::REC_ACTI: mActivators.load(ref, deleted, store); break;
            case ESM::REC_ALCH: mPotions.load(ref, deleted,store); break;
            case ESM::REC_APPA: mAppas.load(ref, deleted, store); break;
            case ESM::REC_ARMO: mArmors.load(ref, deleted, store); break;
            case ESM::REC_BOOK: mBooks.load(ref, deleted, store); break;
            case ESM::REC_CLOT: mClothes.load(ref, deleted, store); break;
            case ESM::REC_CONT: mContainers.load(ref, deleted, store); break;
            case ESM::REC_CREA: mCreatures.load(ref, deleted, store); break;
            case ESM::REC_DOOR: mDoors.load(ref, deleted, store); break;
            case ESM::REC_INGR: mIngreds.load(ref, deleted, store); break;
            case ESM::REC_LEVC: mCreatureLists.load(ref, deleted, store); break;
            case ESM::REC_LEVI: mItemLists.load(ref, deleted, store); break;
            case ESM::REC_LIGH: mLights.load(ref, deleted, store); break;
            case ESM::REC_LOCK: mLockpicks.load(ref, deleted, store); break;
            case ESM::REC_MISC: mMiscItems.load(ref, deleted, store); break;
            case ESM::REC_NPC_: mNpcs.load(ref, deleted, store); break;
            case ESM::REC_PROB: mProbes.load(ref, deleted, store); break;
            case ESM::REC_REPA: mRepairs.load(ref, deleted, store); break;
            case ESM::REC_STAT: mStatics.load(ref, deleted, store); break;
            case ESM::REC_WEAP: mWeapons.load(ref, deleted, store); break;

            case 0: std::cerr << "Cell reference " + ref.mRefID + " not found!\n"; break;

            default:
                std::cerr
                    << "WARNING: Ignoring reference '" << ref.mRefID << "' of unhandled type\n";
        }
    }

    void CellStore::loadState (const ESM::CellState& state)
    {
        mHasState = true;

        if (mCell->mData.mFlags & ESM::Cell::Interior && mCell->mData.mFlags & ESM::Cell::HasWater)
            mWaterLevel = state.mWaterLevel;

        mWaterLevel = state.mWaterLevel;
        mLastRespawn = MWWorld::TimeStamp(state.mLastRespawn);
    }

    void CellStore::saveState (ESM::CellState& state) const
    {
        state.mId = mCell->getCellId();

        if (mCell->mData.mFlags & ESM::Cell::Interior && mCell->mData.mFlags & ESM::Cell::HasWater)
            state.mWaterLevel = mWaterLevel;

        state.mWaterLevel = mWaterLevel;
        state.mHasFogOfWar = (mFogState.get() ? 1 : 0);
        state.mLastRespawn = mLastRespawn.toEsm();
    }

    void CellStore::writeFog(ESM::ESMWriter &writer) const
    {
        if (mFogState.get())
        {
            mFogState->save(writer, mCell->mData.mFlags & ESM::Cell::Interior);
        }
    }

    void CellStore::readFog(ESM::ESMReader &reader)
    {
        mFogState.reset(new ESM::FogState());
        mFogState->load(reader);
    }

    void CellStore::writeReferences (ESM::ESMWriter& writer) const
    {
        writeReferenceCollection<ESM::ObjectState> (writer, mActivators);
        writeReferenceCollection<ESM::ObjectState> (writer, mPotions);
        writeReferenceCollection<ESM::ObjectState> (writer, mAppas);
        writeReferenceCollection<ESM::ObjectState> (writer, mArmors);
        writeReferenceCollection<ESM::ObjectState> (writer, mBooks);
        writeReferenceCollection<ESM::ObjectState> (writer, mClothes);
        writeReferenceCollection<ESM::ContainerState> (writer, mContainers);
        writeReferenceCollection<ESM::CreatureState> (writer, mCreatures);
        writeReferenceCollection<ESM::DoorState> (writer, mDoors);
        writeReferenceCollection<ESM::ObjectState> (writer, mIngreds);
        writeReferenceCollection<ESM::CreatureLevListState> (writer, mCreatureLists);
        writeReferenceCollection<ESM::ObjectState> (writer, mItemLists);
        writeReferenceCollection<ESM::ObjectState> (writer, mLights);
        writeReferenceCollection<ESM::ObjectState> (writer, mLockpicks);
        writeReferenceCollection<ESM::ObjectState> (writer, mMiscItems);
        writeReferenceCollection<ESM::NpcState> (writer, mNpcs);
        writeReferenceCollection<ESM::ObjectState> (writer, mProbes);
        writeReferenceCollection<ESM::ObjectState> (writer, mRepairs);
        writeReferenceCollection<ESM::ObjectState> (writer, mStatics);
        writeReferenceCollection<ESM::ObjectState> (writer, mWeapons);
    }

    void CellStore::readReferences (ESM::ESMReader& reader,
        const std::map<int, int>& contentFileMap)
    {
        mHasState = true;

        while (reader.isNextSub ("OBJE"))
        {
            unsigned int unused;
            reader.getHT (unused);

            // load the RefID first so we know what type of object it is
            ESM::CellRef cref;
            cref.loadId(reader, true);

            int type = MWBase::Environment::get().getWorld()->getStore().find(cref.mRefID);
            if (type == 0)
            {
                std::cerr << "Dropping reference to '" << cref.mRefID << "' (object no longer exists)" << std::endl;
                reader.skipHSubUntil("OBJE");
                continue;
            }

            switch (type)
            {
                case ESM::REC_ACTI:

                    readReferenceCollection<ESM::ObjectState> (reader, mActivators, cref, contentFileMap);
                    break;

                case ESM::REC_ALCH:

                    readReferenceCollection<ESM::ObjectState> (reader, mPotions, cref, contentFileMap);
                    break;

                case ESM::REC_APPA:

                    readReferenceCollection<ESM::ObjectState> (reader, mAppas, cref, contentFileMap);
                    break;

                case ESM::REC_ARMO:

                    readReferenceCollection<ESM::ObjectState> (reader, mArmors, cref, contentFileMap);
                    break;

                case ESM::REC_BOOK:

                    readReferenceCollection<ESM::ObjectState> (reader, mBooks, cref, contentFileMap);
                    break;

                case ESM::REC_CLOT:

                    readReferenceCollection<ESM::ObjectState> (reader, mClothes, cref, contentFileMap);
                    break;

                case ESM::REC_CONT:

                    readReferenceCollection<ESM::ContainerState> (reader, mContainers, cref, contentFileMap);
                    break;

                case ESM::REC_CREA:

                    readReferenceCollection<ESM::CreatureState> (reader, mCreatures, cref, contentFileMap);
                    break;

                case ESM::REC_DOOR:

                    readReferenceCollection<ESM::DoorState> (reader, mDoors, cref, contentFileMap);
                    break;

                case ESM::REC_INGR:

                    readReferenceCollection<ESM::ObjectState> (reader, mIngreds, cref, contentFileMap);
                    break;

                case ESM::REC_LEVC:

                    readReferenceCollection<ESM::CreatureLevListState> (reader, mCreatureLists, cref, contentFileMap);
                    break;

                case ESM::REC_LEVI:

                    readReferenceCollection<ESM::ObjectState> (reader, mItemLists, cref, contentFileMap);
                    break;

                case ESM::REC_LIGH:

                    readReferenceCollection<ESM::ObjectState> (reader, mLights, cref, contentFileMap);
                    break;

                case ESM::REC_LOCK:

                    readReferenceCollection<ESM::ObjectState> (reader, mLockpicks, cref, contentFileMap);
                    break;

                case ESM::REC_MISC:

                    readReferenceCollection<ESM::ObjectState> (reader, mMiscItems, cref, contentFileMap);
                    break;

                case ESM::REC_NPC_:

                    readReferenceCollection<ESM::NpcState> (reader, mNpcs, cref, contentFileMap);
                    break;

                case ESM::REC_PROB:

                    readReferenceCollection<ESM::ObjectState> (reader, mProbes, cref, contentFileMap);
                    break;

                case ESM::REC_REPA:

                    readReferenceCollection<ESM::ObjectState> (reader, mRepairs, cref, contentFileMap);
                    break;

                case ESM::REC_STAT:

                    readReferenceCollection<ESM::ObjectState> (reader, mStatics, cref, contentFileMap);
                    break;

                case ESM::REC_WEAP:

                    readReferenceCollection<ESM::ObjectState> (reader, mWeapons, cref, contentFileMap);
                    break;

                default:

                    throw std::runtime_error ("unknown type in cell reference section");
            }
        }
    }

    bool operator== (const CellStore& left, const CellStore& right)
    {
        return left.getCell()->getCellId()==right.getCell()->getCellId();
    }

    bool operator!= (const CellStore& left, const CellStore& right)
    {
        return !(left==right);
    }

    bool CellStore::isPointConnected(const int start, const int end) const
    {
        return mPathgridGraph.isPointConnected(start, end);
    }

    std::list<ESM::Pathgrid::Point> CellStore::aStarSearch(const int start, const int end) const
    {
        return mPathgridGraph.aStarSearch(start, end);
    }

    void CellStore::setFog(ESM::FogState *fog)
    {
        mFogState.reset(fog);
    }

    ESM::FogState* CellStore::getFog() const
    {
        return mFogState.get();
    }

    void CellStore::respawn()
    {
        if (mState == State_Loaded)
        {
            static int iMonthsToRespawn = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("iMonthsToRespawn")->getInt();
            if (MWBase::Environment::get().getWorld()->getTimeStamp() - mLastRespawn > 24*30*iMonthsToRespawn)
            {
                mLastRespawn = MWBase::Environment::get().getWorld()->getTimeStamp();
                for (CellRefList<ESM::Container>::List::iterator it (mContainers.mList.begin()); it!=mContainers.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
                for (CellRefList<ESM::Creature>::List::iterator it (mCreatures.mList.begin()); it!=mCreatures.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
                for (CellRefList<ESM::NPC>::List::iterator it (mNpcs.mList.begin()); it!=mNpcs.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
                for (CellRefList<ESM::CreatureLevList>::List::iterator it (mCreatureLists.mList.begin()); it!=mCreatureLists.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
            }
        }
    }
}
