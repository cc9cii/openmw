#ifndef GAME_MWWORLD_CONTAINERSTORE_H
#define GAME_MWWORLD_CONTAINERSTORE_H

#include <iterator>
#include <map>

#include <extern/esm4/alch.hpp>
#include <extern/esm4/appa.hpp>
#include <extern/esm4/armo.hpp>
#include <extern/esm4/book.hpp>
#include <extern/esm4/clot.hpp>
#include <extern/esm4/ingr.hpp>
#include <extern/esm4/ligh.hpp>
#include <extern/esm4/slgm.hpp>
#include <extern/esm4/misc.hpp>
#include <extern/esm4/keym.hpp>
#include <extern/esm4/sgst.hpp>
#include <extern/esm4/weap.hpp>
#include <extern/esm4/ammo.hpp>
#include <extern/esm4/lvli.hpp>
#include <extern/esm4/note.hpp>
#include <extern/esm4/imod.hpp>

#include <components/esm/loadalch.hpp>
#include <components/esm/loadappa.hpp>
#include <components/esm/loadarmo.hpp>
#include <components/esm/loadbook.hpp>
#include <components/esm/loadclot.hpp>
#include <components/esm/loadingr.hpp>
#include <components/esm/loadlock.hpp>
#include <components/esm/loadligh.hpp>
#include <components/esm/loadmisc.hpp>
#include <components/esm/loadprob.hpp>
#include <components/esm/loadrepa.hpp>
#include <components/esm/loadweap.hpp>

#include "ptr.hpp"
#include "cellreflist.hpp"

namespace ESM
{
    struct InventoryList;
    struct InventoryState;
}

namespace MWWorld
{
    class ContainerStoreIterator;

    class ContainerStore
    {
        public:

            static const int Type_Potion = 0x0001;
            static const int Type_Apparatus = 0x0002;
            static const int Type_Armor = 0x0004;
            static const int Type_Book = 0x0008;
            static const int Type_Clothing = 0x0010;
            static const int Type_Ingredient = 0x0020;
            static const int Type_Light = 0x0040;
            static const int Type_Lockpick = 0x0080;
            static const int Type_Miscellaneous = 0x0100;
            static const int Type_Probe = 0x0200;
            static const int Type_Repair = 0x0400;
            static const int Type_Weapon = 0x0800;

            static const int Type_ForeignPotion       = 0x1000; // see incType, needs bit shift by one
            static const int Type_ForeignApparatus    = 0x2000 ;
            static const int Type_ForeignArmor        = 0x4000;
            static const int Type_ForeignBook         = 0x8000;
            static const int Type_ForeignClothing    = 0x10000;
            static const int Type_ForeignIngredient  = 0x20000;
            static const int Type_ForeignLight       = 0x40000;
            static const int Type_SoulGem            = 0x80000;
            static const int Type_ForeignMiscItem   = 0x100000;
            static const int Type_ForeignKey        = 0x200000;
            static const int Type_SigilStone        = 0x400000;
            static const int Type_ForeignWeapon     = 0x800000;
            static const int Type_Ammunition       = 0x1000000;
            static const int Type_Note             = 0x2000000;
            static const int Type_LevelledItem     = 0x4000000; // unused?
            static const int Type_ItemMod          = 0x8000000;
            static const int Type_CaravanCard     = 0x10000000;
            static const int Type_CaravanMoney    = 0x20000000;
            static const int Type_CasinoChip      = 0x40000000;

            static const int Type_Last = Type_Note;

            static const int Type_All = 0xffffffff;

            static const std::string sGoldId;

        private:

            MWWorld::CellRefList<ESM::Potion>            potions;
            MWWorld::CellRefList<ESM::Apparatus>         appas;
            MWWorld::CellRefList<ESM::Armor>             armors;
            MWWorld::CellRefList<ESM::Book>              books;
            MWWorld::CellRefList<ESM::Clothing>          clothes;
            MWWorld::CellRefList<ESM::Ingredient>        ingreds;
            MWWorld::CellRefList<ESM::Light>             lights;
            MWWorld::CellRefList<ESM::Lockpick>          lockpicks;
            MWWorld::CellRefList<ESM::Miscellaneous>     miscItems;
            MWWorld::CellRefList<ESM::Probe>             probes;
            MWWorld::CellRefList<ESM::Repair>            repairs;
            MWWorld::CellRefList<ESM::Weapon>            weapons;

            MWWorld::CellRefList<ESM4::Potion>           foreignPotions;
            MWWorld::CellRefList<ESM4::Apparatus>        foreignAppas;
            MWWorld::CellRefList<ESM4::Armor>            foreignArmors;
            MWWorld::CellRefList<ESM4::Book>             foreignBooks;
            MWWorld::CellRefList<ESM4::Clothing>         foreignClothes;
            MWWorld::CellRefList<ESM4::Ingredient>       foreignIngredients;
            MWWorld::CellRefList<ESM4::Light>            foreignLights;
            MWWorld::CellRefList<ESM4::SoulGem>          soulGems;
            MWWorld::CellRefList<ESM4::MiscItem>         foreignMiscItems;
            MWWorld::CellRefList<ESM4::Key>              foreignKeys;
            MWWorld::CellRefList<ESM4::SigilStone>       sigilStones;
            MWWorld::CellRefList<ESM4::Weapon>           foreignWeapons;
            MWWorld::CellRefList<ESM4::Ammunition>       ammunitions;
            MWWorld::CellRefList<ESM4::Note>             notes;
            //MWWorld::CellRefList<ESM4::ItemMod>          itemMods;
            // FIXME: CCRD, CMNY, CHIP (LVLI, MSTT and STAT as well?)

            std::map<std::string, int> mLevelledItemMap;
            ///< Stores result of levelled item spawns. <refId, count>
            /// This is used to remove the spawned item(s) if the levelled item is restocked.

            mutable float mCachedWeight;
            mutable bool mWeightUpToDate;
            ContainerStoreIterator addImp (const Ptr& ptr, int count);
            void addInitialItem (const std::string& id, const std::string& owner, int count, bool topLevel=true, const std::string& levItem = "");

            template<typename T>
            ContainerStoreIterator getState (CellRefList<T>& collection,
                const ESM::ObjectState& state);

            template<typename T>
            void storeState (const LiveCellRef<T>& ref, ESM::ObjectState& state) const;

            template<typename T>
            void storeStates (CellRefList<T>& collection,
                ESM::InventoryState& inventory, int& index,
                bool equipable = false) const;

            virtual void storeEquipmentState (const MWWorld::LiveCellRefBase& ref, int index, ESM::InventoryState& inventory) const;

            virtual void readEquipmentState (const MWWorld::ContainerStoreIterator& iter, int index, const ESM::InventoryState& inventory);
        public:

            ContainerStore();

            virtual ~ContainerStore();

            virtual ContainerStore* clone() { return new ContainerStore(*this); }

            ContainerStoreIterator begin (int mask = Type_All);

            ContainerStoreIterator end();

            virtual ContainerStoreIterator add (const Ptr& itemPtr, int count, const Ptr& actorPtr, bool setOwner=false);
            ///< Add the item pointed to by \a ptr to this container. (Stacks automatically if needed)
            ///
            /// \note The item pointed to is not required to exist beyond this function call.
            ///
            /// \attention Do not add items to an existing stack by increasing the count instead of
            /// calling this function!
            ///
            /// @param setOwner Set the owner of the added item to \a actorPtr? If false, the owner is reset to "".
            ///
            /// @return if stacking happened, return iterator to the item that was stacked against, otherwise iterator to the newly inserted item.

            ContainerStoreIterator add(const std::string& id, int count, const Ptr& actorPtr);
            ///< Utility to construct a ManualRef and call add(ptr, count, actorPtr, true)

            int remove(const std::string& itemId, int count, const Ptr& actor);
            ///< Remove \a count item(s) designated by \a itemId from this container.
            ///
            /// @return the number of items actually removed

            virtual int remove(const Ptr& item, int count, const Ptr& actor);
            ///< Remove \a count item(s) designated by \a item from this inventory.
            ///
            /// @return the number of items actually removed

            void unstack (const Ptr& ptr, const Ptr& container);
            ///< Unstack an item in this container. The item's count will be set to 1, then a new stack will be added with (origCount-1).

            MWWorld::ContainerStoreIterator restack (const MWWorld::Ptr& item);
            ///< Attempt to re-stack an item in this container.
            /// If a compatible stack is found, the item's count is added to that stack, then the original is deleted.
            /// @return If the item was stacked, return the stack, otherwise return the old (untouched) item.

            /// @return How many items with refID \a id are in this container?
            int count (const std::string& id);

            int countForeign (const std::string& id); // count(), but match the base object's EditorId instead

        protected:
            ContainerStoreIterator addNewStack (const Ptr& ptr, int count);
            ///< Add the item to this container (do not try to stack it onto existing items)

            virtual void flagAsModified();

        public:

            virtual bool stacks (const Ptr& ptr1, const Ptr& ptr2);
            ///< @return true if the two specified objects can stack with each other

            void fill (const ESM::InventoryList& items, const std::string& owner);
            ///< Insert items into *this.

            void restock (const ESM::InventoryList& items, const MWWorld::Ptr& ptr, const std::string& owner);

            virtual void clear();
            ///< Empty container.

            float getWeight() const;
            ///< Return total weight of the items contained in *this.

            static int getType (const Ptr& ptr);
            ///< This function throws an exception, if ptr does not point to an object, that can be
            /// put into a container.

            Ptr search (const std::string& id);

            /// \todo make this method const once const-correct ContainerStoreIterators are available
            virtual void writeState (ESM::InventoryState& state);

            virtual void readState (const ESM::InventoryState& state);

        friend class ContainerStoreIterator;
    };

    /// \brief Iteration over a subset of objects in a ContainerStore
    ///
    /// \note The iterator will automatically skip over deleted objects.
    class ContainerStoreIterator
        //: public std::iterator<std::forward_iterator_tag, Ptr, std::ptrdiff_t, Ptr *, Ptr&>
    {
        public:
            // std::iterator to be deprecated in C++17, need C++11 for keyword 'using'
            using iterator_category = std::forward_iterator_tag; // allows multiple copies
            using value_type = Ptr;
            using difference_type = std::ptrdiff_t;
            using pointer = Ptr*;
            using reference = Ptr&;

        private:
            int mType;
            int mMask;
            ContainerStore *mContainer;
            mutable Ptr mPtr;

            MWWorld::CellRefList<ESM::Potion>::List::iterator mPotion;
            MWWorld::CellRefList<ESM::Apparatus>::List::iterator mApparatus;
            MWWorld::CellRefList<ESM::Armor>::List::iterator mArmor;
            MWWorld::CellRefList<ESM::Book>::List::iterator mBook;
            MWWorld::CellRefList<ESM::Clothing>::List::iterator mClothing;
            MWWorld::CellRefList<ESM::Ingredient>::List::iterator mIngredient;
            MWWorld::CellRefList<ESM::Light>::List::iterator mLight;
            MWWorld::CellRefList<ESM::Lockpick>::List::iterator mLockpick;
            MWWorld::CellRefList<ESM::Miscellaneous>::List::iterator mMiscellaneous;
            MWWorld::CellRefList<ESM::Probe>::List::iterator mProbe;
            MWWorld::CellRefList<ESM::Repair>::List::iterator mRepair;
            MWWorld::CellRefList<ESM::Weapon>::List::iterator mWeapon;

            MWWorld::CellRefList<ESM4::Potion>::List::iterator mForeignPotion;
            MWWorld::CellRefList<ESM4::Apparatus>::List::iterator mForeignApparatus;
            MWWorld::CellRefList<ESM4::Armor>::List::iterator mForeignArmor;
            MWWorld::CellRefList<ESM4::Book>::List::iterator mForeignBook;
            MWWorld::CellRefList<ESM4::Clothing>::List::iterator mForeignClothing;
            MWWorld::CellRefList<ESM4::Ingredient>::List::iterator mForeignIngredient;
            MWWorld::CellRefList<ESM4::Light>::List::iterator mForeignLight;
            MWWorld::CellRefList<ESM4::SoulGem>::List::iterator mSoulGem;
            MWWorld::CellRefList<ESM4::MiscItem>::List::iterator mForeignMiscItem;
            MWWorld::CellRefList<ESM4::Key>::List::iterator mForeignKey;
            MWWorld::CellRefList<ESM4::SigilStone>::List::iterator mSigilStone;
            MWWorld::CellRefList<ESM4::Weapon>::List::iterator mForeignWeapon;
            MWWorld::CellRefList<ESM4::Ammunition>::List::iterator mAmmunition;
            MWWorld::CellRefList<ESM4::Note>::List::iterator mNote;
            //MWWorld::CellRefList<ESM4::ItemMod>::List::iterator mItemMod;

        private:

            ContainerStoreIterator (ContainerStore *container);
            ///< End-iterator

            ContainerStoreIterator (int mask, ContainerStore *container);
            ///< Begin-iterator

            // construct iterator using a CellRefList iterator
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Potion>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Apparatus>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Armor>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Book>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Clothing>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Ingredient>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Light>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Lockpick>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Miscellaneous>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Probe>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Repair>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Weapon>::List::iterator);

            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Potion>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Apparatus>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Armor>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Book>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Clothing>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Ingredient>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Light>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::SoulGem>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::MiscItem>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Key>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::SigilStone>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Weapon>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Ammunition>::List::iterator);
            ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Note>::List::iterator);
            //ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::ItemMod>::List::iterator);

            void copy (const ContainerStoreIterator& src);

            void incType();

            void nextType();

            bool resetIterator();
            ///< Reset iterator for selected type.
            ///
            /// \return Type not empty?

            bool incIterator();
            ///< Increment iterator for selected type.
            ///
            /// \return reached the end?

        public:

            ContainerStoreIterator(const ContainerStoreIterator& src);

            Ptr *operator->() const;

            Ptr operator*() const;

            ContainerStoreIterator& operator++();

            ContainerStoreIterator operator++ (int);

            ContainerStoreIterator& operator= (const ContainerStoreIterator& rhs);

            bool isEqual (const ContainerStoreIterator& iter) const;

            int getType() const;

            const ContainerStore *getContainerStore() const;

        friend class ContainerStore;
    };

    bool operator== (const ContainerStoreIterator& left, const ContainerStoreIterator& right);
    bool operator!= (const ContainerStoreIterator& left, const ContainerStoreIterator& right);
}

#endif
