#ifndef OPENMW_MWWORLD_FOREIGNSTORE_H
#define OPENMW_MWWORLD_FOREIGNSTORE_H

#include <string>
#include <vector>
#include <map>

#include <components/loadinglistener/loadinglistener.hpp>

#include <extern/esm4/qust.hpp>
#include <extern/esm4/road.hpp>

#include "storebase.hpp"

#include "foreignworld.hpp"
#include "foreigncell.hpp"
#include "foreignland.hpp"
#include "foreigndialogue.hpp"

namespace ESM4
{
    class Reader;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore;

    struct ForeignId
    {
        ESM4::FormId mId;
        bool mIsDeleted;

        ForeignId(ESM4::FormId id = 0, bool isDeleted = false);
    };

    // TODO: not so sure if we needed to inherit from StoreBase;
    //       we could probably define another base class instead
    template <class T>
    class ForeignStore : public StoreBase
    {
        std::map<ESM4::FormId, T>      mStatic;
        std::vector<T *>    mShared; // Preserves the record order as it came from the content files (this
                                     // is relevant for the spell autocalc code and selection order
                                     // for heads/hairs in the character creation)
        std::map<ESM4::FormId, T> mDynamic; // probably for saved games

        typedef std::map<ESM4::FormId, T> Dynamic;
        typedef std::map<ESM4::FormId, T> Static;

    public:
        ForeignStore();
        ForeignStore(const ForeignStore<T>& orig);

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        virtual void clearDynamic();
        void setUp();

        const T *search(const std::string& id) const; // search EditorId
        const T *searchLower(const std::string &id) const; // search EditorId, convert lowercase first

        const T *search(ESM4::FormId id) const; // search BaseObj or DIAL

        /**
         * Does the record with this ID come from the dynamic store?
         */
        bool isDynamic(const std::string& id) const;

        // TODO: seems to be only used for TES3 werewolf related stuff
        const T *searchRandom(const std::string& id) const { return nullptr; }

        const T *find(const std::string& id) const;

        // TODO: seems to be only used for TES3 werewolf related stuff (always throws an exception)
        const T *findRandom(const std::string& id) const;

        iterator begin() const;
        iterator end() const;

        size_t getSize() const;
        int getDynamicSize() const;

        /// @note The record identifiers are listed in the order that the records were defined by the content files.
        virtual void listIdentifier(std::vector<std::string>& list) const;

        void listForeignIdentifier(std::vector<ESM4::FormId>& list) const;

        T *insert(const T& item);
        T *insertStatic(const T& item);

        virtual bool eraseStatic(const std::string& id);
        virtual bool erase(const std::string& id);
        bool erase(ESM4::FormId formId);
        virtual bool erase(const T& item);

        virtual RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);
        ForeignId loadForeign(T& record);

        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        ///< Read into dynamic storage
        // seems to be used only by ESMStore::readRecord, called by World::readRecord
        // which is called by StateManager for loading a save file
        virtual RecordId read(ESM::ESMReader& reader);
    };

    // FIXME: only one of either TES4 or TES5 allowed (else FormId's may clash)
    //
    // One option might be to use a 64bit version of formid to identify which game,
    // which can also allow more than 255 mods (will need to use some hack such as detecting
    // the base game from the header dependency/master lists)

    template <>
    class ForeignStore<ForeignWorld> : public StoreBase
    {
    private:

        std::map<ESM4::FormId, ForeignWorld*> mWorlds;

    public:

        virtual ~ForeignStore();

        // Would like to make it const, but ForeignStore<ForeignCell> needs to update
        ForeignWorld *getWorld(ESM4::FormId worldId);

        const ForeignWorld *find(ESM4::FormId worldId) const;
        const ForeignWorld *search(ESM4::FormId worldId) const;

        // Assumes editorId to be lower case.
        const ForeignWorld *search(const std::string& editorId) const; // FIXME: deprecated

        // Returns 0 if not found. Does not assume editorId to be lower case.
        ESM4::FormId getFormId(const std::string& editorId) const;

        size_t getSize() const;

        // FIXME: need to overload eraseStatic()?

        RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);
        //void setUp();
    };

    template <>
    class ForeignStore<MWWorld::ForeignCell> : public StoreBase
    {
    private:

        // the key is formid adjusted with modindex
        std::map<ESM4::FormId, ForeignCell*> mCells;

        std::map<std::string, ESM4::FormId> mEditorIdMap;

        std::map<ESM4::FormId, std::vector<ESM4::FormId> > mWorldCellsMap;

        ESM4::FormId mLastPreloadedCell;       // FIXME for testing only

    public:

        virtual ~ForeignStore();

        // probably need some search functions here
        // also utilities e.g. get formId based on EditorId/FullName

        // Used by World::findForeignWorldPosition for teleporting the player, e.g. from
        // console command COC (center on cell)
        const MWWorld::ForeignCell *searchExtByName(const std::string &name) const;

        size_t getSize() const;

        void preload(ESM::ESMReader& esm, ForeignStore<ForeignWorld>& worlds);
        void loadVisibleDist(ESMStore& store, ESM::ESMReader& esm, CellStore *cell);

        void loadTes4Group(ESMStore& store, ESM::ESMReader& esm, CellStore *cell);
        //void loadTes4Record(ESM::ESMReader& esm, ForeignStore<ForeignWorld>& worlds);
        void updateRefrEstimate(ESM::ESMReader& esm);
        void incrementRefrCount(ESM::ESMReader& esm);

        RecordId load(ESM::ESMReader& esm) { return RecordId("", false); } // noop
        RecordId load(ESM::ESMReader& esm, ForeignStore<ForeignWorld>& worlds);
        //void setUp();

        const ForeignCell *search(ESM4::FormId world, int x, int y) const;
        const ForeignCell *search(ESM4::FormId formId) const;
        const ForeignCell *search(const std::string& name) const;

        void testPreload(ESM::ESMReader& esm); // FIXME for testing only
    };

    template <>
    class ForeignStore<ForeignLand> : public StoreBase
    {
    private:

        std::map<ESM4::FormId, ForeignLand*> mLands;

    public:

        virtual ~ForeignStore();

        size_t getSize() const;

        RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);
        //void setUp();

        const ForeignLand *search(ESM4::FormId formId) const;

        // below currently unused
        //ForeignLand *find(ESM4::FormId worldId, int x, int y) const;
        //ForeignLand *search(ESM4::FormId worldId, int x, int y) const;
    };

    template <>
    class ForeignStore<ForeignDialogue> : public StoreBase
    {
    private:

        std::vector<ForeignDialogue*> mDialogues;
        std::map<ESM4::FormId, std::size_t> mFormIdMap;
        std::map<std::string, std::size_t> mTopicMap;

    public:

        virtual ~ForeignStore();

        size_t getSize() const;

        RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);
        //void setUp();

        const ForeignDialogue *find(ESM4::FormId formId) const;
        const ForeignDialogue *search(ESM4::FormId formId) const;

        const ForeignDialogue *search(const std::string& topic) const;
    };

    template <>
    class ForeignStore<ESM4::Quest> : public StoreBase
    {
    private:

        std::vector<ESM4::Quest*> mQuests;
        std::map<ESM4::FormId, std::size_t> mFormIdMap;
        std::map<ESM4::FormId, std::size_t> mConditionMap;
        std::map<std::string, std::size_t> mTopicMap;

    public:

        virtual ~ForeignStore();

        std::vector<ESM4::Quest*>::const_iterator begin() const;
        std::vector<ESM4::Quest*>::const_iterator end() const;

        size_t getSize() const;

        RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);
        //void setUp();

        const ESM4::Quest *find(ESM4::FormId formId) const;
        const ESM4::Quest *search(ESM4::FormId formId) const;

        const ESM4::Quest *searchCondition(ESM4::FormId formId) const;

        const ESM4::Quest *search(const std::string& quest) const;
    };


    template <>
    class ForeignStore<ESM4::Road> : public StoreBase
    {
    private:

        std::vector<ESM4::Road*> mRoads;
        std::map<ESM4::FormId, std::size_t> mFormIdMap;

    public:

        virtual ~ForeignStore();

        size_t getSize() const;

        RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);

        const ESM4::Road *find(ESM4::FormId formId) const;
        const ESM4::Road *search(ESM4::FormId formId) const;

        const ESM4::Road *searchWorld(ESM4::FormId formId) const;
    };

} //end namespace

#endif
