#include "foreigncontainer.hpp"

#include <extern/esm4/cont.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    struct ForeignContainerCustomData : public MWWorld::CustomData
    {
        MWWorld::ContainerStore mContainerStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *ForeignContainerCustomData::clone() const
    {
        return new ForeignContainerCustomData (*this);
    }
}

namespace MWClass
{
    void ForeignContainer::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<ForeignContainerCustomData> data (new ForeignContainerCustomData);

            MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

            // make ESM4 inventory look like ESM::InventoryList so that we can pass it to
            // mInventoryStore.fill()
            ESM::InventoryList inventory;
            for (unsigned int i = 0; i < ref->mBase->mInventory.size(); ++i)
            {
                ESM::ContItem item;
                item.mCount = ref->mBase->mInventory.at(i).count;
                // FIXME: maybe the base object's EditorId should be used here
                // but to do so we need to search the ESMStore
                item.mItem.assign(ESM4::formIdToString(ref->mBase->mInventory.at(i).item));

                inventory.mList.push_back(item);
            }

            // this "fills" the inventory with iterators for quick access?
            data->mContainerStore.fill(inventory, getId(ptr));

            // store the data
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string ForeignContainer::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Container>()->mBase->mEditorId;
    }

    void ForeignContainer::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignContainer::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignContainer::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        return ref->mBase->mFullName;
    }

    bool ForeignContainer::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignContainer::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref =
            ptr.get<ESM4::Container>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName;

        std::string text;
        if (ptr.getCellRef().getLockLevel() > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ptr.getCellRef().getLockLevel());
        else if (ptr.getCellRef().getLockLevel() < 0)
            text += "\n#{sUnlocked}";
        if (ptr.getCellRef().getTrap() != "")
            text += "\n#{sTrapped}";

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

    boost::shared_ptr<MWWorld::Action> ForeignContainer::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        const ESM4::FormId openSoundId = ref->mBase->mOpenSound;
        const ESM4::FormId closeSoundId = ref->mBase->mCloseSound;

        std::string openSound = ESM4::formIdToString(openSoundId);
        std::string closeSound = ESM4::formIdToString(closeSoundId);

        if (!MWBase::Environment::get().getWindowManager()->isAllowed(MWGui::GW_Inventory))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::NullAction ());

        const std::string lockedSound = "LockedChest";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWWorld::InventoryStore& invStore = player.getClass().getInventoryStore(player);

        bool needKey = ptr.getCellRef().getLockLevel() > 0;
        bool hasKey = false;
        std::string keyName;

        // make key id lowercase
        std::string keyId = ptr.getCellRef().getKey();
        Misc::StringUtils::lowerCaseInPlace(keyId);
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            std::string refId = it->getCellRef().getRefId();
            Misc::StringUtils::lowerCaseInPlace(refId);
            if (refId == keyId)
            {
                hasKey = true;
                keyName = it->getClass().getName(*it);
            }
        }

        if (needKey && hasKey)
        {
            MWBase::Environment::get().getWindowManager ()->messageBox (keyName + " #{sKeyUsed}");
            unlock(ptr);
            // using a key disarms the trap
            ptr.getCellRef().setTrap("");
        }


        if (!needKey || hasKey)
        {
            if(ptr.getCellRef().getTrap().empty())
            {
                boost::shared_ptr<MWWorld::Action> action (new MWWorld::ActionOpen(ptr));
                if (openSoundId) // FO3 containers seems to use text key during animation (like some doors)
                    action->setSound(openSound);
                return action;
            }
            else
            {
                // Activate trap
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(actor, ptr.getCellRef().getTrap(), ptr));
                //action->setSound(trapActivationSound);
                return action;
            }
        }
        else
        {
            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction);
            //action->setSound(lockedSound);
            return action;
        }
    }

    std::string ForeignContainer::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    MWWorld::ContainerStore& ForeignContainer::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignContainerCustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
    }

    std::string ForeignContainer::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignContainer::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignContainer);

        registerClass (typeid (ESM4::Container).name(), instance);
    }

    MWWorld::Ptr ForeignContainer::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Container>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_CONT);

        //return std::move(newPtr);
        return newPtr;
    }
}
