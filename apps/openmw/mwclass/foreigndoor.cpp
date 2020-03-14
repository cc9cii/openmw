#include "foreigndoor.hpp"

#include <extern/esm4/cell.hpp>
//#include <components/esm/loaddoor.hpp>
#include <components/esm/doorstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/foreignstore.hpp"
#include "../mwworld/actionteleportforeign.hpp"
#include "../mwworld/foreigncell.hpp"
#include "../mwworld/foreignworld.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/actiondoor.hpp"
#include "../mwworld/actiontrap.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/customdata.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/animation.hpp"

#include "../mwgui/tooltips.hpp"

// FIXME: duplicated from door.cpp
namespace
{
    struct DoorCustomData : public MWWorld::CustomData
    {
        int mDoorState; // 0 = nothing, 1 = opening, 2 = closing

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *DoorCustomData::clone() const
    {
        return new DoorCustomData (*this);
    }
}

namespace MWClass
{
    std::string ForeignDoor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Door>()->mBase->mEditorId;
    }

    // FIXME: having the dummy cell results in *all* the doors being rendered!  Need to be
    // able to limit rendering based on the ref's position
    void ForeignDoor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignDoor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);

        // Resume the door's opening/closing animation if it wasn't finished
        if (ptr.getRefData().getCustomData())
        {
            const DoorCustomData& customData
                = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());
            if (customData.mDoorState > 0)
            {
                MWBase::Environment::get().getWorld()->activateDoor(ptr, customData.mDoorState);
            }
        }

        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string ForeignDoor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    // This is required for activations to work
    std::string ForeignDoor::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref =
            ptr.get<ESM4::Door>();

        return ref->mBase->mFullName;
    }

    void ForeignDoor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignDoor);

        registerClass (typeid (ESM4::Door).name(), instance);
    }

    MWWorld::Ptr ForeignDoor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        return MWWorld::Ptr(&cell.get<ESM4::Door>().insert(*ref), &cell);
    }

    boost::shared_ptr<MWWorld::Action> ForeignDoor::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::ForeignStore<ESM4::Sound>& soundStore = store.getForeign<ESM4::Sound>();
        const ESM4::FormId openSoundId = ref->mBase->mOpenSound;
        const ESM4::FormId closeSoundId = ref->mBase->mCloseSound;

        const std::string &openSound = soundStore.search(openSoundId)->mSoundFile;
        const std::string &closeSound = soundStore.search(closeSoundId)->mSoundFile;
        const std::string lockedSound = "LockedDoor";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::ContainerStore &invStore = actor.getClass().getContainerStore(actor);

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
            if(actor == MWBase::Environment::get().getWorld()->getPlayerPtr())
                MWBase::Environment::get().getWindowManager()->messageBox(keyName + " #{sKeyUsed}");
            unlock(ptr); //Call the function here. because that makes sense.
            // using a key disarms the trap
            ptr.getCellRef().setTrap("");
        }

        if (!needKey || hasKey)
        {
            if(!ptr.getCellRef().getTrap().empty())
            {
                // Trap activation
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(actor, ptr.getCellRef().getTrap(), ptr));
                //action->setSound(trapActivationSound); // FIXME: temp disable
                return action;
            }

            Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
            MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);

            if (ptr.getCellRef().getTeleport())
            {
                //std::cout << "open door" << std::endl;
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                const ESM4::FormId cellId = store.getDoorCell(ptr.getCellRef().getDestDoor());
                const MWWorld::ForeignCell *cell = store.get<MWWorld::ForeignCell>().find(cellId);
                // FIXME check cell is not null
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTeleportForeign(
                            cell->mCell->mEditorId,
                            cellId,
                            ptr.getCellRef().getDoorDest(), true));

                //action->setSound(openSound);  // FIXME: temp disable

                return action;
            }
            else if (anim->hasAnimation("Open") || anim->hasAnimation("Close"))
            {
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionDoor(ptr));
                return action;
            }
            else
            {
                // animated door
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionDoor(ptr));
                int doorstate = getDoorState(ptr);
                bool opening = true;
                if (doorstate == 1)
                    opening = false;
                if (doorstate == 0 && ptr.getRefData().getLocalRotation().rot[2] != 0) // FIXME
                    opening = false;

                if (opening)
                {
                    MWBase::Environment::get().getSoundManager()->fadeOutSound3D(ptr,
                            closeSound, 0.5f);
                    float offset = ptr.getRefData().getLocalRotation().rot[2]/ 3.14159265f * 2.0f;
                    //action->setSoundOffset(offset); // FIXME: temp disable
                    //action->setSound(openSound); // FIXME: temp disable
                }
                else
                {
                    MWBase::Environment::get().getSoundManager()->fadeOutSound3D(ptr,
                                                openSound, 0.5f);
                    float offset = 1.0f - ptr.getRefData().getLocalRotation().rot[2]/ 3.14159265f * 2.0f;
                    //most if not all door have closing bang somewhere in the middle of the sound,
                    //so we divide offset by two
                    //action->setSoundOffset(offset * 0.5f); // FIXME: temp disable
                    //action->setSound(closeSound); // FIXME: temp disable
                }

                return action;
            }
        }
        else
        {
            // locked, and we can't open.
            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction);
            //action->setSound(lockedSound); // FIXME: temp disable
            return action;
        }
    }

    bool ForeignDoor::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref =
            ptr.get<ESM4::Door>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignDoor::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref =
            ptr.get<ESM4::Door>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName;

        std::string text;

        if (ptr.getCellRef().getTeleport())
        {
            text += "\n#{sTo}";
            text += "\n" + getDestination(*ref);
        }

        if (ptr.getCellRef().getLockLevel() > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ptr.getCellRef().getLockLevel());
        else if (ptr.getCellRef().getLockLevel() < 0)
            text += "\n#{sUnlocked}";
        if (ptr.getCellRef().getTrap() != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }

    std::string ForeignDoor::getDestination (const MWWorld::LiveCellRef<ESM4::Door>& door)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string dest;
        if (door.mRef.getDestCell() != "")
        {
            // door leads to an interior, use interior name as tooltip
            // (or external cell with full name)
            dest = door.mRef.getDestCell();
        }
        else
        {
            // use world name if exists
            const ESM4::FormId cellId = store.getDoorCell(door.mRef.getDestDoor());
            const MWWorld::ForeignCell *cell = store.get<MWWorld::ForeignCell>().find(cellId);

            if (cell)
            {
                ESM4::FormId worldId = cell->mCell->mParent;
                const MWWorld::ForeignWorld * world = store.get<MWWorld::ForeignWorld>().find(worldId);
                if (!world->mFullName.empty())
                    dest = world->mFullName;
            }
        }

        return "#{sCell=" + dest + "}";
    }

    void ForeignDoor::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<DoorCustomData> data(new DoorCustomData);

            data->mDoorState = 0;
            ptr.getRefData().setCustomData(data.release());
        }
    }

    int ForeignDoor::getDoorState (const MWWorld::Ptr &ptr) const
    {
        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        ensureCustomData(ptr);
        const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());
        return customData.mDoorState;
    }

    void ForeignDoor::setDoorState (const MWWorld::Ptr &ptr, int state) const
    {
        if (ptr.getCellRef().getTeleport())
            throw std::runtime_error("load doors can't be moved");

        ensureCustomData(ptr);
        DoorCustomData& customData = dynamic_cast<DoorCustomData&>(*ptr.getRefData().getCustomData());
        customData.mDoorState = state;
    }

    void ForeignDoor::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        ensureCustomData(ptr);
        DoorCustomData& customData = dynamic_cast<DoorCustomData&>(*ptr.getRefData().getCustomData());

        const ESM::DoorState& state2 = dynamic_cast<const ESM::DoorState&>(state);
        customData.mDoorState = state2.mDoorState;
    }

    void ForeignDoor::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state) const
    {
        ensureCustomData(ptr);
        const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());

        ESM::DoorState& state2 = dynamic_cast<ESM::DoorState&>(state);
        state2.mDoorState = customData.mDoorState;
    }
}