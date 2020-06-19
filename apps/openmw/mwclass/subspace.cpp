#include "subspace.hpp"

#include <extern/esm4/sbsp.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string SubSpace::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::SubSpace>()->mBase->mEditorId;
    }

    void SubSpace::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::SubSpace> *ref = ptr.get<ESM4::SubSpace>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void SubSpace::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string SubSpace::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    std::string SubSpace::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::Subspace> *ref = ptr.get<ESM4::Subspace>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    void SubSpace::registerSelf()
    {
        boost::shared_ptr<Class> instance (new SubSpace);

        registerClass (typeid (ESM4::SubSpace).name(), instance);
    }

    MWWorld::Ptr SubSpace::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::SubSpace> *ref = ptr.get<ESM4::SubSpace>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::SubSpace>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_SBSP);

        //return std::move(newPtr);
        return newPtr;
    }
}
