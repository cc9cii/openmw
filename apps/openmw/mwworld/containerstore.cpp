#include "containerstore.hpp"

#include <cassert>
#include <typeinfo>
#include <stdexcept>
#include <iostream>

#include <components/esm/inventorystate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/levelledlist.hpp"

#include "manualref.hpp"
#include "refdata.hpp"
#include "class.hpp"
#include "localscripts.hpp"
#include "player.hpp"

namespace
{
    template<typename T>
    float getTotalWeight (const MWWorld::CellRefList<T>& cellRefList)
    {
        float sum = 0;

        for (typename MWWorld::CellRefList<T>::List::const_iterator iter (
            cellRefList.mList.begin());
            iter!=cellRefList.mList.end();
            ++iter)
        {
            if (iter->mData.getCount()>0)
                sum += iter->mData.getCount()*iter->mBase->mData.mWeight;
        }

        return sum;
    }

    // TODO: keep mIdMap?
    template<typename T>
    MWWorld::Ptr searchId (MWWorld::CellRefList<T>& list, const std::string& id,
        MWWorld::ContainerStore *store)
    {
        std::string id2 = Misc::StringUtils::lowerCase (id);

        for (typename MWWorld::CellRefList<T>::List::iterator iter (list.mList.begin());
             iter!=list.mList.end(); ++iter)
        {
            if (Misc::StringUtils::ciEqual(iter->mBase->mId, id2))
            {
                MWWorld::Ptr ptr (&*iter, 0);
                ptr.setContainerStore (store);
                return ptr;
            }
        }

        return MWWorld::Ptr();
    }
}

template<typename T>
MWWorld::ContainerStoreIterator MWWorld::ContainerStore::getState (CellRefList<T>& collection,
    const ESM::ObjectState& state)
{
    if (!LiveCellRef<T>::checkState (state))
        return ContainerStoreIterator (this); // not valid anymore with current content files -> skip

    const T *record = MWBase::Environment::get().getWorld()->getStore().
        get<T>().search (state.mRef.mRefID);

    if (!record)
        return ContainerStoreIterator (this);

    LiveCellRef<T> ref (record);
    ref.load (state);
    collection.mList.push_back (ref);

    return ContainerStoreIterator (this, --collection.mList.end());
}

void MWWorld::ContainerStore::storeEquipmentState(const MWWorld::LiveCellRefBase &ref, int index, ESM::InventoryState &inventory) const
{
}

void MWWorld::ContainerStore::readEquipmentState(const MWWorld::ContainerStoreIterator& iter, int index, const ESM::InventoryState &inventory)
{
}

template<typename T>
void MWWorld::ContainerStore::storeState (const LiveCellRef<T>& ref, ESM::ObjectState& state) const
{
    ref.save (state);
}

template<typename T>
void MWWorld::ContainerStore::storeStates (CellRefList<T>& collection,
    ESM::InventoryState& inventory, int& index, bool equipable) const
{
    for (typename CellRefList<T>::List::const_iterator iter (collection.mList.begin());
        iter!=collection.mList.end(); ++iter)
    {
        if (iter->mData.getCount() == 0)
            continue;
        ESM::ObjectState state;
        storeState (*iter, state);
        if (equipable)
            storeEquipmentState(*iter, index, inventory);
        inventory.mItems.push_back (state);
        ++index;
    }
}

const std::string MWWorld::ContainerStore::sGoldId = "gold_001";

MWWorld::ContainerStore::ContainerStore() : mCachedWeight (0), mWeightUpToDate (false) {}

MWWorld::ContainerStore::~ContainerStore() {}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::begin (int mask)
{
    return ContainerStoreIterator (mask, this);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::end()
{
    return ContainerStoreIterator (this);
}

int MWWorld::ContainerStore::count(const std::string &id)
{
    int total=0;
    for (MWWorld::ContainerStoreIterator iter (begin()); iter!=end(); ++iter)
        if (Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), id))
            total += iter->getRefData().getCount();
    return total;
}

int MWWorld::ContainerStore::countForeign(const std::string &id)
{
    int total=0;
    for (MWWorld::ContainerStoreIterator iter(begin()); iter != end(); ++iter)
    {
        std::string baseEditorId = iter->getClass().getEditorId(*iter);

        if (Misc::StringUtils::ciEqual(baseEditorId, id))
            total += iter->getRefData().getCount();
    }

    return total;
}

void MWWorld::ContainerStore::unstack(const Ptr &ptr, const Ptr& container)
{
    if (ptr.getRefData().getCount() <= 1)
        return;
    addNewStack(ptr, ptr.getRefData().getCount()-1);
    remove(ptr, ptr.getRefData().getCount()-1, container);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::restack(const MWWorld::Ptr& item)
{
    MWWorld::ContainerStoreIterator retval = end();
    for (MWWorld::ContainerStoreIterator iter (begin()); iter != end(); ++iter)
    {
        if (item == *iter)
        {
            retval = iter;
            break;
        }
    }

    if (retval == end())
        throw std::runtime_error("item is not from this container");

    for (MWWorld::ContainerStoreIterator iter (begin()); iter != end(); ++iter)
    {
        if (stacks(*iter, item))
        {
            iter->getRefData().setCount(iter->getRefData().getCount() + item.getRefData().getCount());
            item.getRefData().setCount(0);
            retval = iter;
            break;
        }
    }
    return retval;
}

bool MWWorld::ContainerStore::stacks(const Ptr& ptr1, const Ptr& ptr2)
{
    const MWWorld::Class& cls1 = ptr1.getClass();
    const MWWorld::Class& cls2 = ptr2.getClass();

    if (!Misc::StringUtils::ciEqual(ptr1.getCellRef().getRefId(), ptr2.getCellRef().getRefId()))
        return false;

    // If it has an enchantment, don't stack when some of the charge is already used
    if (!ptr1.getClass().getEnchantment(ptr1).empty())
    {
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    ptr1.getClass().getEnchantment(ptr1));
        float maxCharge = static_cast<float>(enchantment->mData.mCharge);
        float enchantCharge1 = ptr1.getCellRef().getEnchantmentCharge() == -1 ? maxCharge : ptr1.getCellRef().getEnchantmentCharge();
        float enchantCharge2 = ptr2.getCellRef().getEnchantmentCharge() == -1 ? maxCharge : ptr2.getCellRef().getEnchantmentCharge();
        if (enchantCharge1 != maxCharge || enchantCharge2 != maxCharge)
            return false;
    }

    return ptr1 != ptr2 // an item never stacks onto itself
        && ptr1.getCellRef().getOwner() == ptr2.getCellRef().getOwner()
        && ptr1.getCellRef().getSoul() == ptr2.getCellRef().getSoul()

        && ptr1.getClass().getRemainingUsageTime(ptr1) == ptr2.getClass().getRemainingUsageTime(ptr2)

        && cls1.getScript(ptr1) == cls2.getScript(ptr2)

        // item that is already partly used up never stacks
        && (!cls1.hasItemHealth(ptr1) || (
                cls1.getItemHealth(ptr1) == cls1.getItemMaxHealth(ptr1)
            && cls2.getItemHealth(ptr2) == cls2.getItemMaxHealth(ptr2)));
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::add(const std::string &id, int count, const Ptr &actorPtr)
{
    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), id, count);
    // a bit pointless to set owner for the player
    if (actorPtr != MWBase::Environment::get().getWorld()->getPlayerPtr())
        return add(ref.getPtr(), count, actorPtr, true);
    else
        return add(ref.getPtr(), count, actorPtr, false);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::add (const Ptr& itemPtr, int count, const Ptr& actorPtr, bool setOwner)
{
    Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();

    MWWorld::ContainerStoreIterator it = end();

    // HACK: Set owner on the original item, then reset it after we have copied it
    // If we set the owner on the copied item, it would not stack correctly...
    std::string oldOwner = itemPtr.getCellRef().getOwner();
    if (!setOwner || actorPtr == MWBase::Environment::get().getWorld()->getPlayerPtr()) // No point in setting owner to the player - NPCs will not respect this anyway
    {
        itemPtr.getCellRef().setOwner("");
    }
    else
    {
        itemPtr.getCellRef().setOwner(actorPtr.getCellRef().getRefId());
    }

    it = addImp(itemPtr, count);

    itemPtr.getCellRef().setOwner(oldOwner);

    // The copy of the original item we just made
    MWWorld::Ptr item = *it;

    // we may have copied an item from the world, so reset a few things first
    item.getRefData().setBaseNode(NULL); // Especially important, otherwise scripts on the item could think that it's actually in a cell
    ESM::Position pos;
    pos.rot[0] = 0;
    pos.rot[1] = 0;
    pos.rot[2] = 0;
    pos.pos[0] = 0;
    pos.pos[1] = 0;
    pos.pos[2] = 0;
    item.getCellRef().setPosition(pos);

    // reset ownership stuff, owner was already handled above
    item.getCellRef().resetGlobalVariable();
    item.getCellRef().setFaction("");
    item.getCellRef().setFactionRank(-1);

    // must reset the RefNum on the copied item, so that the RefNum on the original item stays unique
    // maybe we should do this in the copy constructor instead?
    item.getCellRef().unsetRefNum(); // destroy link to content file

    std::string script = item.getClass().getScript(item);
    if(script != "")
    {
        if (actorPtr == player)
        {
            // Items in player's inventory have cell set to 0, so their scripts will never be removed
            item.mCell = 0;
        }
        else
        {
            // Set mCell to the cell of the container/actor, so that the scripts are removed properly when
            // the cell of the container/actor goes inactive
            item.mCell = actorPtr.getCell();
        }

        item.mContainerStore = this;

        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, item);

        // Set OnPCAdd special variable, if it is declared
        // Make sure to do this *after* we have added the script to LocalScripts
        if (actorPtr == player)
            item.getRefData().getLocals().setVarByInt(script, "onpcadd", 1);
    }

    return it;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::addImp (const Ptr& ptr, int count)
{
    int type = getType(ptr);

    const MWWorld::ESMStore &esmStore =
        MWBase::Environment::get().getWorld()->getStore();

    // gold needs special handling: when it is inserted into a container, the base object automatically becomes Gold_001
    // this ensures that gold piles of different sizes stack with each other (also, several scripts rely on Gold_001 for detecting player gold)
    if (Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_001")
        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_005")
        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_010")
        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_025")
        || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_100"))
    {
        int realCount = count * ptr.getClass().getValue(ptr);

        for (MWWorld::ContainerStoreIterator iter (begin(type)); iter!=end(); ++iter)
        {
            if (Misc::StringUtils::ciEqual((*iter).getCellRef().getRefId(), MWWorld::ContainerStore::sGoldId))
            {
                iter->getRefData().setCount(iter->getRefData().getCount() + realCount);
                flagAsModified();
                return iter;
            }
        }

        MWWorld::ManualRef ref(esmStore, MWWorld::ContainerStore::sGoldId, realCount);
        return addNewStack(ref.getPtr(), realCount);
    }

    // determine whether to stack or not
    for (MWWorld::ContainerStoreIterator iter (begin(type)); iter!=end(); ++iter)
    {
        if (stacks(*iter, ptr))
        {
            // stack
            iter->getRefData().setCount( iter->getRefData().getCount() + count );

            flagAsModified();
            return iter;
        }
    }
    // if we got here, this means no stacking
    return addNewStack(ptr, count);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::addNewStack (const Ptr& ptr, int count)
{
    ContainerStoreIterator it = begin();

    switch (getType(ptr))
    {
        case Type_Potion: potions.mList.push_back (*ptr.get<ESM::Potion>()); it = ContainerStoreIterator(this, --potions.mList.end()); break;
        case Type_Apparatus: appas.mList.push_back (*ptr.get<ESM::Apparatus>()); it = ContainerStoreIterator(this, --appas.mList.end()); break;
        case Type_Armor: armors.mList.push_back (*ptr.get<ESM::Armor>()); it = ContainerStoreIterator(this, --armors.mList.end()); break;
        case Type_Book: books.mList.push_back (*ptr.get<ESM::Book>()); it = ContainerStoreIterator(this, --books.mList.end()); break;
        case Type_Clothing: clothes.mList.push_back (*ptr.get<ESM::Clothing>()); it = ContainerStoreIterator(this, --clothes.mList.end()); break;
        case Type_Ingredient: ingreds.mList.push_back (*ptr.get<ESM::Ingredient>()); it = ContainerStoreIterator(this, --ingreds.mList.end()); break;
        case Type_Light: lights.mList.push_back (*ptr.get<ESM::Light>()); it = ContainerStoreIterator(this, --lights.mList.end()); break;
        case Type_Lockpick: lockpicks.mList.push_back (*ptr.get<ESM::Lockpick>()); it = ContainerStoreIterator(this, --lockpicks.mList.end()); break;
        case Type_Miscellaneous: miscItems.mList.push_back (*ptr.get<ESM::Miscellaneous>()); it = ContainerStoreIterator(this, --miscItems.mList.end()); break;
        case Type_Probe: probes.mList.push_back (*ptr.get<ESM::Probe>()); it = ContainerStoreIterator(this, --probes.mList.end()); break;
        case Type_Repair: repairs.mList.push_back (*ptr.get<ESM::Repair>()); it = ContainerStoreIterator(this, --repairs.mList.end()); break;
        case Type_Weapon: weapons.mList.push_back (*ptr.get<ESM::Weapon>()); it = ContainerStoreIterator(this, --weapons.mList.end()); break;

        case Type_ForeignPotion: foreignPotions.mList.push_back (*ptr.get<ESM4::Potion>()); it = ContainerStoreIterator(this, --foreignPotions.mList.end()); break;
        case Type_ForeignApparatus: foreignAppas.mList.push_back (*ptr.get<ESM4::Apparatus>()); it = ContainerStoreIterator(this, --foreignAppas.mList.end()); break;
        case Type_ForeignArmor: foreignArmors.mList.push_back (*ptr.get<ESM4::Armor>()); it = ContainerStoreIterator(this, --foreignArmors.mList.end()); break;
        case Type_ForeignBook: foreignBooks.mList.push_back (*ptr.get<ESM4::Book>()); it = ContainerStoreIterator(this, --foreignBooks.mList.end()); break;
        case Type_ForeignClothing: foreignClothes.mList.push_back (*ptr.get<ESM4::Clothing>()); it = ContainerStoreIterator(this, --foreignClothes.mList.end()); break;
        case Type_ForeignIngredient: foreignIngredients.mList.push_back (*ptr.get<ESM4::Ingredient>()); it = ContainerStoreIterator(this, --foreignIngredients.mList.end()); break;
        case Type_ForeignLight: foreignLights.mList.push_back (*ptr.get<ESM4::Light>()); it = ContainerStoreIterator(this, --foreignLights.mList.end()); break;
        case Type_SoulGem: soulGems.mList.push_back (*ptr.get<ESM4::SoulGem>()); it = ContainerStoreIterator(this, --soulGems.mList.end()); break;
        case Type_ForeignMiscItem: foreignMiscItems.mList.push_back (*ptr.get<ESM4::MiscItem>()); it = ContainerStoreIterator(this, --foreignMiscItems.mList.end()); break;
        case Type_ForeignKey: foreignKeys.mList.push_back (*ptr.get<ESM4::Key>()); it = ContainerStoreIterator(this, --foreignKeys.mList.end()); break;
        case Type_SigilStone: sigilStones.mList.push_back (*ptr.get<ESM4::SigilStone>()); it = ContainerStoreIterator(this, --sigilStones.mList.end()); break;
        case Type_ForeignWeapon: foreignWeapons.mList.push_back (*ptr.get<ESM4::Weapon>()); it = ContainerStoreIterator(this, --foreignWeapons.mList.end()); break;
        case Type_Ammunition: ammunitions.mList.push_back (*ptr.get<ESM4::Ammunition>()); it = ContainerStoreIterator(this, --ammunitions.mList.end()); break;
        case Type_Note: notes.mList.push_back (*ptr.get<ESM4::Note>()); it = ContainerStoreIterator(this, --notes.mList.end()); break;
        //case Type_ItemMod: itemMods.mList.push_back (*ptr.get<ESM4::ItemMod>()); it = ContainerStoreIterator(this, --itemMods.mList.end()); break;
    }

    it->getRefData().setCount(count);

    flagAsModified();
    return it;
}

int MWWorld::ContainerStore::remove(const std::string& itemId, int count, const Ptr& actor)
{
    int toRemove = count;

    for (ContainerStoreIterator iter(begin()); iter != end() && toRemove > 0; ++iter)
        if (Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), itemId))
            toRemove -= remove(*iter, toRemove, actor);

    flagAsModified();

    // number of removed items
    return count - toRemove;
}

int MWWorld::ContainerStore::remove(const Ptr& item, int count, const Ptr& actor)
{
    assert(this == item.getContainerStore());

    int toRemove = count;
    RefData& itemRef = item.getRefData();

    if (itemRef.getCount() <= toRemove)
    {
        toRemove -= itemRef.getCount();
        itemRef.setCount(0);
    }
    else
    {
        itemRef.setCount(itemRef.getCount() - toRemove);
        toRemove = 0;
    }

    flagAsModified();

    // number of removed items
    return count - toRemove;
}

void MWWorld::ContainerStore::fill (const ESM::InventoryList& items, const std::string& owner)
{
    for (std::vector<ESM::ContItem>::const_iterator iter (items.mList.begin()); iter!=items.mList.end();
        ++iter)
    {
        std::string id = Misc::StringUtils::lowerCase(iter->mItem.toString());
        addInitialItem(id, owner, iter->mCount);
    }

    flagAsModified();
}

void MWWorld::ContainerStore::addInitialItem (const std::string& id, const std::string& owner,
                                              int count, bool topLevel, const std::string& levItem)
{
    ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), id, count);

    if (ref.getPtr().getTypeName()==typeid (ESM::ItemLevList).name())
    {
        const ESM::ItemLevList* levItem = ref.getPtr().get<ESM::ItemLevList>()->mBase;

        if (topLevel && std::abs(count) > 1 && levItem->mFlags & ESM::ItemLevList::Each)
        {
            for (int i=0; i<std::abs(count); ++i)
                addInitialItem(id, owner, count > 0 ? 1 : -1, true, levItem->mId);
            return;
        }
        else
        {
            std::string id = MWMechanics::getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, false);
            if (id.empty())
                return;
            addInitialItem(id, owner, count, false, levItem->mId);
        }
    }
    else if (ref.getPtr().getTypeName()==typeid (ESM4::LevelledItem).name())
    {
        const ESM4::LevelledItem* levItem = ref.getPtr().get<ESM4::LevelledItem>()->mBase;

        if (topLevel && std::abs(count) > 1 && levItem->calcEachItemInCount())
        {
            for (int i = 0; i < std::abs(count); ++i)
            {
                // go topLevel again but this time with negative count
                addInitialItem(id, owner, count > 0 ? 1 : -1, true/*topLevel*/,
                               ESM4::formIdToString(ref.getPtr().get<ESM4::LevelledItem>()->mBase->mFormId));
            }
        }
        else
        {
            std::vector<std::string> ids;
            MWMechanics::getTES4LevelledItem(ids, ref.getPtr().get<ESM4::LevelledItem>()->mBase);

            for (std::size_t i = 0; i < ids.size(); ++i)
            {
                if (ids[i].empty())
                    return; // RNG not kind today :-(

                addInitialItem(ids[i], owner, count, false,
                               ESM4::formIdToString(ref.getPtr().get<ESM4::LevelledItem>()->mBase->mFormId));
            }
        }
    }
    else
    {
        // A negative count indicates restocking items
        // For a restocking levelled item, remember what we spawned so we can delete it later when the merchant restocks
        if (!levItem.empty() && count < 0)
        {
            // FIXME: not yet done for TES4
            if (mLevelledItemMap.find(id) == mLevelledItemMap.end())
                mLevelledItemMap[id] = 0;
            mLevelledItemMap[id] += std::abs(count);
        }
        count = std::abs(count);

        ref.getPtr().getCellRef().setOwner(owner);
        addImp (ref.getPtr(), count);
    }
}

void MWWorld::ContainerStore::restock (const ESM::InventoryList& items, const MWWorld::Ptr& ptr, const std::string& owner)
{
    // Remove the items already spawned by levelled items that will restock
    for (std::map<std::string, int>::iterator it = mLevelledItemMap.begin(); it != mLevelledItemMap.end(); ++it)
    {
        if (count(it->first) >= it->second)
            remove(it->first, it->second, ptr);
    }
    mLevelledItemMap.clear();

    for (std::vector<ESM::ContItem>::const_iterator it = items.mList.begin(); it != items.mList.end(); ++it)
    {
        if (it->mCount >= 0)
            continue;

        std::string item = Misc::StringUtils::lowerCase(it->mItem.toString());

        if (MWBase::Environment::get().getWorld()->getStore().get<ESM::ItemLevList>().search(it->mItem.toString()))
        {
            addInitialItem(item, owner, it->mCount, true);
        }
        else
        {
            int currentCount = count(item);
            if (currentCount < std::abs(it->mCount))
                addInitialItem(item, owner, std::abs(it->mCount) - currentCount, true);
        }
    }
    flagAsModified();
}

void MWWorld::ContainerStore::clear()
{
    for (ContainerStoreIterator iter (begin()); iter!=end(); ++iter)
        iter->getRefData().setCount (0);

    flagAsModified();
}

void MWWorld::ContainerStore::flagAsModified()
{
    mWeightUpToDate = false;
}

float MWWorld::ContainerStore::getWeight() const
{
    if (!mWeightUpToDate)
    {
        mCachedWeight = 0;

        mCachedWeight += getTotalWeight (potions);
        mCachedWeight += getTotalWeight (appas);
        mCachedWeight += getTotalWeight (armors);
        mCachedWeight += getTotalWeight (books);
        mCachedWeight += getTotalWeight (clothes);
        mCachedWeight += getTotalWeight (ingreds);
        mCachedWeight += getTotalWeight (lights);
        mCachedWeight += getTotalWeight (lockpicks);
        mCachedWeight += getTotalWeight (miscItems);
        mCachedWeight += getTotalWeight (probes);
        mCachedWeight += getTotalWeight (repairs);
        mCachedWeight += getTotalWeight (weapons);

        mWeightUpToDate = true;
    }

    return mCachedWeight;
}

int MWWorld::ContainerStore::getType (const Ptr& ptr)
{
    if (ptr.isEmpty())
        throw std::runtime_error ("can't put a non-existent object into a container");

    if (ptr.getTypeName()==typeid (ESM::Potion).name())
        return Type_Potion;

    if (ptr.getTypeName()==typeid (ESM::Apparatus).name())
        return Type_Apparatus;

    if (ptr.getTypeName()==typeid (ESM::Armor).name())
        return Type_Armor;

    if (ptr.getTypeName()==typeid (ESM::Book).name())
        return Type_Book;

    if (ptr.getTypeName()==typeid (ESM::Clothing).name())
        return Type_Clothing;

    if (ptr.getTypeName()==typeid (ESM::Ingredient).name())
        return Type_Ingredient;

    if (ptr.getTypeName()==typeid (ESM::Light).name())
        return Type_Light;

    if (ptr.getTypeName()==typeid (ESM::Lockpick).name())
        return Type_Lockpick;

    if (ptr.getTypeName()==typeid (ESM::Miscellaneous).name())
        return Type_Miscellaneous;

    if (ptr.getTypeName()==typeid (ESM::Probe).name())
        return Type_Probe;

    if (ptr.getTypeName()==typeid (ESM::Repair).name())
        return Type_Repair;

    if (ptr.getTypeName()==typeid (ESM::Weapon).name())
        return Type_Weapon;

    if (ptr.getTypeName() == typeid(ESM4::Apparatus).name())
        return Type_ForeignApparatus;

    if (ptr.getTypeName() == typeid(ESM4::Armor).name())
        return Type_ForeignArmor;

    if (ptr.getTypeName() == typeid(ESM4::Book).name())
        return Type_ForeignBook;

    if (ptr.getTypeName() == typeid(ESM4::Clothing).name())
        return Type_ForeignClothing;

    if (ptr.getTypeName() == typeid(ESM4::Ingredient).name())
        return Type_ForeignIngredient;

    if (ptr.getTypeName() == typeid(ESM4::Light).name())
        return Type_ForeignLight;

    if (ptr.getTypeName() == typeid(ESM4::MiscItem).name())
        return Type_ForeignMiscItem;

    if (ptr.getTypeName() == typeid(ESM4::Weapon).name())
        return Type_ForeignWeapon;

    if (ptr.getTypeName() == typeid(ESM4::Ammunition).name())
        return Type_Ammunition;

    if (ptr.getTypeName() == typeid(ESM4::SoulGem).name())
        return Type_SoulGem;

    if (ptr.getTypeName() == typeid(ESM4::Key).name())
        return Type_ForeignKey;

    if (ptr.getTypeName() == typeid(ESM4::Potion).name())
        return Type_ForeignPotion;

    if (ptr.getTypeName() == typeid(ESM4::SigilStone).name())
        return Type_SigilStone;

    if (ptr.getTypeName() == typeid(ESM4::LevelledItem).name())
        return Type_LevelledItem;

    if (ptr.getTypeName() == typeid(ESM4::Note).name())
        return Type_Note;

    //if (ptr.getTypeName() == typeid(ESM4::ItemMod).name())
        //return Type_ItemMod;

    throw std::runtime_error (
        "Object of type " + ptr.getTypeName() + " can not be placed into a container");
}

MWWorld::Ptr MWWorld::ContainerStore::search (const std::string& id)
{
    {
        Ptr ptr = searchId (potions, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (appas, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (armors, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (books, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (clothes, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (ingreds, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (lights, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (lockpicks, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (miscItems, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (probes, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (repairs, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (weapons, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    return Ptr();
}

void MWWorld::ContainerStore::writeState (ESM::InventoryState& state)
{
    state.mItems.clear();

    int index = 0;
    storeStates (potions, state, index);
    storeStates (appas, state, index);
    storeStates (armors, state, index, true);
    storeStates (books, state, index, true); // not equipable as such, but for selectedEnchantItem
    storeStates (clothes, state, index, true);
    storeStates (ingreds, state, index);
    storeStates (lockpicks, state, index, true);
    storeStates (miscItems, state, index);
    storeStates (probes, state, index, true);
    storeStates (repairs, state, index);
    storeStates (weapons, state, index, true);
    storeStates (lights, state, index, true);

    state.mLevelledItemMap = mLevelledItemMap;
}

void MWWorld::ContainerStore::readState (const ESM::InventoryState& inventory)
{
    clear();

    int index = 0;
    for (std::vector<ESM::ObjectState>::const_iterator
        iter (inventory.mItems.begin()); iter!=inventory.mItems.end(); ++iter)
    {
        const ESM::ObjectState& state = *iter;

        int type = MWBase::Environment::get().getWorld()->getStore().find(state.mRef.mRefID);

        int thisIndex = index++;

        switch (type)
        {
            case ESM::REC_ALCH: getState (potions, state); break;
            case ESM::REC_APPA: getState (appas, state); break;
            case ESM::REC_ARMO: readEquipmentState (getState (armors, state), thisIndex, inventory); break;
            case ESM::REC_BOOK: readEquipmentState (getState (books, state), thisIndex, inventory); break; // not equipable as such, but for selectedEnchantItem
            case ESM::REC_CLOT: readEquipmentState (getState (clothes, state), thisIndex, inventory); break;
            case ESM::REC_INGR: getState (ingreds, state); break;
            case ESM::REC_LOCK: readEquipmentState (getState (lockpicks, state), thisIndex, inventory); break;
            case ESM::REC_MISC: getState (miscItems, state); break;
            case ESM::REC_PROB: readEquipmentState (getState (probes, state), thisIndex, inventory); break;
            case ESM::REC_REPA: getState (repairs, state); break;
            case ESM::REC_WEAP: readEquipmentState (getState (weapons, state), thisIndex, inventory); break;
            case ESM::REC_LIGH: readEquipmentState (getState (lights, state), thisIndex, inventory); break;
            case 0:
                std::cerr << "Dropping reference to '" << state.mRef.mRefID << "' (object no longer exists)" << std::endl;
                break;
            default:
                std::cerr << "Invalid item type in inventory state, refid " << state.mRef.mRefID << std::endl;
                break;
        }
    }


    mLevelledItemMap = inventory.mLevelledItemMap;
}


MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container)
: mType (-1), mMask (0), mContainer (container)
{}

MWWorld::ContainerStoreIterator::ContainerStoreIterator (int mask, ContainerStore *container)
: mType (0), mMask (mask), mContainer (container)
{
    nextType();

    if (mType==-1 || (**this).getRefData().getCount())
        return;

    ++*this;
}

MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Potion>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Potion), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mPotion(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Apparatus>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Apparatus), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mApparatus(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Armor>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Armor), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mArmor(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Book>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Book), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mBook(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Clothing>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Clothing), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mClothing(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Ingredient>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Ingredient), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mIngredient(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Light>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Light), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLight(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Lockpick>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Lockpick), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLockpick(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Miscellaneous>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Miscellaneous), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mMiscellaneous(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Probe>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Probe), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mProbe(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Repair>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Repair), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mRepair(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Weapon>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Weapon), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mWeapon(iterator){}

MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Potion>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignPotion), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignPotion(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Apparatus>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignApparatus), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignApparatus(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Armor>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignArmor), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignArmor(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Book>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignBook), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignBook(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Clothing>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignClothing), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignClothing(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Ingredient>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignIngredient), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignIngredient(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Light>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignLight), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignLight(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::SoulGem>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_SoulGem), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mSoulGem(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::MiscItem>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignMiscItem), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignMiscItem(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Key>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignKey), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignKey(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::SigilStone>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_SigilStone), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mSigilStone(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Weapon>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_ForeignWeapon), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mForeignWeapon(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::Ammunition>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Ammunition), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mAmmunition(iterator){}
//MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM4::LevelledItem>::List::iterator iterator)
//    : mType(MWWorld::ContainerStore::Type_LevelledItem), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLevelledItem(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator(ContainerStore *container, MWWorld::CellRefList<ESM4::Note>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Note), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mNote(iterator){}
// ItemMod

MWWorld::ContainerStoreIterator::ContainerStoreIterator( const ContainerStoreIterator& src )
{
    copy(src);
}

void MWWorld::ContainerStoreIterator::incType()
{
    if (mType==0)
        mType = 1;
    else if (mType!=-1)
    {
        mType <<= 1;

        if (mType>ContainerStore::Type_Last)
            mType = -1;
    }
}

void MWWorld::ContainerStoreIterator::nextType()
{
    while (mType!=-1)
    {
        incType();

        if ((mType & mMask) && mType>0)
            if (resetIterator())
                break;
    }
}

bool MWWorld::ContainerStoreIterator::resetIterator()
{
    switch (mType)
    {
        case ContainerStore::Type_Potion:

            mPotion = mContainer->potions.mList.begin();
            return mPotion!=mContainer->potions.mList.end();

        case ContainerStore::Type_Apparatus:

            mApparatus = mContainer->appas.mList.begin();
            return mApparatus!=mContainer->appas.mList.end();

        case ContainerStore::Type_Armor:

            mArmor = mContainer->armors.mList.begin();
            return mArmor!=mContainer->armors.mList.end();

        case ContainerStore::Type_Book:

            mBook = mContainer->books.mList.begin();
            return mBook!=mContainer->books.mList.end();

        case ContainerStore::Type_Clothing:

            mClothing = mContainer->clothes.mList.begin();
            return mClothing!=mContainer->clothes.mList.end();

        case ContainerStore::Type_Ingredient:

            mIngredient = mContainer->ingreds.mList.begin();
            return mIngredient!=mContainer->ingreds.mList.end();

        case ContainerStore::Type_Light:

            mLight = mContainer->lights.mList.begin();
            return mLight!=mContainer->lights.mList.end();

        case ContainerStore::Type_Lockpick:

            mLockpick = mContainer->lockpicks.mList.begin();
            return mLockpick!=mContainer->lockpicks.mList.end();

        case ContainerStore::Type_Miscellaneous:

            mMiscellaneous = mContainer->miscItems.mList.begin();
            return mMiscellaneous!=mContainer->miscItems.mList.end();

        case ContainerStore::Type_Probe:

            mProbe = mContainer->probes.mList.begin();
            return mProbe!=mContainer->probes.mList.end();

        case ContainerStore::Type_Repair:

            mRepair = mContainer->repairs.mList.begin();
            return mRepair!=mContainer->repairs.mList.end();

        case ContainerStore::Type_Weapon:

            mWeapon = mContainer->weapons.mList.begin();
            return mWeapon!=mContainer->weapons.mList.end();

        case ContainerStore::Type_ForeignPotion:

            mForeignPotion = mContainer->foreignPotions.mList.begin();
            return mForeignPotion!=mContainer->foreignPotions.mList.end();

        case ContainerStore::Type_ForeignApparatus:

            mForeignApparatus = mContainer->foreignAppas.mList.begin();
            return mForeignApparatus!=mContainer->foreignAppas.mList.end();

        case ContainerStore::Type_ForeignArmor:

            mForeignArmor = mContainer->foreignArmors.mList.begin();
            return mForeignArmor!=mContainer->foreignArmors.mList.end();

        case ContainerStore::Type_ForeignBook:

            mForeignBook = mContainer->foreignBooks.mList.begin();
            return mForeignBook!=mContainer->foreignBooks.mList.end();

        case ContainerStore::Type_ForeignClothing:

            mForeignClothing = mContainer->foreignClothes.mList.begin();
            return mForeignClothing!=mContainer->foreignClothes.mList.end();

        case ContainerStore::Type_ForeignIngredient:

            mForeignIngredient = mContainer->foreignIngredients.mList.begin();
            return mForeignIngredient!=mContainer->foreignIngredients.mList.end();

        case ContainerStore::Type_ForeignLight:

            mForeignLight = mContainer->foreignLights.mList.begin();
            return mForeignLight!=mContainer->foreignLights.mList.end();

        case ContainerStore::Type_SoulGem:

            mSoulGem = mContainer->soulGems.mList.begin();
            return mSoulGem!=mContainer->soulGems.mList.end();

        case ContainerStore::Type_ForeignMiscItem:

            mForeignMiscItem = mContainer->foreignMiscItems.mList.begin();
            return mForeignMiscItem!=mContainer->foreignMiscItems.mList.end();

        case ContainerStore::Type_ForeignKey:

            mForeignKey = mContainer->foreignKeys.mList.begin();
            return mForeignKey!=mContainer->foreignKeys.mList.end();

        case ContainerStore::Type_SigilStone:

            mSigilStone = mContainer->sigilStones.mList.begin();
            return mSigilStone!=mContainer->sigilStones.mList.end();

        case ContainerStore::Type_ForeignWeapon:

            mForeignWeapon = mContainer->foreignWeapons.mList.begin();
            return mForeignWeapon!=mContainer->foreignWeapons.mList.end();

        case ContainerStore::Type_Ammunition:

            mAmmunition = mContainer->ammunitions.mList.begin();
            return mAmmunition!=mContainer->ammunitions.mList.end();

        //case ContainerStore::Type_LevelledItem:

            //mLevelledItem = mContainer->foreignLvlItems.mList.begin();
            //return mLevelledItem!=mContainer->foreignLvlItems.mList.end();

        case ContainerStore::Type_Note:

            mNote = mContainer->notes.mList.begin();
            return mNote!=mContainer->notes.mList.end();

        //case ContainerStore::Type_ItemMod:

            //mItemMod = mContainer->itemMods.mList.begin();
            //return mItemMod!=mContainer->itemMods.mList.end();
    }

    return false;
}

bool MWWorld::ContainerStoreIterator::incIterator()
{
    switch (mType)
    {
        case ContainerStore::Type_Potion:

            ++mPotion;
            return mPotion==mContainer->potions.mList.end();

        case ContainerStore::Type_Apparatus:

            ++mApparatus;
            return mApparatus==mContainer->appas.mList.end();

        case ContainerStore::Type_Armor:

            ++mArmor;
            return mArmor==mContainer->armors.mList.end();

        case ContainerStore::Type_Book:

            ++mBook;
            return mBook==mContainer->books.mList.end();

        case ContainerStore::Type_Clothing:

            ++mClothing;
            return mClothing==mContainer->clothes.mList.end();

        case ContainerStore::Type_Ingredient:

            ++mIngredient;
            return mIngredient==mContainer->ingreds.mList.end();

        case ContainerStore::Type_Light:

            ++mLight;
            return mLight==mContainer->lights.mList.end();

        case ContainerStore::Type_Lockpick:

            ++mLockpick;
            return mLockpick==mContainer->lockpicks.mList.end();

        case ContainerStore::Type_Miscellaneous:

            ++mMiscellaneous;
            return mMiscellaneous==mContainer->miscItems.mList.end();

        case ContainerStore::Type_Probe:

            ++mProbe;
            return mProbe==mContainer->probes.mList.end();

        case ContainerStore::Type_Repair:

            ++mRepair;
            return mRepair==mContainer->repairs.mList.end();

        case ContainerStore::Type_Weapon:

            ++mWeapon;
            return mWeapon==mContainer->weapons.mList.end();

        case ContainerStore::Type_ForeignPotion:

            ++mForeignPotion;
            return mForeignPotion==mContainer->foreignPotions.mList.end();

        case ContainerStore::Type_ForeignApparatus:

            ++mForeignApparatus;
            return mForeignApparatus==mContainer->foreignAppas.mList.end();

        case ContainerStore::Type_ForeignArmor:

            ++mForeignArmor;
            return mForeignArmor==mContainer->foreignArmors.mList.end();

        case ContainerStore::Type_ForeignBook:

            ++mForeignBook;
            return mForeignBook==mContainer->foreignBooks.mList.end();

        case ContainerStore::Type_ForeignClothing:

            ++mForeignClothing;
            return mForeignClothing==mContainer->foreignClothes.mList.end();

        case ContainerStore::Type_ForeignIngredient:

            ++mForeignIngredient;
            return mForeignIngredient==mContainer->foreignIngredients.mList.end();

        case ContainerStore::Type_ForeignLight:

            ++mForeignLight;
            return mForeignLight==mContainer->foreignLights.mList.end();

        case ContainerStore::Type_SoulGem:

            ++mSoulGem;
            return mSoulGem==mContainer->soulGems.mList.end();

        case ContainerStore::Type_ForeignMiscItem:

            ++mForeignMiscItem;
            return mForeignMiscItem==mContainer->foreignMiscItems.mList.end();

        case ContainerStore::Type_ForeignKey:

            ++mForeignKey;
            return mForeignKey==mContainer->foreignKeys.mList.end();

        case ContainerStore::Type_SigilStone:

            ++mSigilStone;
            return mSigilStone==mContainer->sigilStones.mList.end();

        case ContainerStore::Type_ForeignWeapon:

            ++mForeignWeapon;
            return mForeignWeapon==mContainer->foreignWeapons.mList.end();

        case ContainerStore::Type_Ammunition:

            ++mAmmunition;
            return mAmmunition==mContainer->ammunitions.mList.end();

        case ContainerStore::Type_Note:

            ++mNote;
            return mNote==mContainer->notes.mList.end();

        //case ContainerStore::Type_ItemMod:

            //++mItemMod;
            //return mItemMod==mContainer->itemMods.mList.end();
    }

    return true;
}

MWWorld::Ptr *MWWorld::ContainerStoreIterator::operator->() const
{
    mPtr = **this;
    return &mPtr;
}

MWWorld::Ptr MWWorld::ContainerStoreIterator::operator*() const
{
    Ptr ptr;

    switch (mType)
    {
        case ContainerStore::Type_Potion: ptr = MWWorld::Ptr (&*mPotion, 0); break;
        case ContainerStore::Type_Apparatus: ptr = MWWorld::Ptr (&*mApparatus, 0); break;
        case ContainerStore::Type_Armor: ptr = MWWorld::Ptr (&*mArmor, 0); break;
        case ContainerStore::Type_Book: ptr = MWWorld::Ptr (&*mBook, 0); break;
        case ContainerStore::Type_Clothing: ptr = MWWorld::Ptr (&*mClothing, 0); break;
        case ContainerStore::Type_Ingredient: ptr = MWWorld::Ptr (&*mIngredient, 0); break;
        case ContainerStore::Type_Light: ptr = MWWorld::Ptr (&*mLight, 0); break;
        case ContainerStore::Type_Lockpick: ptr = MWWorld::Ptr (&*mLockpick, 0); break;
        case ContainerStore::Type_Miscellaneous: ptr = MWWorld::Ptr (&*mMiscellaneous, 0); break;
        case ContainerStore::Type_Probe: ptr = MWWorld::Ptr (&*mProbe, 0); break;
        case ContainerStore::Type_Repair: ptr = MWWorld::Ptr (&*mRepair, 0); break;
        case ContainerStore::Type_Weapon: ptr = MWWorld::Ptr (&*mWeapon, 0); break;

        case ContainerStore::Type_ForeignPotion: ptr = MWWorld::Ptr (&*mForeignPotion, 0); break;
        case ContainerStore::Type_ForeignApparatus: ptr = MWWorld::Ptr (&*mForeignApparatus, 0); break;
        case ContainerStore::Type_ForeignArmor: ptr = MWWorld::Ptr (&*mForeignArmor, 0); break;
        case ContainerStore::Type_ForeignBook: ptr = MWWorld::Ptr (&*mForeignBook, 0); break;
        case ContainerStore::Type_ForeignClothing: ptr = MWWorld::Ptr (&*mForeignClothing, 0); break;
        case ContainerStore::Type_ForeignIngredient: ptr = MWWorld::Ptr (&*mForeignIngredient, 0); break;
        case ContainerStore::Type_ForeignLight: ptr = MWWorld::Ptr (&*mForeignLight, 0); break;
        case ContainerStore::Type_SoulGem: ptr = MWWorld::Ptr (&*mSoulGem, 0); break;
        case ContainerStore::Type_ForeignMiscItem: ptr = MWWorld::Ptr (&*mForeignMiscItem, 0); break;
        case ContainerStore::Type_ForeignKey: ptr = MWWorld::Ptr (&*mForeignKey, 0); break;
        case ContainerStore::Type_SigilStone: ptr = MWWorld::Ptr (&*mSigilStone, 0); break;
        case ContainerStore::Type_ForeignWeapon: ptr = MWWorld::Ptr (&*mForeignWeapon, 0); break;
        case ContainerStore::Type_Ammunition: ptr = MWWorld::Ptr (&*mAmmunition, 0); break;
        //case ContainerStore::Type_LevelledItem: ptr = MWWorld::Ptr (&*mLevelledItem, 0); break;
        case ContainerStore::Type_Note: ptr = MWWorld::Ptr (&*mNote, 0); break;
        //case ContainerStore::Type_ItemMod: ptr = MWWorld::Ptr (&*mItemMod, 0); break;
    }

    if (ptr.isEmpty())
        throw std::runtime_error ("invalid iterator");

    ptr.setContainerStore (mContainer);

    return ptr;
}

MWWorld::ContainerStoreIterator& MWWorld::ContainerStoreIterator::operator++()
{
    do
    {
        if (incIterator())
            nextType();
    }
    while (mType!=-1 && !(**this).getRefData().getCount());

    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStoreIterator::operator++ (int)
{
    ContainerStoreIterator iter (*this);
    ++*this;
    return iter;
}

bool MWWorld::ContainerStoreIterator::isEqual (const ContainerStoreIterator& iter) const
{
    if (mContainer!=iter.mContainer)
        return false;

    if (mType!=iter.mType)
        return false;

    switch (mType)
    {
        case ContainerStore::Type_Potion: return mPotion==iter.mPotion;
        case ContainerStore::Type_Apparatus: return mApparatus==iter.mApparatus;
        case ContainerStore::Type_Armor: return mArmor==iter.mArmor;
        case ContainerStore::Type_Book: return mBook==iter.mBook;
        case ContainerStore::Type_Clothing: return mClothing==iter.mClothing;
        case ContainerStore::Type_Ingredient: return mIngredient==iter.mIngredient;
        case ContainerStore::Type_Light: return mLight==iter.mLight;
        case ContainerStore::Type_Lockpick: return mLockpick==iter.mLockpick;
        case ContainerStore::Type_Miscellaneous: return mMiscellaneous==iter.mMiscellaneous;
        case ContainerStore::Type_Probe: return mProbe==iter.mProbe;
        case ContainerStore::Type_Repair: return mRepair==iter.mRepair;
        case ContainerStore::Type_Weapon: return mWeapon==iter.mWeapon;

        case ContainerStore::Type_ForeignPotion: return mForeignPotion==iter.mForeignPotion;
        case ContainerStore::Type_ForeignApparatus: return mForeignApparatus==iter.mForeignApparatus;
        case ContainerStore::Type_ForeignArmor: return mForeignArmor==iter.mForeignArmor;
        case ContainerStore::Type_ForeignBook: return mForeignBook==iter.mForeignBook;
        case ContainerStore::Type_ForeignClothing: return mForeignClothing==iter.mForeignClothing;
        case ContainerStore::Type_ForeignIngredient: return mForeignIngredient==iter.mForeignIngredient;
        case ContainerStore::Type_ForeignLight: return mForeignLight==iter.mForeignLight;
        case ContainerStore::Type_SoulGem: return mSoulGem==iter.mSoulGem;
        case ContainerStore::Type_ForeignMiscItem: return mForeignMiscItem==iter.mForeignMiscItem;
        case ContainerStore::Type_ForeignKey: return mForeignKey==iter.mForeignKey;
        case ContainerStore::Type_SigilStone: return mSigilStone==iter.mSigilStone;
        case ContainerStore::Type_ForeignWeapon: return mForeignWeapon==iter.mForeignWeapon;
        case ContainerStore::Type_Ammunition: return mAmmunition==iter.mAmmunition;
        //case ContainerStore::Type_LevelledItem: return mLevelledItem==iter.mLevelledItem;
        case ContainerStore::Type_Note: return mNote==iter.mNote;
        //case ContainerStore::Type_ItemMod: return mItemMod==iter.mItemMod;
        case -1: return true;
    }

    return false;
}

int MWWorld::ContainerStoreIterator::getType() const
{
    return mType;
}

const MWWorld::ContainerStore *MWWorld::ContainerStoreIterator::getContainerStore() const
{
    return mContainer;
}

void MWWorld::ContainerStoreIterator::copy(const ContainerStoreIterator& src)
{
    mType = src.mType;
    mMask = src.mMask;
    mContainer = src.mContainer;
    mPtr = src.mPtr;

    switch (mType)
    {
        case MWWorld::ContainerStore::Type_Potion: mPotion = src.mPotion; break;
        case MWWorld::ContainerStore::Type_Apparatus: mApparatus = src.mApparatus; break;
        case MWWorld::ContainerStore::Type_Armor: mArmor = src.mArmor; break;
        case MWWorld::ContainerStore::Type_Book: mBook = src.mBook; break;
        case MWWorld::ContainerStore::Type_Clothing: mClothing = src.mClothing; break;
        case MWWorld::ContainerStore::Type_Ingredient: mIngredient = src.mIngredient; break;
        case MWWorld::ContainerStore::Type_Light: mLight = src.mLight; break;
        case MWWorld::ContainerStore::Type_Lockpick: mLockpick = src.mLockpick; break;
        case MWWorld::ContainerStore::Type_Miscellaneous: mMiscellaneous = src.mMiscellaneous; break;
        case MWWorld::ContainerStore::Type_Probe: mProbe = src.mProbe; break;
        case MWWorld::ContainerStore::Type_Repair: mRepair = src.mRepair; break;
        case MWWorld::ContainerStore::Type_Weapon: mWeapon = src.mWeapon; break;

        case MWWorld::ContainerStore::Type_ForeignPotion: mForeignPotion = src.mForeignPotion; break;
        case MWWorld::ContainerStore::Type_ForeignApparatus: mForeignApparatus = src.mForeignApparatus; break;
        case MWWorld::ContainerStore::Type_ForeignArmor: mForeignArmor = src.mForeignArmor; break;
        case MWWorld::ContainerStore::Type_ForeignBook: mForeignBook = src.mForeignBook; break;
        case MWWorld::ContainerStore::Type_ForeignClothing: mForeignClothing = src.mForeignClothing; break;
        case MWWorld::ContainerStore::Type_ForeignIngredient: mForeignIngredient = src.mForeignIngredient; break;
        case MWWorld::ContainerStore::Type_ForeignLight: mForeignLight = src.mForeignLight; break;
        case MWWorld::ContainerStore::Type_SoulGem: mSoulGem = src.mSoulGem; break;
        case MWWorld::ContainerStore::Type_ForeignMiscItem: mForeignMiscItem = src.mForeignMiscItem; break;
        case MWWorld::ContainerStore::Type_ForeignKey: mForeignKey = src.mForeignKey; break;
        case MWWorld::ContainerStore::Type_SigilStone: mSigilStone = src.mSigilStone; break;
        case MWWorld::ContainerStore::Type_ForeignWeapon: mForeignWeapon = src.mForeignWeapon; break;
        case MWWorld::ContainerStore::Type_Ammunition: mAmmunition = src.mAmmunition; break;
        //case MWWorld::ContainerStore::Type_LevelledItem: mLevelledItem = src.mLevelledItem; break;
        case MWWorld::ContainerStore::Type_Note: mNote = src.mNote; break;
        //case MWWorld::ContainerStore::Type_ItemMod: mItemMod = src.mItemMod; break;
        case -1: break;
        default: assert(0);
    }
}

MWWorld::ContainerStoreIterator& MWWorld::ContainerStoreIterator::operator=( const ContainerStoreIterator& rhs )
{
    if (this!=&rhs)
    {
        copy(rhs);
    }
    return *this;
}

bool MWWorld::operator== (const ContainerStoreIterator& left, const ContainerStoreIterator& right)
{
    return left.isEqual (right);
}

bool MWWorld::operator!= (const ContainerStoreIterator& left, const ContainerStoreIterator& right)
{
    return !(left==right);
}
