#include "foreignweapon.hpp"

#include <extern/esm4/weap.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/inventorystoretes4.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignWeapon::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Weapon>()->mBase->mEditorId;
    }

    void ForeignWeapon::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignWeapon::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignWeapon::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignWeapon::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        return ref->mBase->mFullName;
    }

    std::string ForeignWeapon::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        return ref->mBase->mEditorId;
    }

    bool ForeignWeapon::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        return (ref->mBase->mFullName != "");
    }

    // FIXME
    MWGui::ToolTipInfo ForeignWeapon::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        MWGui::ToolTipInfo info;

        //const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore(); // currently unused

        //int count = ptr.getRefData().getCount(); // currently unused

        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignWeapon::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::pair<std::vector<int>, bool> ForeignWeapon::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        //MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>(); // currently unused

        std::vector<int> slots_;
        bool stack = false;

        // FIXME
//      if (ref->mBase->mData.type==ESM::Weapon::Arrow || ref->mBase->mData.type==ESM::Weapon::Bolt)
//      {
//          slots_.push_back (int (MWWorld::InventoryStore::Slot_Ammunition));
//          stack = true;
//      }
//      else if (ref->mBase->mData.type==ESM::Weapon::MarksmanThrown)
//      {
//          slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));
//          stack = true;
//      }
//      else
//          slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        // FIXME: testing
        slots_.push_back (int (MWWorld::InventoryStoreTES4::Slot_TES4_SideWeapon));

        return std::make_pair (slots_, stack);
    }

    int ForeignWeapon::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref =
            ptr.get<ESM4::Weapon>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string ForeignWeapon::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate other weapon types?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMWeaponBladeUp"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
        }

        return "";
    }

    std::string ForeignWeapon::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate other weapon types?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMWeaponBladeDown"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
        }

        return "";
    }

    std::string ForeignWeapon::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        if (ref->mBase->mMiniIcon != "")
            return ref->mBase->mMiniIcon;
        else
            return ref->mBase->mIcon;
    }

    std::pair<int, std::string> ForeignWeapon::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        return std::make_pair(1,""); // FIXME: for testing always equip
    }

    void ForeignWeapon::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignWeapon);

        registerClass (typeid (ESM4::Weapon).name(), instance);
    }

    MWWorld::Ptr ForeignWeapon::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Weapon>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_WEAP);

        //return std::move(newPtr);
        return newPtr;
    }
}
