#include "foreignactivator.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/acti.hpp>

#include <components/esm/doorstate.hpp> // FIXME: pretending to be a door

#include "../mwgui/tooltips.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/nullaction.hpp"
#include "../mwworld/actiondoor.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/animation.hpp"
#include "../mwworld/customdata.hpp"

// FIXME: duplicated from door.cpp
// FIXME: pretending to be a door
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
    std::string ForeignActivator::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Activator>()->mBase->mEditorId;
    }

    void ForeignActivator::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        // model = "meshes\\Furniture\\FXspiderWebKitDoorSpecial.nif" BleakFallsBarrow01
        // FIXME: TES5 furniture\fxspiderwebkitdoorspecial.nif crashes Ogre
        if (model.find("FXspiderWeb") != std::string::npos)
            return; // std::cout << "stop" << std::endl;

        //MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignActivator::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        // model = "meshes\\Furniture\\FXspiderWebKitDoorSpecial.nif"
        // FIXME: TES5 furniture\fxspiderwebkitdoorspecial.nif crashes Ogre
        if (model.find("FXspiderWeb") != std::string::npos)
            return; // std::cout << "stop" << std::endl;

        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignActivator::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    // This is required for activations to work
    std::string ForeignActivator::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        return ref->mBase->mFullName;
    }

    bool ForeignActivator::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignActivator::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());

            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Script *script = store.getForeign<ESM4::Script>().search(ref->mBase->mScriptId);
            if (script)
                text += MWGui::ToolTips::getMiscString(script->mScript.scriptSource, "Script");
        }
        info.text = text;

        return info;
    }

    // FIXME: only some actions supported for now
    boost::shared_ptr<MWWorld::Action> ForeignActivator::activate(const MWWorld::Ptr &ptr, const MWWorld::Ptr &actor) const
    {
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::NullAction);
    }

    std::string ForeignActivator::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    void ForeignActivator::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<DoorCustomData> data(new DoorCustomData);

            data->mDoorState = 0;
            ptr.getRefData().setCustomData(data.release());
        }
    }

    int ForeignActivator::getDoorState (const MWWorld::Ptr &ptr) const
    {
        //MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr); // currently unused
        ensureCustomData(ptr);
        const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());
        return customData.mDoorState;
    }

    void ForeignActivator::setDoorState (const MWWorld::Ptr &ptr, int state) const
    {
        if (ptr.getCellRef().getTeleport())
            throw std::runtime_error("load doors can't be moved");

        ensureCustomData(ptr);
        DoorCustomData& customData = dynamic_cast<DoorCustomData&>(*ptr.getRefData().getCustomData());
        customData.mDoorState = state;
    }

    void ForeignActivator::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        ensureCustomData(ptr);
        DoorCustomData& customData = dynamic_cast<DoorCustomData&>(*ptr.getRefData().getCustomData());

        const ESM::DoorState& state2 = dynamic_cast<const ESM::DoorState&>(state);
        customData.mDoorState = state2.mDoorState;
    }

    void ForeignActivator::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state) const
    {
        ensureCustomData(ptr);
        const DoorCustomData& customData = dynamic_cast<const DoorCustomData&>(*ptr.getRefData().getCustomData());

        ESM::DoorState& state2 = dynamic_cast<ESM::DoorState&>(state);
        state2.mDoorState = customData.mDoorState;
    }

    void ForeignActivator::playgroup(const MWWorld::Ptr& ptr, const std::string& animation, int flag)
    {
        MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (anim->hasAnimation(animation))
        {
            std::cout << "Activator playgroup: " << animation << std::endl; // FIXME: temp testing

            // FIXME: flag not used yet
            //        0 = wait till current animation completes before starting
            //        1 = immediate start
            //        2 = immediate start at the loop cycle
            anim->play(animation, 0, 1, true, 1.0f, "start", "end", 0.0f, 0);
        }
    }

    void ForeignActivator::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignActivator);

        registerClass (typeid (ESM4::Activator).name(), instance);
    }

    MWWorld::Ptr ForeignActivator::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Activator>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_ACTI);

        //return std::move(newPtr);
        return newPtr;
    }
}
