#include "soulgem.hpp"

#include <extern/esm4/slgm.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/actionapply.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string SoulGem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::SoulGem>()->mBase->mEditorId;
    }

    void SoulGem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void SoulGem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string SoulGem::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        return ref->mBase->mFullName;
    }

    std::string SoulGem::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        return ref->mBase->mEditorId;
    }

    bool SoulGem::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo SoulGem::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        //info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffect);


        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> SoulGem::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    boost::shared_ptr<MWWorld::Action> SoulGem::use (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        boost::shared_ptr<MWWorld::Action> action (new MWWorld::ActionApply (ptr, ref->mBase->mEditorId));

        //action->setSound ("Drink");

        return action;
    }

    int SoulGem::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string SoulGem::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMSoulGemUp");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);

        return "";
    }

    std::string SoulGem::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMSoulGemDown");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);

        return "";
    }

    std::string SoulGem::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        return ref->mBase->mIcon;
    }

    std::string SoulGem::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void SoulGem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new SoulGem);

        registerClass (typeid (ESM4::SoulGem).name(), instance);
    }

    MWWorld::Ptr SoulGem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::SoulGem>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_SLGM);

        //return std::move(newPtr);
        return newPtr;
    }
}
