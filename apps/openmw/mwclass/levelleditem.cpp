#include "levelleditem.hpp"

#include <extern/esm4/lvli.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string LevelledItem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::LevelledItem>()->mBase->mEditorId;
    }

    void LevelledItem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::LevelledItem> *ref = ptr.get<ESM4::LevelledItem>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void LevelledItem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string LevelledItem::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::LevelledItem> *ref = ptr.get<ESM4::LevelledItem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string LevelledItem::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void LevelledItem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new LevelledItem);

        registerClass (typeid (ESM4::LevelledItem).name(), instance);
    }

    MWWorld::Ptr LevelledItem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::LevelledItem> *ref = ptr.get<ESM4::LevelledItem>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::LevelledItem>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_LVLI);

        //return std::move(newPtr);
        return newPtr;
    }
}
