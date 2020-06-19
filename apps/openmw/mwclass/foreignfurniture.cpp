#include "foreignfurniture.hpp"

#include <extern/esm4/furn.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignFurniture::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Furniture>()->mBase->mEditorId;
    }

    void ForeignFurniture::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Furniture> *ref = ptr.get<ESM4::Furniture>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignFurniture::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignFurniture::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Furniture> *ref = ptr.get<ESM4::Furniture>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignFurniture::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignFurniture::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignFurniture);

        registerClass (typeid (ESM4::Furniture).name(), instance);
    }

    MWWorld::Ptr ForeignFurniture::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Furniture> *ref = ptr.get<ESM4::Furniture>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Furniture>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_FURN);

        //return std::move(newPtr);
        return newPtr;
    }
}
