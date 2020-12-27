#include "ammunition.hpp"

#include <extern/esm4/ammo.hpp>

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

namespace MWClass
{
    std::string ForeignAmmo::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Ammunition>()->mBase->mEditorId;
    }

    void ForeignAmmo::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignAmmo::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignAmmo::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        return ref->mBase->mFullName;
    }

    std::string ForeignAmmo::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        return ref->mBase->mEditorId;
    }

    bool ForeignAmmo::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignAmmo::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignAmmo::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    int ForeignAmmo::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string ForeignAmmo::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mPickUpSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMAmmoUp"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("ITMAmmunitionUp"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignAmmo::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMAmmoDown"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("ITMAmmunitionDown"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignAmmo::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        if (ref->mBase->mMiniIcon != "")
            return ref->mBase->mMiniIcon;
        else
            return ref->mBase->mIcon;
    }

    std::string ForeignAmmo::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignAmmo::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignAmmo);

        registerClass (typeid (ESM4::Ammunition).name(), instance);
    }

    MWWorld::Ptr ForeignAmmo::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Ammunition> *ref = ptr.get<ESM4::Ammunition>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Ammunition>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_AMMO);

        //return std::move(newPtr);
        return newPtr;
    }
}
