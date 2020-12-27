#include "foreignpotion.hpp"

#include <extern/esm4/alch.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/actionapply.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignPotion::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Potion>()->mBase->mEditorId;
    }

    void ForeignPotion::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignPotion::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignPotion::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return ref->mBase->mFullName;
    }

    std::string ForeignPotion::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return ref->mBase->mEditorId;
    }

    bool ForeignPotion::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignPotion::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mItem.value, "#{sValue}");

        //info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffects);

        // hide effects the player doesnt know about
        //MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr(); // currently unused
        //MWMechanics::NpcStats& npcStats = player.getClass().getNpcStats (player); // currently unused
#if 0
        int alchemySkill = npcStats.getSkill (ESM::Skill::Alchemy).getBase();
        int i=0;
        static const float fWortChanceValue =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWortChanceValue")->getFloat();
        for (MWGui::Widgets::SpellEffectList::iterator it = info.effects.begin(); it != info.effects.end(); ++it)
        {
            it->mKnown = (i <= 1 && alchemySkill >= fWortChanceValue)
                 || (i <= 3 && alchemySkill >= fWortChanceValue*2);
            ++i;
        }
#endif
        info.isPotion = true;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());

            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Script *script = store.getForeign<ESM4::Script>().search(ref->mBase->mScriptId);
            if (script)
                text += MWGui::ToolTips::getMiscString(script->mScript.scriptSource, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignPotion::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    boost::shared_ptr<MWWorld::Action> ForeignPotion::use (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        boost::shared_ptr<MWWorld::Action> action (
            new MWWorld::ActionApply (ptr, ref->mBase->mEditorId));

        action->setSound ("Drink"); // FIXME

        return action;
    }

    std::string ForeignPotion::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    int ForeignPotion::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        int value = ref->mBase->mItem.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string ForeignPotion::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mPickUpSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMPotionUp"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("ITMBottleUp"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignPotion::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMPotionDown"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("ITMBottleDown"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignPotion::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        if (ref->mBase->mMiniIcon != "")
            return ref->mBase->mMiniIcon;
        else
            return ref->mBase->mIcon;
    }

    std::string ForeignPotion::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignPotion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignPotion);

        registerClass (typeid (ESM4::Potion).name(), instance);
    }

    MWWorld::Ptr ForeignPotion::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Potion>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_ALCH);

        //return std::move(newPtr);
        return newPtr;
    }
}
