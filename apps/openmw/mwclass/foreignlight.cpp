#include "foreignlight.hpp"

#include <iostream> // FIXME: testing only

#include <extern/esm4/ligh.hpp>

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

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignLight::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Light>()->mBase->mEditorId;
    }

    void ForeignLight::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>(); // currently unused

        // some LIGH records have models (and most likely will have "AttachLight" node/bone) -
        // for those insertLight should specify to attach
        //
        // some ACTI records (e.g. Lights\TorchTall01.NIF) also have "AttachLight" node/bone -
        // thse may also have associated scripts for damage
        //
        // some ACTI, MISC or STAT records may be torches - not sure what kind of light to attach
        if (!model.empty()) {
            //std::cout << "Foreign light has model " << model << std::endl;
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
        else
            renderingInterface.getObjects().insertLight(ptr);
    }

    void ForeignLight::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignLight::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        return ref->mBase->mFullName;
    }

    std::string ForeignLight::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        return ref->mBase->mEditorId;
    }

    bool ForeignLight::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        return (ref->mBase->mFullName != "");
    }

    // FIXME
    MWGui::ToolTipInfo ForeignLight::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        MWGui::ToolTipInfo info;

        //int count = ptr.getRefData().getCount(); // currently unused

        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIconMale;

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

    boost::shared_ptr<MWWorld::Action> ForeignLight::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::pair<std::vector<int>, bool> ForeignLight::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        std::vector<int> slots_;

        if ((ref->mBase->mData.flags & 0x0002/*can be carried*/) != 0)
            slots_.push_back (int (MWWorld::InventoryStoreTES4::Slot_TES4_Torch));

        return std::make_pair (slots_, false);
    }

    int ForeignLight::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        return ref->mBase->mData.value;
    }

    std::string ForeignLight::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMGenericUp");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);

        return "";
    }

    std::string ForeignLight::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMGenericDown");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);

        return "";
    }

    std::string ForeignLight::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        return ref->mBase->mIcon;
    }


    std::pair<int, std::string> ForeignLight::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
//      MWWorld::InventoryStore& invStore = npc.getClass().getInventoryStore(npc);
//      MWWorld::ContainerStoreIterator weapon = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

//      if(weapon == invStore.end())
//          return std::make_pair(1,"");

//      /// \todo the 2h check is repeated many times; put it in a function
//      if(weapon->getTypeName() == typeid(ESM::Weapon).name() &&
//              (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
//      weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose ||
//      weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide ||
//      weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
//      weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand ||
//      weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow ||
//      weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow))
//      {
//          return std::make_pair(3,"");
//      }
        return std::make_pair(1,"");
    }

    std::string ForeignLight::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignLight::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignLight);

        registerClass (typeid (ESM4::Light).name(), instance);
    }

    MWWorld::Ptr ForeignLight::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Light>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_LIGH);

        //return std::move(newPtr);
        return newPtr;
    }
}
