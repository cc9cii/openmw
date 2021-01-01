#ifndef OPENMW_MECHANICS_LEVELLEDLIST_H
#define OPENMW_MECHANICS_LEVELLEDLIST_H

#include <components/misc/rng.hpp>

#include <extern/esm4/common.hpp>
#include <extern/esm4/formid.hpp>
#include <extern/esm4/crea.hpp>

#include <iostream>


#include "../mwworld/ptr.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWMechanics
{

    /// @return ID of resulting item, or empty if none
    inline std::string getLevelledItem (const ESM::LevelledListBase* levItem, bool creature, unsigned char failChance=0)
    {
        const std::vector<ESM::LevelledListBase::LevelItem>& items = levItem->mList;

        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerLevel = player.getClass().getCreatureStats(player).getLevel();

        failChance += levItem->mChanceNone;

        if (Misc::Rng::roll0to99() < failChance)
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (it->mLevel > highestLevel && it->mLevel <= playerLevel)
                highestLevel = it->mLevel;
        }

        // For levelled creatures, the flags are swapped. This file format just makes so much sense.
        bool allLevels = (levItem->mFlags & ESM::ItemLevList::AllLevels) != 0;
        if (creature)
            allLevels = levItem->mFlags & ESM::CreatureLevList::AllLevels;

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (playerLevel >= it->mLevel
                    && (allLevels || it->mLevel == highestLevel))
            {
                candidates.push_back(it->mId);
                if (it->mLevel >= highest.first)
                    highest = std::make_pair(it->mLevel, it->mId);
            }
        }
        if (candidates.empty())
            return std::string();
        std::string item = candidates[Misc::Rng::rollDice(int(candidates.size()))];

        // Vanilla doesn't fail on nonexistent items in levelled lists
        if (!MWBase::Environment::get().getWorld()->getStore().find(Misc::StringUtils::lowerCase(item)))
        {
            std::cerr << "Warning: ignoring nonexistent item '" << item << "' in levelled list '" << levItem->mId << "'" << std::endl;
            return std::string();
        }

        // Is this another levelled item or a real item?
        MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), item, 1);
        if (ref.getPtr().getTypeName() != typeid(ESM::ItemLevList).name()
                && ref.getPtr().getTypeName() != typeid(ESM::CreatureLevList).name())
        {
            return item;
        }
        else
        {
            if (ref.getPtr().getTypeName() == typeid(ESM::ItemLevList).name())
                return getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, false, failChance);
            else
                return getLevelledItem(ref.getPtr().get<ESM::CreatureLevList>()->mBase, true, failChance);
        }
    }

    // NOTE: "calculate each item" is done in ContainerStore::addInitialitem()
    inline std::string getLeveledObject(const std::vector<ESM4::LVLO>& items, bool allLevels, int8_t failChance)
    {
        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        // FIXME: temporarily increased player level to help with testing Stmorm Atronach animations
        int playerLevel = 19;//player.getClass().getCreatureStats(player).getLevel();

        // FIXME: FO3/FONV/TES5 have LVLI with failChance of 100 - may have a special meaning
        if (Misc::Rng::roll0to99() < failChance) // failChance [0..99] for TES4
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (std::vector<ESM4::LVLO>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (it->level > highestLevel && it->level <= playerLevel)
                highestLevel = it->level;
        }

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (std::vector<ESM4::LVLO>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (playerLevel >= it->level && (allLevels || it->level == highestLevel))
            {
                candidates.push_back(ESM4::formIdToString(it->item));
                if (it->level >= highest.first)
                    highest = std::make_pair(it->level, ESM4::formIdToString(it->item));
#if 0
                // FIXME
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), ESM4::formIdToString(it->item), 1);
                if (ref.getPtr().getTypeName() == typeid(ESM4::LevelledItem).name()) // only print LVLI
                    std::cout << "candidate " << ref.getPtr().get<ESM4::LevelledItem>()->mBase->mEditorId << std::endl;
#endif
            }
        }

        if (candidates.empty())
            return std::string();

        return candidates[Misc::Rng::rollDice(int(candidates.size()))];
    }

    inline void getTES4LevelledItem(std::vector<std::string>& ids, const ESM4::LevelledItem* levItem)
    {
        const std::vector<ESM4::LVLO>& items = levItem->mLvlObject;
        bool allLevels = levItem->calcAllLvlLessThanPlayer();
        std::int8_t failChance = levItem->chanceNone();
        bool useAll = levItem->useAll();
        if (useAll)
        {
            for (std::size_t i = 0; i < items.size(); ++i)
            {
                std::string itemId = ESM4::formIdToString(items[i].item);
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemId, 1);
                if (ref.getPtr().getTypeName() == typeid(ESM4::LevelledItem).name())
                {
                    getTES4LevelledItem(ids, ref.getPtr().get<ESM4::LevelledItem>()->mBase);
                }
                else
                    ids.push_back(itemId);
            }
        }
        else
        {
            std::string itemId = getLeveledObject(items, allLevels, failChance);
            if (itemId.empty())
                return;

            // copy Morrowind behaviour for now
            if (!MWBase::Environment::get().getWorld()->getStore().getRecordType(ESM4::stringToFormId(itemId)))
            {
                std::cerr << "Warning: LVLI ignoring nonexistent item '" << itemId << "' in levelled list '" <<
                    ESM4::formIdToString(levItem->mFormId) << "'" << std::endl;

                return;
            }

            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemId, 1);

            if (ref.getPtr().getTypeName() == typeid(ESM4::LevelledItem).name())
            {
#if 0
                std::cout << ref.getPtr().get<ESM4::LevelledItem>()->mBase->mEditorId << std::endl; // FIXME:
#endif
                getTES4LevelledItem(ids, ref.getPtr().get<ESM4::LevelledItem>()->mBase);
            }
            else
            {
//#if 0
                // FO3 COC "SuperDuperMart" sometime spawns BookGeneric05, etc
                if (ref.getPtr().getTypeName() == typeid(ESM4::Book).name())
                    std::cout << "leveled book " << ref.getPtr().get<ESM4::Book>()->mBase->mEditorId << std::endl; // FIXME:
//#endif
                ids.push_back(itemId);
            }
        }
    }

    inline std::string getFO3LevelledNpc(const ESM4::LevelledNpc* levActor);

    inline std::string getFO3LevelledNpc(const ESM4::Npc* npc)
    {
        if (npc && npc->mBaseTemplate != 0
                && (npc->mModel.empty() || npc->mModel == "marker_creature.nif"))
        {
            ESM4::FormId base = npc->mBaseTemplate;
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            uint32_t type = store.getRecordType(base);
            if (type == ESM4::REC_NPC_)
            {
                const ESM4::Npc* newNpc = store.getForeign<ESM4::Npc>().search(base);
                std::cout << "newNpc " << newNpc->mEditorId << std::endl; // FIXME

                return getFO3LevelledNpc(newNpc);
            }
            else if (type == ESM4::REC_LVLN)
            {
                const ESM4::LevelledNpc* lvlActor = store.getForeign<ESM4::LevelledNpc>().search(base);
                //std::cout << "newlvl " << lvlActor->mEditorId << std::endl; // FIXME

                return getFO3LevelledNpc(lvlActor);
            }
            else
                throw std::runtime_error ("levelled actor not found!");
        }
        else
            return ESM4::formIdToString(npc->mFormId);
    }

    inline std::string getFO3LevelledNpc(const ESM4::LevelledNpc* levActor)
    {
        const std::vector<ESM4::LVLO>& actors = levActor->mLvlObject;
        bool allLevels = levActor->calcAllLvlLessThanPlayer();
        std::int8_t failChance = levActor->chanceNone(); // seems to be always 0

        std::string npcName = getLeveledObject(actors, allLevels, failChance);

        if (npcName.empty())
            return std::string();

        ESM4::FormId id = ESM4::stringToFormId(npcName);
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

        uint32_t type = store.getRecordType(id);
        if (type == ESM4::REC_NPC_)
        {
            const ESM4::Npc* newNpc = store.getForeign<ESM4::Npc>().search(id);
            std::cout << "newNpc " << newNpc->mEditorId << std::endl; // FIXME

            return getFO3LevelledNpc(newNpc);
        }
        else if (type == ESM4::REC_LVLN)
        {
            const ESM4::LevelledNpc* lvlActor = store.getForeign<ESM4::LevelledNpc>().search(id);
            //std::cout << "newlvl " << lvlActor->mEditorId << std::endl; // FIXME

            return getFO3LevelledNpc(lvlActor);
        }
        else if (type == 0)
        {
            std::cerr << "Warning: LVLN ignoring nonexistent item '" << npcName << "' in levelled list '" <<
                levActor->mEditorId << "'" << std::endl;

            return std::string();
        }
        else
            throw std::runtime_error ("levelled actor not found!");
            //return std::string();
    }

    inline std::string getFO3LevelledCreature(const ESM4::LevelledCreature* levCreature);

    inline std::string getFO3LevelledCreature(const ESM4::Creature* creature)
    {
        if (creature && creature->mBaseTemplate != 0
                && (creature->mModel.empty() || creature->mModel == "marker_creature.nif"))
        {
            ESM4::FormId base = creature->mBaseTemplate;
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            uint32_t type = store.getRecordType(base);
            if (type == ESM4::REC_CREA)
            {
                const ESM4::Creature* newCreature = store.getForeign<ESM4::Creature>().search(base);
                std::cout << "newCreature " << newCreature->mEditorId << std::endl; // FIXME

                return getFO3LevelledCreature(newCreature);
            }
            else if (type == ESM4::REC_LVLC)
            {
                const ESM4::LevelledCreature* lvlCreature = store.getForeign<ESM4::LevelledCreature>().search(base);
                //std::cout << "newlvl " << lvlCreature->mEditorId << std::endl; // FIXME

                return getFO3LevelledCreature(lvlCreature);
            }
            else
                throw std::runtime_error ("levelled creature not found!");
        }
        else
            return ESM4::formIdToString(creature->mFormId);
    }

    inline std::string getFO3LevelledCreature(const ESM4::LevelledCreature* levCreature)
    {
        const std::vector<ESM4::LVLO>& actors = levCreature->mLvlObject;
        bool allLevels = levCreature->calcAllLvlLessThanPlayer();
        std::int8_t failChance = levCreature->chanceNone(); // seems to be always 0

        std::string creatureName = getLeveledObject(actors, allLevels, failChance);

        if (creatureName.empty())
            return std::string();

        ESM4::FormId id = ESM4::stringToFormId(creatureName);
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

        uint32_t type = store.getRecordType(id);
        if (type == ESM4::REC_CREA)
        {
            const ESM4::Creature* newCreature = store.getForeign<ESM4::Creature>().search(id);
            std::cout << "newCreature " << newCreature->mEditorId << std::endl; // FIXME

            return getFO3LevelledCreature(newCreature);
        }
        else if (type == ESM4::REC_LVLC)
        {
            const ESM4::LevelledCreature* lvlCreature = store.getForeign<ESM4::LevelledCreature>().search(id);
            //std::cout << "newlvl " << lvlCreature->mEditorId << std::endl; // FIXME

            return getFO3LevelledCreature(lvlCreature);
        }
        else if (type == 0)
        {
            std::cerr << "Warning: LVLC ignoring nonexistent item '" << creatureName << "' in levelled list '" <<
                levCreature->mEditorId << "'" << std::endl;

            return std::string();
        }
        else
            throw std::runtime_error ("levelled creature not found!");
            //return std::string();
    }

    inline std::string getTES4LevelledNpc(const ESM4::LevelledNpc* levActor)
    {
        const std::vector<ESM4::LVLO>& items = levActor->mLvlObject;
        bool allLevels = levActor->calcAllLvlLessThanPlayer();
        std::int8_t failChance = levActor->chanceNone(); // seems to be always 0

        std::string itemId = getLeveledObject(items, allLevels, failChance);

        if (itemId.empty())
            return std::string();

        // copy Morrowind behaviour for now
        if (!MWBase::Environment::get().getWorld()->getStore().getRecordType(ESM4::stringToFormId(itemId)))
        {
            std::cerr << "Warning: ignoring nonexistent actor '" << itemId << "' in levelled list '" <<
                ESM4::formIdToString(levActor->mFormId) << "'" << std::endl;

            return std::string();
        }

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemId, 1);

        std::string typeName = ref.getPtr().getTypeName();
        if (typeName == typeid(ESM4::Npc).name())
        {
            return itemId;
        }
        else if (typeName == typeid(ESM4::LevelledNpc).name())
        {
            return getTES4LevelledNpc(ref.getPtr().get<ESM4::LevelledNpc>()->mBase);
        }
        else
            return std::string();
    }

    // FIXME: duplicated code, use a template?
    inline std::string getTES4LevelledCreature(const ESM4::LevelledCreature* levCreature)
    {
        const std::vector<ESM4::LVLO>& items = levCreature->mLvlObject;
        bool allLevels = levCreature->calcAllLvlLessThanPlayer();
        std::int8_t failChance = levCreature->chanceNone(); // seems to be always 0

        std::string itemId = getLeveledObject(items, allLevels, failChance);

        if (itemId.empty())
            return std::string();

        // copy Morrowind behaviour for now
        if (!MWBase::Environment::get().getWorld()->getStore().getRecordType(ESM4::stringToFormId(itemId)))
        {
            std::cerr << "Warning: LVLC ignoring nonexistent creature '" << itemId << "' in levelled list '" <<
                ESM4::formIdToString(levCreature->mFormId) << "'" << std::endl;

            return std::string();
        }

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemId, 1);

        std::string typeName = ref.getPtr().getTypeName();
        if (typeName == typeid(ESM4::Creature).name() || typeName == typeid(ESM4::Npc).name())
        {
            return itemId;
        }
        else if (typeName == typeid(ESM4::LevelledCreature).name())
        {
            return getTES4LevelledCreature(ref.getPtr().get<ESM4::LevelledCreature>()->mBase);
        }
        else
        {
            std::cerr << "LVLC nothing found?" << std::endl;
            return std::string();
        }
    }
}

#endif
