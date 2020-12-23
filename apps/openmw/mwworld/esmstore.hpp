#ifndef OPENMW_MWWORLD_ESMSTORE_H
#define OPENMW_MWWORLD_ESMSTORE_H

#include <stdexcept>

#include <components/esm/records.hpp>

#include <extern/esm4/records.hpp>

#include "store.hpp"
#include "foreignstore.hpp"

namespace ESM4
{
    class Reader;
    union RecordHeader;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore
    {
        Store<ESM::Activator>       mActivators;
        Store<ESM::Potion>          mPotions;
        Store<ESM::Apparatus>       mAppas;
        Store<ESM::Armor>           mArmors;
        Store<ESM::BodyPart>        mBodyParts;
        Store<ESM::Book>            mBooks;
        Store<ESM::BirthSign>       mBirthSigns;
        Store<ESM::Class>           mClasses;
        Store<ESM::Clothing>        mClothes;
        Store<ESM::Container>       mContainers;
        Store<ESM::Creature>        mCreatures;
        Store<ESM::Dialogue>        mDialogs;
        Store<ESM::Door>            mDoors;
        Store<ESM::Enchantment>     mEnchants;
        Store<ESM::Faction>         mFactions;
        Store<ESM::Global>          mGlobals;
        Store<ESM::Ingredient>      mIngreds;
        Store<ESM::CreatureLevList> mCreatureLists;
        Store<ESM::ItemLevList>     mItemLists;
        Store<ESM::Light>           mLights;
        Store<ESM::Lockpick>        mLockpicks;
        Store<ESM::Miscellaneous>   mMiscItems;
        Store<ESM::NPC>             mNpcs;
        Store<ESM::Probe>           mProbes;
        Store<ESM::Race>            mRaces;
        Store<ESM::Region>          mRegions;
        Store<ESM::Repair>          mRepairs;
        Store<ESM::SoundGenerator>  mSoundGens;
        Store<ESM::Sound>           mSounds;
        Store<ESM::Spell>           mSpells;
        Store<ESM::StartScript>     mStartScripts;
        Store<ESM::Static>          mStatics;
        Store<ESM::Weapon>          mWeapons;

        Store<ESM::GameSetting>     mGameSettings;
        Store<ESM::Script>          mScripts;

        // Lists that need special rules
        Store<ESM::Cell>        mCells;
        Store<ESM::Land>        mLands;
        Store<ESM::LandTexture> mLandTextures;
        Store<ESM::Pathgrid>    mPathgrids;

        Store<ESM::MagicEffect> mMagicEffects;
        Store<ESM::Skill>       mSkills;

        // Special entry which is hardcoded and not loaded from an ESM
        Store<ESM::Attribute>   mAttributes;

        // Lists that are foreign
        ForeignStore<MWWorld::ForeignWorld> mForeignWorlds;
        ForeignStore<MWWorld::ForeignCell>  mForeignCells;
        ForeignStore<MWWorld::ForeignLand>  mForeignLands;
        ForeignStore<MWWorld::ForeignDialogue> mForeignDialogues;
        //
        ForeignStore<ESM4::GlobalVariable> mForeignGlobals;
        ForeignStore<ESM4::Hair>       mForeignHairs;
        ForeignStore<ESM4::Eyes>       mForeignEyesSet;
        ForeignStore<ESM4::Race>       mForeignRaces;
        ForeignStore<ESM4::BodyPartData> mForeignBodyParts;
        ForeignStore<ESM4::HeadPart>   mHeadParts;
        ForeignStore<ESM4::ActorCharacter> mForeignACharacters;
        ForeignStore<ESM4::ActorCreature> mForeignACreatures;
        ForeignStore<ESM4::LandTexture> mForeignLandTextures;
        ForeignStore<ESM4::Script>     mForeignScripts;
        ForeignStore<ESM4::Quest>      mForeignQuests;
        ForeignStore<ESM4::AIPackage>  mForeignAIPackages;
        ForeignStore<ESM4::AnimObject> mForeignAnimObjs;
        ForeignStore<ESM4::Region>     mForeignRegions;
        ForeignStore<ESM4::PlacedGrenade> mPlacedGrenades;
        ForeignStore<ESM4::LightingTemplate> mLightingTemplates;
        ForeignStore<ESM4::Music>      mMusic;
        ForeignStore<ESM4::MediaSet>   mMediaSet;
        ForeignStore<ESM4::MediaLocationController> mMediaLocCtlr;
        ForeignStore<ESM4::DefaultObj> mDefaultObj;
        // Foreign referenceables
        ForeignStore<ESM4::Sound>      mForeignSounds;
        ForeignStore<ESM4::Activator>  mForeignActivators;
        ForeignStore<ESM4::Apparatus>  mForeignApparatuses;
        ForeignStore<ESM4::Armor>      mForeignArmors;
        ForeignStore<ESM4::Book>       mForeignBooks;
        ForeignStore<ESM4::Clothing>   mForeignClothes;
        ForeignStore<ESM4::Container>  mForeignContainers;
        ForeignStore<ESM4::Door>       mForeignDoors;
        ForeignStore<ESM4::Ingredient> mForeignIngredients;
        ForeignStore<ESM4::Light>      mForeignLights;
        ForeignStore<ESM4::MiscItem>   mForeignMiscItems;
        ForeignStore<ESM4::Static>     mForeignStatics;
        ForeignStore<ESM4::Grass>      mForeignGrasses;
        ForeignStore<ESM4::Tree>       mForeignTrees;
        ForeignStore<ESM4::Flora>      mForeignFloras;
        ForeignStore<ESM4::Furniture>  mForeignFurnitures;
        ForeignStore<ESM4::Weapon>     mForeignWeapons;
        ForeignStore<ESM4::Ammunition> mAmmunitions;
        ForeignStore<ESM4::Npc>        mForeignNpcs;
        ForeignStore<ESM4::Creature>   mForeignCreatures;
        ForeignStore<ESM4::LevelledCreature> mLevelledCreatures;
        ForeignStore<ESM4::SoulGem>    mSoulGems;
        ForeignStore<ESM4::Key>        mForeignKeys;
        ForeignStore<ESM4::Potion>     mForeignPotions;
        ForeignStore<ESM4::SubSpace>   mSubSpaces;
        ForeignStore<ESM4::SigilStone> mSigilStones;
        ForeignStore<ESM4::LevelledItem> mLevelledItems;
        ForeignStore<ESM4::LevelledNpc> mLevelledNpcs;
        ForeignStore<ESM4::IdleMarker> mIdleMarkers;
        ForeignStore<ESM4::MovableStatic> mMovableStatics;
        ForeignStore<ESM4::TextureSet> mTextureSets;
        ForeignStore<ESM4::Scroll>     mForeignScrolls;
        ForeignStore<ESM4::ArmorAddon> mArmorAddons;
        ForeignStore<ESM4::Terminal>   mTerminals;
        ForeignStore<ESM4::TalkingActivator> mTalkingActivators;
        ForeignStore<ESM4::Note>       mNotes;
        ForeignStore<ESM4::AcousticSpace> mAcousticSpaces;
        ForeignStore<ESM4::ItemMod>    mItemMods;
        ForeignStore<ESM4::PlaceableWater> mPlaceableWaters;
        ForeignStore<ESM4::StaticCollection> mStaticCollections;
        ForeignStore<ESM4::SoundReference> mSoundReferences;
        ForeignStore<ESM4::Outfit>     mOutfits;

        // Lookup of all IDs. Makes looking up references faster. Just
        // maps the id name to the record type.
        std::map<std::string, int> mIds;
        std::map<ESM4::FormId, int> mForeignIds;

        std::map<int, StoreBase *> mStores;
        std::map<int, StoreBase *> mForeignStores;

        // Unlike TES3, the destination cell is not specified in the reference record.
        // Need a lookup map to work around this issue.
        // FIXME: there must be another way
        mutable std::map<ESM4::FormId, ESM4::FormId> mDoorDestCell;
        // INFO records follow DIAL
        ForeignDialogue *mCurrentDialogue;

        ESM::NPC mPlayerTemplate;

        unsigned int mDynamicCount;

        void loadTes4Group (ESM::ESMReader& esm);
        void loadTes4Record (ESM::ESMReader& esm);

    public:
        /// \todo replace with SharedIterator<StoreBase>
        typedef std::map<int, StoreBase *>::const_iterator iterator;

        iterator begin() const {
            return mStores.begin();
        }

        iterator end() const {
            return mStores.end();
        }

        /// Look up the given ID in 'all'. Returns 0 if not found.
        /// \note id must be in lower case.
        int find(const std::string &id) const
        {
            std::map<std::string, int>::const_iterator it = mIds.find(id);
            if (it == mIds.end()) {
                return 0;
            }
            return it->second;
        }

        // NOTE: returns TES4::RecordTypes
        int getRecordType(ESM4::FormId formId) const
        {
            std::map<ESM4::FormId, int>::const_iterator it = mForeignIds.find(formId);
            if (it == mForeignIds.end())
                return 0;
            else
                return it->second;
        }

        ESM4::FormId getDoorCellId(ESM4::FormId doorId) const
        {
            std::map<ESM4::FormId, ESM4::FormId>::const_iterator it
                = mDoorDestCell.find(doorId);

            if (it != mDoorDestCell.end())
                return it->second;

            return 0;
        }

        bool setDoorCell(ESM4::FormId doorId, ESM4::FormId cellId) const
        {
            std::pair<std::map<ESM4::FormId, ESM4::FormId>::iterator, bool> result
                = mDoorDestCell.insert({ doorId, cellId }); // mutable

            return result.second;
        }

        ESMStore()
          : mCurrentDialogue(0), mDynamicCount(0)
        {
            mStores[ESM::REC_ACTI] = &mActivators;
            mStores[ESM::REC_ALCH] = &mPotions;
            mStores[ESM::REC_APPA] = &mAppas;
            mStores[ESM::REC_ARMO] = &mArmors;
            mStores[ESM::REC_BODY] = &mBodyParts;
            mStores[ESM::REC_BOOK] = &mBooks;
            mStores[ESM::REC_BSGN] = &mBirthSigns;
            mStores[ESM::REC_CELL] = &mCells;
            mStores[ESM::REC_CLAS] = &mClasses;
            mStores[ESM::REC_CLOT] = &mClothes;
            mStores[ESM::REC_CONT] = &mContainers;
            mStores[ESM::REC_CREA] = &mCreatures;
            mStores[ESM::REC_DIAL] = &mDialogs;
            mStores[ESM::REC_DOOR] = &mDoors;
            mStores[ESM::REC_ENCH] = &mEnchants;
            mStores[ESM::REC_FACT] = &mFactions;
            mStores[ESM::REC_GLOB] = &mGlobals;
            mStores[ESM::REC_GMST] = &mGameSettings;
            mStores[ESM::REC_INGR] = &mIngreds;
            mStores[ESM::REC_LAND] = &mLands;
            mStores[ESM::REC_LEVC] = &mCreatureLists;
            mStores[ESM::REC_LEVI] = &mItemLists;
            mStores[ESM::REC_LIGH] = &mLights;
            mStores[ESM::REC_LOCK] = &mLockpicks;
            mStores[ESM::REC_LTEX] = &mLandTextures;
            mStores[ESM::REC_MISC] = &mMiscItems;
            mStores[ESM::REC_NPC_] = &mNpcs;
            mStores[ESM::REC_PGRD] = &mPathgrids;
            mStores[ESM::REC_PROB] = &mProbes;
            mStores[ESM::REC_RACE] = &mRaces;
            mStores[ESM::REC_REGN] = &mRegions;
            mStores[ESM::REC_REPA] = &mRepairs;
            mStores[ESM::REC_SCPT] = &mScripts;
            mStores[ESM::REC_SNDG] = &mSoundGens;
            mStores[ESM::REC_SOUN] = &mSounds;
            mStores[ESM::REC_SPEL] = &mSpells;
            mStores[ESM::REC_SSCR] = &mStartScripts;
            mStores[ESM::REC_STAT] = &mStatics;
            mStores[ESM::REC_WEAP] = &mWeapons;

            // have a separate store so that there is no key clash
            mForeignStores[ESM4::REC_GLOB] = &mForeignGlobals;
            mForeignStores[ESM4::REC_HAIR] = &mForeignHairs;
            mForeignStores[ESM4::REC_EYES] = &mForeignEyesSet;
            mForeignStores[ESM4::REC_RACE] = &mForeignRaces;
            mForeignStores[ESM4::REC_BPTD] = &mForeignBodyParts;
            mForeignStores[ESM4::REC_HDPT] = &mHeadParts;
            mForeignStores[ESM4::REC_ACHR] = &mForeignACharacters;
            mForeignStores[ESM4::REC_ACRE] = &mForeignACreatures;
            mForeignStores[ESM4::REC_LTEX] = &mForeignLandTextures;
            mForeignStores[ESM4::REC_SCPT] = &mForeignScripts;
            mForeignStores[ESM4::REC_DIAL] = &mForeignDialogues;
            mForeignStores[ESM4::REC_QUST] = &mForeignQuests;
            mForeignStores[ESM4::REC_PACK] = &mForeignAIPackages;
            mForeignStores[ESM4::REC_PGRE] = &mPlacedGrenades;
            mForeignStores[ESM4::REC_LGTM] = &mLightingTemplates;
            mForeignStores[ESM4::REC_MUSC] = &mMusic;
            mForeignStores[ESM4::REC_MSET] = &mMediaSet;
            mForeignStores[ESM4::REC_ALOC] = &mMediaLocCtlr;
            mForeignStores[ESM4::REC_DOBJ] = &mDefaultObj;
            //
            mForeignStores[ESM4::REC_SOUN] = &mForeignSounds;
            mForeignStores[ESM4::REC_ACTI] = &mForeignActivators;
            mForeignStores[ESM4::REC_APPA] = &mForeignApparatuses;
            mForeignStores[ESM4::REC_ARMO] = &mForeignArmors;
            mForeignStores[ESM4::REC_BOOK] = &mForeignBooks;
            mForeignStores[ESM4::REC_CLOT] = &mForeignClothes;
            mForeignStores[ESM4::REC_CONT] = &mForeignContainers;
            mForeignStores[ESM4::REC_DOOR] = &mForeignDoors;
            mForeignStores[ESM4::REC_INGR] = &mForeignIngredients;
            mForeignStores[ESM4::REC_LIGH] = &mForeignLights;
            mForeignStores[ESM4::REC_MISC] = &mForeignMiscItems;
            mForeignStores[ESM4::REC_STAT] = &mForeignStatics;
            mForeignStores[ESM4::REC_GRAS] = &mForeignGrasses;
            mForeignStores[ESM4::REC_TREE] = &mForeignTrees;
            mForeignStores[ESM4::REC_FLOR] = &mForeignFloras;
            mForeignStores[ESM4::REC_FURN] = &mForeignFurnitures;
            mForeignStores[ESM4::REC_WEAP] = &mForeignWeapons;
            mForeignStores[ESM4::REC_AMMO] = &mAmmunitions;
            mForeignStores[ESM4::REC_NPC_] = &mForeignNpcs;
            mForeignStores[ESM4::REC_CREA] = &mForeignCreatures;
            mForeignStores[ESM4::REC_LVLC] = &mLevelledCreatures;
            mForeignStores[ESM4::REC_SLGM] = &mSoulGems;
            mForeignStores[ESM4::REC_KEYM] = &mForeignKeys;
            mForeignStores[ESM4::REC_ALCH] = &mForeignPotions;
            mForeignStores[ESM4::REC_SBSP] = &mSubSpaces;
            mForeignStores[ESM4::REC_SGST] = &mSigilStones;
            mForeignStores[ESM4::REC_LVLI] = &mLevelledItems;
            mForeignStores[ESM4::REC_LVLN] = &mLevelledNpcs;
            mForeignStores[ESM4::REC_IDLM] = &mIdleMarkers;
            mForeignStores[ESM4::REC_MSTT] = &mMovableStatics;
            mForeignStores[ESM4::REC_TXST] = &mTextureSets;
            mForeignStores[ESM4::REC_SCRL] = &mForeignScrolls;
            mForeignStores[ESM4::REC_ARMA] = &mArmorAddons;
            mForeignStores[ESM4::REC_TERM] = &mTerminals;
            mForeignStores[ESM4::REC_TACT] = &mTalkingActivators;
            mForeignStores[ESM4::REC_NOTE] = &mNotes;
            mForeignStores[ESM4::REC_ASPC] = &mAcousticSpaces;
            mForeignStores[ESM4::REC_IMOD] = &mItemMods;
            mForeignStores[ESM4::REC_PWAT] = &mPlaceableWaters;
            mForeignStores[ESM4::REC_SCOL] = &mStaticCollections;
          //mForeignStores[ESM4::REC_CCRD] = &mCaravanCard;
          //mForeignStores[ESM4::REC_CMNY] = &mCaravanMoney;
            mForeignStores[ESM4::REC_SNDR] = &mSoundReferences;
            mForeignStores[ESM4::REC_OTFT] = &mOutfits;

            mForeignStores[ESM4::REC_ANIO] = &mForeignAnimObjs;

            // FIXME: do we need these or only referenceables?
          //mForeignStores[ESM4::REC_WRLD] = &mForeignWorlds;
          //mForeignStores[ESM4::REC_CELL] = &mForeignCells;
          //mForeignStores[ESM4::REC_LAND] = &mForeignLands;
          //mForeignStores[ESM4::REC_REGN] = &mForeignRegions;

            mPathgrids.setCells(mCells);
            mForeignIds.clear();
        }

        void clearDynamic ()
        {
            for (std::map<int, StoreBase *>::iterator it = mStores.begin(); it != mStores.end(); ++it)
                it->second->clearDynamic();

            for (std::map<int, StoreBase *>::iterator it = mForeignStores.begin(); it != mForeignStores.end(); ++it)
                it->second->clearDynamic();

            mNpcs.insert(mPlayerTemplate);
        }

        void movePlayerRecord ()
        {
            mPlayerTemplate = *mNpcs.find("player");
            mNpcs.eraseStatic(mPlayerTemplate.mId);
            mNpcs.insert(mPlayerTemplate);
        }

        void load(ESM::ESMReader &esm, Loading::Listener* listener);

        template <class T>
        const Store<T> &get() const {
            throw std::runtime_error("Storage for this type not exist");
        }

        template <class T>
        Store<T> &getModifiable() {
            throw std::runtime_error("Storage for this type not exist");
        }

        template <class T>
        const ForeignStore<T> &getForeign() const {
            throw std::runtime_error("Storage for this foreign type not exist");
        }

        template <class T>
        ForeignStore<T> &getForeignModifiable() {
            throw std::runtime_error("Storage for this foreign type not exist");
        }

        /// Insert a custom record (i.e. with a generated ID that will not clash will pre-existing records)
        // FIXME: used by WorldImp, we probably need an equivalent for TES4, etc
        template <class T>
        const T *insert(const T &x) {
            std::ostringstream id;
            id << "$dynamic" << mDynamicCount++;

            Store<T> &store = const_cast<Store<T> &>(get<T>());
            if (store.search(id.str()) != 0) {
                std::ostringstream msg;
                msg << "Try to override existing record '" << id.str() << "'";
                throw std::runtime_error(msg.str());
            }
            T record = x;

            record.mId = id.str();

            T *ptr = store.insert(record);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        /// Insert a record with set ID, and allow it to override a pre-existing static record.
        // FIXME: used by TES3 LVLC and LVLI in WorldImp, we probably need an equivalent for TES4, etc
        template <class T>
        const T *overrideRecord(const T &x) {
            Store<T> &store = const_cast<Store<T> &>(get<T>());

            T *ptr = store.insert(x);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        // FIXME: used by TES3 GMST and GLOB in WorldImp, we probably need an equivalent for TES4, etc
        template <class T>
        const T *insertStatic(const T &x) {
            std::ostringstream id;
            id << "$dynamic" << mDynamicCount++;

            Store<T> &store = const_cast<Store<T> &>(get<T>());
            if (store.search(id.str()) != 0) {
                std::ostringstream msg;
                msg << "Try to override existing record '" << id.str() << "'";
                throw std::runtime_error(msg.str());
            }
            T record = x;

            T *ptr = store.insertStatic(record);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        // This method must be called once, after loading all master/plugin files. This can only be done
        //  from the outside, so it must be public.
        void setUp();

        int countSavedGameRecords() const;

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord (ESM::ESMReader& reader, uint32_t type);
        ///< \return Known type?
    };

    template <>
    inline const ESM::Cell *ESMStore::insert<ESM::Cell>(const ESM::Cell &cell) {
        return mCells.insert(cell);
    }

    template <>
    inline const ESM::NPC *ESMStore::insert<ESM::NPC>(const ESM::NPC &npc) {
        std::ostringstream id;
        id << "$dynamic" << mDynamicCount++;

        if (Misc::StringUtils::ciEqual(npc.mId, "player")) {
            return mNpcs.insert(npc);
        } else if (mNpcs.search(id.str()) != 0) {
            std::ostringstream msg;
            msg << "Try to override existing record '" << id.str() << "'";
            throw std::runtime_error(msg.str());
        }
        ESM::NPC record = npc;

        record.mId = id.str();

        ESM::NPC *ptr = mNpcs.insert(record);
        mIds[ptr->mId] = ESM::REC_NPC_;
        return ptr;
    }

    template <>
    inline const Store<ESM::Activator> &ESMStore::get<ESM::Activator>() const {
        return mActivators;
    }

    template <>
    inline const Store<ESM::Potion> &ESMStore::get<ESM::Potion>() const {
        return mPotions;
    }

    template <>
    inline const Store<ESM::Apparatus> &ESMStore::get<ESM::Apparatus>() const {
        return mAppas;
    }

    template <>
    inline const Store<ESM::Armor> &ESMStore::get<ESM::Armor>() const {
        return mArmors;
    }

    template <>
    inline const Store<ESM::BodyPart> &ESMStore::get<ESM::BodyPart>() const {
        return mBodyParts;
    }

    template <>
    inline const Store<ESM::Book> &ESMStore::get<ESM::Book>() const {
        return mBooks;
    }

    template <>
    inline const Store<ESM::BirthSign> &ESMStore::get<ESM::BirthSign>() const {
        return mBirthSigns;
    }

    template <>
    inline const Store<ESM::Class> &ESMStore::get<ESM::Class>() const {
        return mClasses;
    }

    template <>
    inline const Store<ESM::Clothing> &ESMStore::get<ESM::Clothing>() const {
        return mClothes;
    }

    template <>
    inline const Store<ESM::Container> &ESMStore::get<ESM::Container>() const {
        return mContainers;
    }

    template <>
    inline const Store<ESM::Creature> &ESMStore::get<ESM::Creature>() const {
        return mCreatures;
    }

    template <>
    inline const Store<ESM::Dialogue> &ESMStore::get<ESM::Dialogue>() const {
        return mDialogs;
    }

    template <>
    inline const Store<ESM::Door> &ESMStore::get<ESM::Door>() const {
        return mDoors;
    }

    template <>
    inline const Store<ESM::Enchantment> &ESMStore::get<ESM::Enchantment>() const {
        return mEnchants;
    }

    template <>
    inline const Store<ESM::Faction> &ESMStore::get<ESM::Faction>() const {
        return mFactions;
    }

    template <>
    inline const Store<ESM::Global> &ESMStore::get<ESM::Global>() const {
        return mGlobals;
    }

    template <>
    inline const Store<ESM::Ingredient> &ESMStore::get<ESM::Ingredient>() const {
        return mIngreds;
    }

    template <>
    inline const Store<ESM::CreatureLevList> &ESMStore::get<ESM::CreatureLevList>() const {
        return mCreatureLists;
    }

    template <>
    inline const Store<ESM::ItemLevList> &ESMStore::get<ESM::ItemLevList>() const {
        return mItemLists;
    }

    template <>
    inline const Store<ESM::Light> &ESMStore::get<ESM::Light>() const {
        return mLights;
    }

    template <>
    inline const Store<ESM::Lockpick> &ESMStore::get<ESM::Lockpick>() const {
        return mLockpicks;
    }

    template <>
    inline const Store<ESM::Miscellaneous> &ESMStore::get<ESM::Miscellaneous>() const {
        return mMiscItems;
    }

    template <>
    inline const Store<ESM::NPC> &ESMStore::get<ESM::NPC>() const {
        return mNpcs;
    }

    template <>
    inline const Store<ESM::Probe> &ESMStore::get<ESM::Probe>() const {
        return mProbes;
    }

    template <>
    inline const Store<ESM::Race> &ESMStore::get<ESM::Race>() const {
        return mRaces;
    }

    template <>
    inline const Store<ESM::Region> &ESMStore::get<ESM::Region>() const {
        return mRegions;
    }

    template <>
    inline const Store<ESM::Repair> &ESMStore::get<ESM::Repair>() const {
        return mRepairs;
    }

    template <>
    inline const Store<ESM::SoundGenerator> &ESMStore::get<ESM::SoundGenerator>() const {
        return mSoundGens;
    }

    template <>
    inline const Store<ESM::Sound> &ESMStore::get<ESM::Sound>() const {
        return mSounds;
    }

    template <>
    inline const Store<ESM::Spell> &ESMStore::get<ESM::Spell>() const {
        return mSpells;
    }

    template <>
    inline const Store<ESM::StartScript> &ESMStore::get<ESM::StartScript>() const {
        return mStartScripts;
    }

    template <>
    inline const Store<ESM::Static> &ESMStore::get<ESM::Static>() const {
        return mStatics;
    }

    template <>
    inline const Store<ESM::Weapon> &ESMStore::get<ESM::Weapon>() const {
        return mWeapons;
    }

    template <>
    inline const Store<ESM::GameSetting> &ESMStore::get<ESM::GameSetting>() const {
        return mGameSettings;
    }

    template <>
    inline const Store<ESM::Script> &ESMStore::get<ESM::Script>() const {
        return mScripts;
    }

    template <>
    inline const Store<ESM::Cell> &ESMStore::get<ESM::Cell>() const {
        return mCells;
    }

    template <>
    inline const Store<ESM::Land> &ESMStore::get<ESM::Land>() const {
        return mLands;
    }

    template <>
    inline const Store<ESM::LandTexture> &ESMStore::get<ESM::LandTexture>() const {
        return mLandTextures;
    }

    template <>
    inline const Store<ESM::Pathgrid> &ESMStore::get<ESM::Pathgrid>() const {
        return mPathgrids;
    }

    template <>
    inline const Store<ESM::MagicEffect> &ESMStore::get<ESM::MagicEffect>() const {
        return mMagicEffects;
    }

    template <>
    inline const Store<ESM::Skill> &ESMStore::get<ESM::Skill>() const {
        return mSkills;
    }

    template <>
    inline const Store<ESM::Attribute> &ESMStore::get<ESM::Attribute>() const {
        return mAttributes;
    }

    template <>
    inline ForeignStore<ForeignWorld> &ESMStore::getForeignModifiable<ForeignWorld>() {
        return mForeignWorlds;
    }

    template <>
    inline const ForeignStore<ForeignWorld> &ESMStore::getForeign<ForeignWorld>() const {
        return mForeignWorlds;
    }

    template <>
    inline const ForeignStore<ForeignCell> &ESMStore::getForeign<ForeignCell>() const {
        return mForeignCells;
    }

    template <>
    inline ForeignStore<ForeignLand> &ESMStore::getForeignModifiable<ForeignLand>() {
        return mForeignLands;
    }

    template <>
    inline const ForeignStore<ForeignLand> &ESMStore::getForeign<ForeignLand>() const {
        return mForeignLands;
    }

    template <>
    inline const ForeignStore<ESM4::GlobalVariable>& ESMStore::getForeign<ESM4::GlobalVariable>() const {
        return mForeignGlobals;
    }

    template <>
    inline const ForeignStore<ESM4::Hair>& ESMStore::getForeign<ESM4::Hair>() const {
        return mForeignHairs;
    }

    template <>
    inline const ForeignStore<ESM4::Eyes>& ESMStore::getForeign<ESM4::Eyes>() const {
        return mForeignEyesSet;
    }

    template <>
    inline const ForeignStore<ESM4::Race>& ESMStore::getForeign<ESM4::Race>() const {
        return mForeignRaces;
    }

    template <>
    inline const ForeignStore<ESM4::Sound>& ESMStore::getForeign<ESM4::Sound>() const {
        return mForeignSounds;
    }

    template <>
    inline const ForeignStore<ESM4::LandTexture>& ESMStore::getForeign<ESM4::LandTexture>() const {
        return mForeignLandTextures;
    }

    template <>
    inline const ForeignStore<ESM4::Script>& ESMStore::getForeign<ESM4::Script>() const {
        return mForeignScripts;
    }

    template <>
    inline const ForeignStore<ForeignDialogue>& ESMStore::getForeign<ForeignDialogue>() const {
        return mForeignDialogues;
    }

    template <>
    inline const ForeignStore<ESM4::Quest>& ESMStore::getForeign<ESM4::Quest>() const {
        return mForeignQuests;
    }

    template <>
    inline const ForeignStore<ESM4::AIPackage>& ESMStore::getForeign<ESM4::AIPackage>() const {
        return mForeignAIPackages;
    }

    template <>
    inline const ForeignStore<ESM4::LightingTemplate>& ESMStore::getForeign<ESM4::LightingTemplate>() const {
        return mLightingTemplates;
    }

    template <>
    inline const ForeignStore<ESM4::Music>& ESMStore::getForeign<ESM4::Music>() const {
        return mMusic;
    }

    template <>
    inline const ForeignStore<ESM4::MediaSet>& ESMStore::getForeign<ESM4::MediaSet>() const {
        return mMediaSet;
    }

    template <>
    inline const ForeignStore<ESM4::MediaLocationController>& ESMStore::getForeign<ESM4::MediaLocationController>() const {
        return mMediaLocCtlr;
    }

    template <>
    inline const ForeignStore<ESM4::Region>& ESMStore::getForeign<ESM4::Region>() const {
        return mForeignRegions;
    }

    template <>
    inline const ForeignStore<ESM4::Activator>& ESMStore::getForeign<ESM4::Activator>() const {
        return mForeignActivators;
    }

    template <>
    inline const ForeignStore<ESM4::Apparatus>& ESMStore::getForeign<ESM4::Apparatus>() const {
        return mForeignApparatuses;
    }

    template <>
    inline const ForeignStore<ESM4::Armor>& ESMStore::getForeign<ESM4::Armor>() const {
        return mForeignArmors;
    }

    template <>
    inline const ForeignStore<ESM4::Book>& ESMStore::getForeign<ESM4::Book>() const {
        return mForeignBooks;
    }

    template <>
    inline const ForeignStore<ESM4::Clothing>& ESMStore::getForeign<ESM4::Clothing>() const {
        return mForeignClothes;
    }

    template <>
    inline const ForeignStore<ESM4::Container>& ESMStore::getForeign<ESM4::Container>() const {
        return mForeignContainers;
    }

    template <>
    inline const ForeignStore<ESM4::Door>& ESMStore::getForeign<ESM4::Door>() const {
        return mForeignDoors;
    }

    template <>
    inline const ForeignStore<ESM4::Ingredient>& ESMStore::getForeign<ESM4::Ingredient>() const {
        return mForeignIngredients;
    }

    template <>
    inline const ForeignStore<ESM4::Light>& ESMStore::getForeign<ESM4::Light>() const {
        return mForeignLights;
    }

    template <>
    inline const ForeignStore<ESM4::MiscItem>& ESMStore::getForeign<ESM4::MiscItem>() const {
        return mForeignMiscItems;
    }

    template <>
    inline const ForeignStore<ESM4::Static>& ESMStore::getForeign<ESM4::Static>() const {
        return mForeignStatics;
    }

    template <>
    inline const ForeignStore<ESM4::Grass>& ESMStore::getForeign<ESM4::Grass>() const {
        return mForeignGrasses;
    }

    template <>
    inline const ForeignStore<ESM4::Tree>& ESMStore::getForeign<ESM4::Tree>() const {
        return mForeignTrees;
    }

    template <>
    inline const ForeignStore<ESM4::Flora>& ESMStore::getForeign<ESM4::Flora>() const {
        return mForeignFloras;
    }

    template <>
    inline const ForeignStore<ESM4::Furniture>& ESMStore::getForeign<ESM4::Furniture>() const {
        return mForeignFurnitures;
    }

    template <>
    inline const ForeignStore<ESM4::Weapon>& ESMStore::getForeign<ESM4::Weapon>() const {
        return mForeignWeapons;
    }

    template <>
    inline const ForeignStore<ESM4::Ammunition>& ESMStore::getForeign<ESM4::Ammunition>() const {
        return mAmmunitions;
    }

    template <>
    inline const ForeignStore<ESM4::Npc>& ESMStore::getForeign<ESM4::Npc>() const {
        return mForeignNpcs;
    }

    template <>
    inline const ForeignStore<ESM4::Creature>& ESMStore::getForeign<ESM4::Creature>() const {
        return mForeignCreatures;
    }

    template <>
    inline const ForeignStore<ESM4::LevelledCreature>& ESMStore::getForeign<ESM4::LevelledCreature>() const {
        return mLevelledCreatures;
    }

    template <>
    inline const ForeignStore<ESM4::SoulGem>& ESMStore::getForeign<ESM4::SoulGem>() const {
        return mSoulGems;
    }

    template <>
    inline const ForeignStore<ESM4::Key>& ESMStore::getForeign<ESM4::Key>() const {
        return mForeignKeys;
    }

    template <>
    inline const ForeignStore<ESM4::Potion>& ESMStore::getForeign<ESM4::Potion>() const {
        return mForeignPotions;
    }

    template <>
    inline const ForeignStore<ESM4::SubSpace>& ESMStore::getForeign<ESM4::SubSpace>() const {
        return mSubSpaces;
    }

    template <>
    inline const ForeignStore<ESM4::SigilStone>& ESMStore::getForeign<ESM4::SigilStone>() const {
        return mSigilStones;
    }

    template <>
    inline const ForeignStore<ESM4::LevelledItem>& ESMStore::getForeign<ESM4::LevelledItem>() const {
        return mLevelledItems;
    }

    template <>
    inline const ForeignStore<ESM4::LevelledNpc>& ESMStore::getForeign<ESM4::LevelledNpc>() const {
        return mLevelledNpcs;
    }

    template <>
    inline const ForeignStore<ESM4::IdleMarker>& ESMStore::getForeign<ESM4::IdleMarker>() const {
        return mIdleMarkers;
    }

    template <>
    inline const ForeignStore<ESM4::MovableStatic>& ESMStore::getForeign<ESM4::MovableStatic>() const {
        return mMovableStatics;
    }

    template <>
    inline const ForeignStore<ESM4::TextureSet>& ESMStore::getForeign<ESM4::TextureSet>() const {
        return mTextureSets;
    }

    template <>
    inline const ForeignStore<ESM4::Scroll>& ESMStore::getForeign<ESM4::Scroll>() const {
        return mForeignScrolls;
    }

    template <>
    inline const ForeignStore<ESM4::ArmorAddon>& ESMStore::getForeign<ESM4::ArmorAddon>() const {
        return mArmorAddons;
    }

    template <>
    inline const ForeignStore<ESM4::HeadPart>& ESMStore::getForeign<ESM4::HeadPart>() const {
        return mHeadParts;
    }

    template <>
    inline const ForeignStore<ESM4::Terminal>& ESMStore::getForeign<ESM4::Terminal>() const {
        return mTerminals;
    }

    template <>
    inline const ForeignStore<ESM4::TalkingActivator>& ESMStore::getForeign<ESM4::TalkingActivator>() const {
        return mTalkingActivators;
    }

    template <>
    inline const ForeignStore<ESM4::Note>& ESMStore::getForeign<ESM4::Note>() const {
        return mNotes;
    }

    template <>
    inline const ForeignStore<ESM4::AcousticSpace>& ESMStore::getForeign<ESM4::AcousticSpace>() const {
        return mAcousticSpaces;
    }

    template <>
    inline const ForeignStore<ESM4::ItemMod>& ESMStore::getForeign<ESM4::ItemMod>() const {
        return mItemMods;
    }

    template <>
    inline const ForeignStore<ESM4::PlaceableWater>& ESMStore::getForeign<ESM4::PlaceableWater>() const {
        return mPlaceableWaters;
    }

    template <>
    inline const ForeignStore<ESM4::StaticCollection>& ESMStore::getForeign<ESM4::StaticCollection>() const {
        return mStaticCollections;
    }

    template <>
    inline const ForeignStore<ESM4::SoundReference>& ESMStore::getForeign<ESM4::SoundReference>() const {
        return mSoundReferences;
    }

    template <>
    inline const ForeignStore<ESM4::Outfit>& ESMStore::getForeign<ESM4::Outfit>() const {
        return mOutfits;
    }
}

#endif
