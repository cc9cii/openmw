#include "compilercontext.hpp"

#include "../mwworld/esmstore.hpp"

#include <extern/esm4/scpt.hpp>
#include <extern/esm4/qust.hpp>
//#include <extern/esm4/npc_.hpp>

#include <components/esm/loaddial.hpp>

#include <components/compiler/locals.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

namespace MWScript
{
    CompilerContext::CompilerContext (Type type)
    : mType (type)
    {}

    bool CompilerContext::canDeclareLocals() const
    {
        return mType==Type_Full;
    }

    char CompilerContext::getGlobalType (const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalVariableType (name);
    }

    // NOTE: assumes that the ScriptManager knows TES4 script names in the string form of FormId
    std::pair<char, bool> CompilerContext::getMemberType (const std::string& name,
        const std::string& id) const
    {
        std::string script;
        bool reference = false; // FIXME: how do we know if the script is local or global?

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        if (const ESM4::Script *scriptRecord = store.getForeign<ESM4::Script>().search (id))
        {
            // FIXME: does this ever occur?  also it will fail since the search is case sensitive
            script = ESM4::formIdToString(scriptRecord->mFormId);
            //reference = true; // this means not global scripts
        }
        else if (const ESM4::Quest* questRecord = store.getForeign<ESM4::Quest>().search(id))
        {
            script = ESM4::formIdToString(questRecord->mQuestScript);
            // quests are global scripts so reference is kept false
        }
        else if (const ESM4::ActorCharacter *achr = store.getForeign<ESM4::ActorCharacter>().searchLower(id))
        {
            // maybe a member access of a reference e.g. SEHaskillRef.summoned in SE02QuestScript
            ESM4::FormId baseObj = achr->mBaseObj;
            int baseType = store.getRecordType(baseObj);
            switch (baseType)
            {
                case ESM4::REC_NPC_:
                {
                    const ESM4::Npc *npc = store.getForeign<ESM4::Npc>().search(baseObj);
                    if (npc)
                    {
                        ESM4::FormId scriptId = npc->mScriptId;
                        const ESM4::Script* scriptRecord = store.getForeign<ESM4::Script>().search(scriptId);
                        if (scriptRecord)
                            script = ESM4::formIdToString(scriptRecord->mFormId);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else if (const ESM4::Reference* ref = store.getForeign<ESM4::Reference>().searchLower(id))
        {
            ESM4::FormId baseObj = ref->mBaseObj;
            int baseType = store.getRecordType(baseObj);
            switch (baseType)
            {
                case ESM4::REC_ACTI: //e.g. DASheogorathShrineREF in DASheogorathScript
                {
                    const ESM4::Activator *acti = store.getForeign<ESM4::Activator>().search(baseObj);
                    if (acti)
                    {
                        ESM4::FormId scriptId = acti->mScriptId;
                        const ESM4::Script* scriptRecord2 = store.getForeign<ESM4::Script>().search(scriptId);
                        if (scriptRecord2)
                            script = ESM4::formIdToString(scriptRecord2->mFormId);
                    }
                    break;
                }
                case ESM4::REC_DOOR: // e.g. ICArenaMatchGateRef in ArenaScript
                {
                    const ESM4::Door *door = store.getForeign<ESM4::Door>().search(baseObj);
                    if (door)
                    {
                        ESM4::FormId scriptId = door->mScriptId;
                        const ESM4::Script* scriptRecord2 = store.getForeign<ESM4::Script>().search(scriptId);
                        if (scriptRecord2)
                            script = ESM4::formIdToString(scriptRecord2->mFormId);
                    }
                    break;
                }
                case ESM4::REC_CONT: // e.g. MS51PaintSuppliesRef (in ChorrolCastlePrivateQuarters) in MS51script
                {
                    const ESM4::Container *cont = store.getForeign<ESM4::Container>().search(baseObj);
                    if (cont)
                    {
                        ESM4::FormId scriptId = cont->mScriptId;
                        const ESM4::Script* scriptRecord2 = store.getForeign<ESM4::Script>().search(scriptId);
                        if (scriptRecord2)
                            script = ESM4::formIdToString(scriptRecord2->mFormId);
                    }
                    break;
                }
                case ESM4::REC_WEAP: // e.g. goblinheadCrackedWood in MS46Script
                {
                    const ESM4::Weapon *weap = store.getForeign<ESM4::Weapon>().search(baseObj);
                    if (weap)
                    {
                        ESM4::FormId scriptId = weap->mScriptId;
                        const ESM4::Script* scriptRecord2 = store.getForeign<ESM4::Script>().search(scriptId);
                        if (scriptRecord2)
                            script = ESM4::formIdToString(scriptRecord2->mFormId);
                    }
                    break;
                }
                default:
                    break;
            }
        }

        char type = ' ';

        if (!script.empty())
            type = MWBase::Environment::get().getScriptManager()->getLocals (script).getType (
                Misc::StringUtils::lowerCase (name));

        return std::make_pair (type, reference);
    }

    bool CompilerContext::isId (const std::string& name) const
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        return
            store.get<ESM::Activator>().search (name) ||
            store.get<ESM::Potion>().search (name) ||
            store.get<ESM::Apparatus>().search (name) ||
            store.get<ESM::Armor>().search (name) ||
            store.get<ESM::Book>().search (name) ||
            store.get<ESM::Clothing>().search (name) ||
            store.get<ESM::Container>().search (name) ||
            store.get<ESM::Creature>().search (name) ||
            store.get<ESM::Door>().search (name) ||
            store.get<ESM::Ingredient>().search (name) ||
            store.get<ESM::CreatureLevList>().search (name) ||
            store.get<ESM::ItemLevList>().search (name) ||
            store.get<ESM::Light>().search (name) ||
            store.get<ESM::Lockpick>().search (name) ||
            store.get<ESM::Miscellaneous>().search (name) ||
            store.get<ESM::NPC>().search (name) ||
            store.get<ESM::Probe>().search (name) ||
            store.get<ESM::Repair>().search (name) ||
            store.get<ESM::Static>().search (name) ||
            store.get<ESM::Weapon>().search (name);
    }

    ESM4::FormId CompilerContext::getReference (const std::string& lowerEditorId) const
    {
        if (lowerEditorId == "player")
            return 0x121212; // FIXME: temp testing hack

        // first search the active cells
        MWWorld::Ptr ptr
            = MWBase::Environment::get().getWorld()->searchPtrViaEditorId(lowerEditorId, true/*activeOnly*/);

        if (ptr)
            return ptr.getCellRef().getFormId();

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        const ESM4::ActorCharacter *achr = store.getForeign<ESM4::ActorCharacter>().searchLower(lowerEditorId);
        if (achr)
            return achr->mFormId;

        const ESM4::Reference *ref = store.getForeign<ESM4::Reference>().searchLower(lowerEditorId);
        if (ref)
            return ref->mFormId;

        const ESM4::ActorCreature *acre = store.getForeign<ESM4::ActorCreature>().searchLower(lowerEditorId);
        if (acre)
            return acre->mFormId;

        const ESM4::AIPackage *pack = store.getForeign<ESM4::AIPackage>().searchLower(lowerEditorId);
        if (pack)
            return pack->mFormId;

        const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().search (lowerEditorId);
        if (quest && quest->mQuestScript)
        {
            const ESM4::Script *script = store.getForeign<ESM4::Script>().search(quest->mQuestScript);
            if (script)
                return script->mFormId;
        }

        return 0;
    }

    bool CompilerContext::isJournalId (const std::string& name) const
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::Dialogue *topic = store.get<ESM::Dialogue>().search (name);

        return topic && topic->mType==ESM::Dialogue::Journal;
    }
}
