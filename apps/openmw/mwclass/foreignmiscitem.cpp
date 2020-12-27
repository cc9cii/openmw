#include "foreignmiscitem.hpp"

#include <boost/lexical_cast.hpp>

#include <extern/esm4/misc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/action.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
bool isGold (const MWWorld::Ptr& ptr)
{
    // FIXME: probably no longer relevant for TES4
    return Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_001")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_005")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_010")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_025")
                    || Misc::StringUtils::ciEqual(ptr.getCellRef().getRefId(), "gold_100");
}
}

namespace MWClass
{
    std::string ForeignMiscItem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::MiscItem>()->mBase->mEditorId;
    }

    void ForeignMiscItem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignMiscItem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignMiscItem::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        return ref->mBase->mFullName;
    }

    std::string ForeignMiscItem::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        return ref->mBase->mEditorId;
    }

    bool ForeignMiscItem::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignMiscItem::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        MWGui::ToolTipInfo info;

        //const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore(); // currently unused

        int count = ptr.getRefData().getCount();

        bool gold = isGold(ptr);
        if (gold)
            count *= getValue(ptr);

        std::string countString;
        if (!gold)
            countString = MWGui::ToolTips::getCountString(count);
        else // gold displays its count also if it's 1.
            countString = " (" + boost::lexical_cast<std::string>(count) + ")";

        info.caption = ref->mBase->mFullName + countString;
        //info.icon = ref->mBase->mIcon;

//      if (ref->mRef.getSoul() != "")
//      {
//          const ESM4::Creature *creature = store.get<ESM4::Creature>().find(ref->mRef.getSoul());
//          info.caption += " (" + creature->mName + ")";
//      }

        std::string text;

        if (!gold /*&& !ref->mBase->mData.mIsKey*/) // FIXME: mIsKey missing
        {
            text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
            text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");
        }

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignMiscItem::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    int ForeignMiscItem::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

//      if (ptr.getCellRef().getSoul() != "")
//      {
//          const ESM::Creature *creature = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>().find(ref->mRef.getSoul());
//          value *= creature->mData.mSoul;
//      }

        return value;
    }

    std::string ForeignMiscItem::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mPickUpSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate gold/welkyndstone/lockpick?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMGenericUp"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("UIItemGenericUp"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignMiscItem::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate gold/welkyndstone/lockpick?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMGenericDown"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("UIItemGenericDown"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignMiscItem::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        if (ref->mBase->mMiniIcon != "")
            return ref->mBase->mMiniIcon;
        else
            return ref->mBase->mIcon;
    }

    std::string ForeignMiscItem::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignMiscItem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignMiscItem);

        registerClass (typeid (ESM4::MiscItem).name(), instance);
    }

    MWWorld::Ptr ForeignMiscItem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::MiscItem>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_MISC);

        //return std::move(newPtr);
        return newPtr;
    }
}
