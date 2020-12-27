#include "foreignarmor.hpp"

#include <extern/esm4/armo.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/misc/stringops.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/inventorystoretes4.hpp"
#include "../mwworld/inventorystorefo3.hpp"
#include "../mwworld/inventorystoretes5.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignArmor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Armor>()->mBase->mEditorId;
    }

    void ForeignArmor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>(); // currently unused

        if (!model.empty()) {
            // TES4 seems to lack _gnd models for shields
            std::string lowerModel = Misc::StringUtils::lowerCase(model);
            size_t pos = lowerModel.find("shield_gnd.");
            if (pos != std::string::npos)
                renderingInterface.getObjects().insertModel(ptr, lowerModel.replace(pos+5,4,"")); // FIXME
            else
            {
                pos = lowerModel.find("hatchild_gnd.");
                if (pos != std::string::npos)
                    renderingInterface.getObjects().insertModel(ptr, lowerModel.replace(pos+7,4,"")); // FIXME
                else
                    renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
            }
        }
    }

    void ForeignArmor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
        {
            // TES4 seems to lack _gnd models for shields
            std::string lowerModel = Misc::StringUtils::lowerCase(model);
            size_t pos = lowerModel.find("shield_gnd.");
            if (pos != std::string::npos)
                physics.addObject(ptr, lowerModel.replace(pos+5,4,""));
            else
            {
                pos = lowerModel.find("hatchild_gnd.");
                if (pos != std::string::npos)
                    physics.addObject(ptr, lowerModel.replace(pos+7,4,""));
                else
                    physics.addObject(ptr, model);
            }
        }
    }

    std::string ForeignArmor::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return ref->mBase->mFullName;
    }

    std::string ForeignArmor::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return ref->mBase->mEditorId;
    }

    bool ForeignArmor::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return (ref->mBase->mFullName != "");
    }

    // FIXME
    MWGui::ToolTipInfo ForeignArmor::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        MWGui::ToolTipInfo info;

        //const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore(); // currently unused

        //int count = ptr.getRefData().getCount(); // currently unused

        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIconMale;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

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

    boost::shared_ptr<MWWorld::Action> ForeignArmor::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string ForeignArmor::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    std::pair<std::vector<int>, bool> ForeignArmor::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        std::vector<int> slots_;

        const int sizeTES4 = 7;
        const int sizeFO3 = 17; // ?
        const int sizeTES5 = 29; // ?

        static const int sMappingTES4[sizeTES4][2] =
        {
            { ESM4::Armor::TES4_Hair,      MWWorld::InventoryStoreTES4::Slot_TES4_Hair },
            { ESM4::Armor::TES4_UpperBody, MWWorld::InventoryStoreTES4::Slot_TES4_UpperBody },
            { ESM4::Armor::TES4_LowerBody, MWWorld::InventoryStoreTES4::Slot_TES4_LowerBody },
            { ESM4::Armor::TES4_Hands,     MWWorld::InventoryStoreTES4::Slot_TES4_Hands },
            { ESM4::Armor::TES4_Feet,      MWWorld::InventoryStoreTES4::Slot_TES4_Feet },
            { ESM4::Armor::TES4_Shield,    MWWorld::InventoryStoreTES4::Slot_TES4_Shield },
            { ESM4::Armor::TES4_Tail,      MWWorld::InventoryStoreTES4::Slot_TES4_Tail },
        };

        static const int sMappingFO3[sizeFO3][2] =
        {
            { ESM4::Armor::FO3_Hair,        MWWorld::InventoryStoreFO3::Slot_FO3_Hair },
            { ESM4::Armor::FO3_UpperBody,   MWWorld::InventoryStoreFO3::Slot_FO3_UpperBody },
            { ESM4::Armor::FO3_LeftHand,    MWWorld::InventoryStoreFO3::Slot_FO3_LeftHand },
            { ESM4::Armor::FO3_RightHand,   MWWorld::InventoryStoreFO3::Slot_FO3_RightHand },
            { ESM4::Armor::FO3_PipBoy,      MWWorld::InventoryStoreFO3::Slot_FO3_PipBoy },
            { ESM4::Armor::FO3_Backpack,    MWWorld::InventoryStoreFO3::Slot_FO3_Backpack },
            { ESM4::Armor::FO3_Necklace,    MWWorld::InventoryStoreFO3::Slot_FO3_Necklace },
            { ESM4::Armor::FO3_Headband,    MWWorld::InventoryStoreFO3::Slot_FO3_Headband },
            { ESM4::Armor::FO3_Hat,         MWWorld::InventoryStoreFO3::Slot_FO3_Hat },
            { ESM4::Armor::FO3_EyeGlasses,  MWWorld::InventoryStoreFO3::Slot_FO3_EyeGlasses },
            { ESM4::Armor::FO3_NoseRing,    MWWorld::InventoryStoreFO3::Slot_FO3_NoseRing },
            { ESM4::Armor::FO3_Earrings,    MWWorld::InventoryStoreFO3::Slot_FO3_Earrings },
            { ESM4::Armor::FO3_Mask,        MWWorld::InventoryStoreFO3::Slot_FO3_Mask },
            { ESM4::Armor::FO3_MouthObject, MWWorld::InventoryStoreFO3::Slot_FO3_MouthObject },
            { ESM4::Armor::FO3_BodyAddOn1,  MWWorld::InventoryStoreFO3::Slot_FO3_BodyAddOn1 }, //?
            { ESM4::Armor::FO3_BodyAddOn2,  MWWorld::InventoryStoreFO3::Slot_FO3_BodyAddOn2 }, //?
            { ESM4::Armor::FO3_BodyAddOn3,  MWWorld::InventoryStoreFO3::Slot_FO3_BodyAddOn3 }, //?
        };

        static const int sMappingTES5[sizeTES5][2] =
        {
            { ESM4::Armor::TES5_Hair,        MWWorld::InventoryStoreTES5::Slot_TES5_Hair },
            { ESM4::Armor::TES5_Body,        MWWorld::InventoryStoreTES5::Slot_TES5_Body },
            { ESM4::Armor::TES5_Hands,       MWWorld::InventoryStoreTES5::Slot_TES5_Hands },
            { ESM4::Armor::TES5_Forearms,    MWWorld::InventoryStoreTES5::Slot_TES5_Forearms },
            { ESM4::Armor::TES5_Amulet,      MWWorld::InventoryStoreTES5::Slot_TES5_Amulet },
            { ESM4::Armor::TES5_Ring,        MWWorld::InventoryStoreTES5::Slot_TES5_Ring },
            { ESM4::Armor::TES5_Feet,        MWWorld::InventoryStoreTES5::Slot_TES5_Feet },
            { ESM4::Armor::TES5_Calves,      MWWorld::InventoryStoreTES5::Slot_TES5_Calves },
            { ESM4::Armor::TES5_Shield,      MWWorld::InventoryStoreTES5::Slot_TES5_Shield },
            { ESM4::Armor::TES5_Tail,        MWWorld::InventoryStoreTES5::Slot_TES5_Tail },
            { ESM4::Armor::TES5_LongHair,    MWWorld::InventoryStoreTES5::Slot_TES5_LongHair }, //?
            { ESM4::Armor::TES5_Circlet,     MWWorld::InventoryStoreTES5::Slot_TES5_Circlet },
            { ESM4::Armor::TES5_Ears,        MWWorld::InventoryStoreTES5::Slot_TES5_Ears },
            { ESM4::Armor::TES5_BodyAddOn3,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn3 }, //?
            { ESM4::Armor::TES5_BodyAddOn4,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn4 }, //?
            { ESM4::Armor::TES5_BodyAddOn5,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn5 }, //?
            { ESM4::Armor::TES5_BodyAddOn6,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn6 }, //?
            { ESM4::Armor::TES5_BodyAddOn7,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn7 }, //?
            { ESM4::Armor::TES5_BodyAddOn8,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn8 }, //?
            { ESM4::Armor::TES5_BodyAddOn9,  MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn9 }, //?
            { ESM4::Armor::TES5_BodyAddOn10, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn10 }, //?
            { ESM4::Armor::TES5_BodyAddOn11, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn11 }, //?
            { ESM4::Armor::TES5_BodyAddOn12, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn12 }, //?
            { ESM4::Armor::TES5_BodyAddOn13, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn13 }, //?
            { ESM4::Armor::TES5_BodyAddOn14, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn14 }, //?
            { ESM4::Armor::TES5_BodyAddOn15, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn15 }, //?
            { ESM4::Armor::TES5_BodyAddOn16, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn16 }, //?
            { ESM4::Armor::TES5_BodyAddOn17, MWWorld::InventoryStoreTES5::Slot_TES5_BodyAddOn17 }, //?
            //TES5_DecapHead
            //TES5_Decapitate
            //TES5_FX01
        };

        const int (*sMapping)[2];
        std::size_t size;
        if (ref->mBase->mIsTES4)
        {
            sMapping = &sMappingTES4[0];
            size = sizeTES4;
        }
        else if (ref->mBase->mIsFO3 || ref->mBase->mIsFONV)
        {
            sMapping = &sMappingFO3[0];
            size = sizeFO3;
        }
        else
        {
            sMapping = &sMappingTES5[0];
            size = sizeTES5;
        }

        for (std::size_t i = 0; i < size; ++i)
            if ((sMapping[i][0] & ref->mBase->mArmorFlags) != 0)
                slots_.push_back (int (sMapping[i][1]));

        return std::make_pair (slots_, false);
    }

    int ForeignArmor::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string ForeignArmor::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mPickUpSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate heavy armor?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMArmorLightUp");
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
        }

        return "";
    }

    std::string ForeignArmor::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate heavy armor?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMArmorLightDown");
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
        }

        return "";
    }

    float ForeignArmor::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return ref->mBase->mData.armor / 100.f;
    }

    std::string ForeignArmor::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        if (ref->mBase->mMiniIconMale != "")
            return ref->mBase->mMiniIconMale;
        else
            return ref->mBase->mIconMale; // FIXME: is there a way to check if female?
    }

    std::string ForeignArmor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
        assert(ref->mBase != NULL);

        // clothing and armor need "ground" models (with physics) unless being worn
        if (ref->mBase->mIsTES4)
        {
            std::string model = ref->mBase->mModelMale; // FIXME: what about female?
            if (!model.empty())
            {
                size_t pos = Misc::StringUtils::lowerCase(model).find_last_of("."); // pos points at '.'
                if (pos == std::string::npos || model.substr(pos+1) != "nif") // mModel does not end in ".nif"
                    return "meshes\\" + model.substr(0, pos) + "_gnd.nif";
            }
        }
        else if (ref->mBase->mIsFO3 || ref->mBase->mIsFONV)
        {
            return "meshes\\" + ref->mBase->mModelMaleWorld;
        }
        //else
            // FIXME TES5

        return "";
    }

    // Armor and Weapon have "health" - a character will not wear the item if the health is zero.
    std::pair<int, std::string> ForeignArmor::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        MWWorld::InventoryStore& invStore = npc.getClass().getInventoryStore(npc);

        if (ptr.getCellRef().getCharge() == 0)
            return std::make_pair(0, "#{sInventoryMessage1}");

        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots_ = ptr.getClass().getEquipmentSlots(ptr);

        if (slots_.first.empty())
            return std::make_pair(0, "");

//      if (npc.getClass().isNpc())
//      {
//          std::string npcRace = npc.get<ESM::NPC>()->mBase->mRace;

//          // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
//          const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npcRace);
//          if(race->mData.mFlags & ESM::Race::Beast)
//          {
//              std::vector<ESM::PartReference> parts = ptr.get<ESM::Armor>()->mBase->mParts.mParts;

//              for(std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
//              {
//                  if((*itr).mPart == ESM::PRT_Head)
//                      return std::make_pair(0, "#{sNotifyMessage13}");
//                  if((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
//                      return std::make_pair(0, "#{sNotifyMessage14}");
//              }
//          }
//      }

        for (std::vector<int>::const_iterator slot=slots_.first.begin();
            slot!=slots_.first.end(); ++slot)
        {
            // If equipping a shield, check if there's a twohanded weapon conflicting with it
            if(*slot == MWWorld::InventoryStore::Slot_CarriedLeft)
            {
                MWWorld::ContainerStoreIterator weapon = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

                if(weapon == invStore.end())
                    return std::make_pair(1,"");

//              if(weapon->getTypeName() == typeid(ESM::Weapon).name() &&
//                      (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow))
//              {
//                  return std::make_pair(3,"");
//              }
                return std::make_pair(1,"");
            }
        }
        return std::make_pair(1,"");
    }

    void ForeignArmor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignArmor);

        registerClass (typeid (ESM4::Armor).name(), instance);
    }

    MWWorld::Ptr ForeignArmor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Armor>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_ARMO);

        //return std::move(newPtr);
        return newPtr;
    }
}
