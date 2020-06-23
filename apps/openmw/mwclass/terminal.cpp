#include "terminal.hpp"

#include <extern/esm4/term.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Terminal::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Terminal>()->mBase->mEditorId;
    }

    void Terminal::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Terminal> *ref = ptr.get<ESM4::Terminal>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void Terminal::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string Terminal::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Terminal> *ref = ptr.get<ESM4::Terminal>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Terminal::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Terminal> *ref = ptr.get<ESM4::Terminal>();

        return ref->mBase->mFullName;
    }

    bool Terminal::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Terminal> *ref = ptr.get<ESM4::Terminal>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo Terminal::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Terminal> *ref = ptr.get<ESM4::Terminal>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName;
        //info.icon = ref->mBase->mIcon;

        std::string text;

        //text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        //text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        info.text = text;

        return info;
    }

    void Terminal::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Terminal);

        registerClass (typeid (ESM4::Terminal).name(), instance);
    }

    MWWorld::Ptr Terminal::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Terminal> *ref = ptr.get<ESM4::Terminal>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Terminal>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_TERM);

        //return std::move(newPtr);
        return newPtr;
    }
}
