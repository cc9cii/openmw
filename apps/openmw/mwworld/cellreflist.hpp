#ifndef GAME_MWWORLD_CELLREFLIST_H
#define GAME_MWWORLD_CELLREFLIST_H

#include <list>
#include <vector>
#include <map>
#include <stdexcept>

#include <extern/esm4/common.hpp> // Rec_Persistent, Rec_VisDistant

#include "livecellref.hpp"

namespace MWWorld
{
    enum SearchMask
    {
        SCH_Persistent = 0x10,
        SCH_VisDistant = 0x20,
        SCH_Temporary  = 0x40,
        SCH_All        = 0x70
    };

    struct CellRefStoreBase
    {
        //int mStoreType; // TODO: unused for now

        virtual ~CellRefStoreBase() {}

        virtual LiveCellRefBase *find(const ESM4::FormId formId, int mask = SCH_All)
        {
            return nullptr;
        }

        virtual LiveCellRefBase *find(const std::string& name) = 0;

        virtual std::vector<LiveCellRefBase*> search(std::int32_t x, std::int32_t y,
                std::size_t range = 0, std::size_t exclude = 0, int mask = SCH_All)
        {
            return std::vector<LiveCellRefBase*>(); // empty
        }

        virtual LiveCellRefBase *searchViaHandle(const std::string& handle) = 0;
    };

    /// \brief Collection of references of one type
    template <typename X>
    struct CellRefList : public CellRefStoreBase
    {
        typedef LiveCellRef<X> LiveRef;
        typedef std::list<LiveRef> List;
        List mList;

        typedef std::map<ESM4::FormId, LiveCellRefBase*> FormIdMap;
        FormIdMap mFormIdMap;

        std::vector<LiveCellRefBase*> mDummyActive;
        std::vector<LiveCellRefBase*> mDummyVisible;

        typedef std::map<std::pair<std::int32_t, std::int32_t>, std::vector<ESM4::FormId> > GridMap;
        GridMap mGridMap;

        // FIXME: a failed attempt to get rid of exceptions under linux
        ~CellRefList()
        {
            mGridMap.clear();
            mDummyVisible.clear();
            mDummyActive.clear();
            mFormIdMap.clear();
            // FIXME: neither of these work
            //mList.erase(mList.begin(), mList.end());
            //mList.clear();
        }

        /// Search for the given reference in the given reclist from
        /// ESMStore. Insert the reference into the list if a match is
        /// found. If not, throw an exception.
        /// Moved to cpp file, as we require a custom compare operator for it,
        /// and the build will fail with an ugly three-way cyclic header dependence
        /// so we need to pass the instantiation of the method to the linker, when
        /// all methods are known.
        void load (ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore);
        void load (ESM4::Reference &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy = false);
        void load (ESM4::ActorCreature &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy = false);
        void load (ESM4::ActorCharacter &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy = false);

        LiveCellRefBase *find(const ESM4::FormId formId, int mask = SCH_All)
        {
            FormIdMap::const_iterator iter = mFormIdMap.find(formId);
            if (iter == mFormIdMap.end())
                return nullptr;

            LiveCellRefBase *ref = iter->second;

            // TODO: keep deleted refs separately?
            // TODO: why return if hasContentFile()?
            if (!ref->mData.isDeletedByContentFile() &&
                (ref->mRef.hasContentFile() || ref->mData.getCount() > 0))
            {
                // TODO: allow both persistent and visibly distant at the same time?
                std::uint32_t flags = ref->mRef.getFlags();
                if ((mask == SCH_All) // all
                    ||
                    (mask == SCH_Persistent && ((flags & ESM4::Rec_Persistent) != 0 && // only persistent
                                                (flags & ESM4::Rec_VisDistant) == 0))
                    ||
                    (mask == SCH_VisDistant && ((flags & ESM4::Rec_Persistent) == 0 && // only vis distant
                                                (flags & ESM4::Rec_VisDistant) != 0))
                    ||
                    (mask == SCH_Temporary  && ((flags & ESM4::Rec_Persistent) == 0 && // only temporary
                                                (flags & ESM4::Rec_VisDistant) == 0)))
                {
                    return ref;
                }
            }

            return nullptr;
        }

        LiveCellRefBase *find(const std::string& name)
        {
            for (typename List::iterator iter (mList.begin()); iter!=mList.end(); ++iter)
                if (!iter->mData.isDeletedByContentFile()
                        && (iter->mRef.hasContentFile() || iter->mData.getCount() > 0)
                        && iter->mRef.getRefId() == name)
                {
                    return &*iter;
                }

            return nullptr;
        }

        std::vector<LiveCellRefBase*> search(std::int32_t x, std::int32_t y,
                std::size_t range = 0, std::size_t exclude = 0, int mask = SCH_All)
        {
            if (exclude > range)
                throw std::logic_error("The excluded area is larger than the included range.");

            std::vector<LiveCellRefBase*> res;

            for (std::int32_t i = x - std::int32_t(range); i <= x + std::int32_t(range); ++i)
            {
                for (std::int32_t j = y - std::int32_t(range); j <= y + std::int32_t(range); ++j)
                {
#if 0
                    std::int32_t distSq = (x - i) * (x - i) + (y - j) * (y - j);
                    if (exclude != 0 && distSq < exclude)
                        continue;
#else
                    if ((int)exclude != 0 && i >= x - (int)exclude && i <= x + (int)exclude)
                        continue;

                    if ((int)exclude != 0 && j >= y - (int)exclude && j <= y + (int)exclude)
                        continue;
#endif

                    GridMap::const_iterator iter = mGridMap.find(std::make_pair(i, j));

                    if (iter != mGridMap.end())
                    {
                        for (std::size_t k = 0; k < iter->second.size(); ++k)
                        {
                            // find() may return a nullptr after checking deleted, count etc;
                            // count is set to 0 when an actor in a dummy cell gets moved to another
                            // - see World::moveObject()
                            LiveCellRefBase* ref = find(iter->second[k], mask);
                            if (ref)
                                res.push_back(ref);
                        }
                    }
                }
            }

            return res;
        }

        // used by copyToCellImpl()
        LiveCellRefBase *insert(const LiveRef &item)
        {
            ESM4::FormId formId = item.mRef.getFormId();

            mList.push_back(item);

            LiveCellRefBase *refPtr = &mList.back();
            mFormIdMap[formId] = refPtr;

            // WARN: assumed that it is not possible to insert records to a dummy cell,
            //       hence not updating GridMap

            return refPtr;
        }

        // this method is looking for Ogre::SceneNode handles which is created in
        // Objects::insertBegin() and Actors::insertBegin()
        //
        // unfortunately the handle map needs to be manually maintained when objects and actors
        // move to different cells - see World::copyObjectToCell() and World::moveObject()
        LiveCellRefBase *searchViaHandle(const std::string& handle)
        {
            for (typename List::iterator iter (mList.begin()); iter != mList.end(); ++iter)
                if (iter->mData.getBaseNode() &&
                    iter->mData.getHandle() == handle)
                    return &*iter;

            return nullptr;
        }
    };
}

#endif
