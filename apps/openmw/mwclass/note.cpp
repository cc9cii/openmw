#include "note.hpp"

#include <extern/esm4/note.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Note::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Note>()->mBase->mEditorId;
    }

    void Note::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void Note::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string Note::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        return ref->mBase->mFullName;
    }

    std::string Note::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        return ref->mBase->mEditorId;
    }

    bool Note::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo Note::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        //text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        //text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        //info.enchant = ref->mBase->mEnchant; // FIXME: need to look up FormId

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Note::activate (const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    int Note::getValue (const MWWorld::Ptr& ptr) const
    {
        return 0;
    }

    std::string Note::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    std::string Note::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    std::string Note::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        return ref->mBase->mIcon;
    }

    std::string Note::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void Note::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Note);

        registerClass (typeid (ESM4::Note).name(), instance);
    }

    MWWorld::Ptr Note::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Note>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_NOTE);

        //return std::move(newPtr);
        return newPtr;
    }
}
