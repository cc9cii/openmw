#include "foreignapparatus.hpp"

#include <extern/esm4/appa.hpp>

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
    std::string ForeignApparatus::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Apparatus>()->mBase->mEditorId;
    }

    void ForeignApparatus::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignApparatus::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignApparatus::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        return ref->mBase->mFullName;
    }

    std::string ForeignApparatus::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        return ref->mBase->mEditorId;
    }

    bool ForeignApparatus::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        return (ref->mBase->mFullName != "");
    }

    // FIXME
    MWGui::ToolTipInfo ForeignApparatus::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

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

    boost::shared_ptr<MWWorld::Action> ForeignApparatus::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string ForeignApparatus::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    int ForeignApparatus::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string ForeignApparatus::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMApparatusUp");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);
        else
            return "";
    }

    std::string ForeignApparatus::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMApparatusDown");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);
        else
            return "";
    }

    std::string ForeignApparatus::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        return ref->mBase->mIcon;
    }

    std::string ForeignApparatus::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignApparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignApparatus);

        registerClass (typeid (ESM4::Apparatus).name(), instance);
    }

    MWWorld::Ptr ForeignApparatus::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Apparatus>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_APPA);

        //return std::move(newPtr);
        return newPtr;
    }
}
