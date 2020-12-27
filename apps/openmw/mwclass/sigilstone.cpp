#include "sigilstone.hpp"

#include <extern/esm4/sgst.hpp>

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
    std::string SigilStone::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::SigilStone>()->mBase->mEditorId;
    }

    void SigilStone::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void SigilStone::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string SigilStone::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        return ref->mBase->mFullName;
    }

    std::string SigilStone::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        return ref->mBase->mEditorId;
    }

    bool SigilStone::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo SigilStone::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

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

    boost::shared_ptr<MWWorld::Action> SigilStone::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    boost::shared_ptr<MWWorld::Action> SigilStone::use (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        boost::shared_ptr<MWWorld::Action> action (new MWWorld::ActionApply (ptr, ref->mBase->mEditorId));

        //action->setSound ("Drink");

        return action;
    }

    int SigilStone::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string SigilStone::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return ""; // FIXME
    }

    std::string SigilStone::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return ""; // FIXME
    }

    std::string SigilStone::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        return ref->mBase->mIcon;
    }

    std::string SigilStone::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void SigilStone::registerSelf()
    {
        boost::shared_ptr<Class> instance (new SigilStone);

        registerClass (typeid (ESM4::SigilStone).name(), instance);
    }

    MWWorld::Ptr SigilStone::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::SigilStone>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_SGST);

        //return std::move(newPtr);
        return newPtr;
    }
}
