#include "foreignflora.hpp"

#include <extern/esm4/flor.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignFlora::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Flora>()->mBase->mEditorId;
    }

    void ForeignFlora::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Flora> *ref = ptr.get<ESM4::Flora>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignFlora::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignFlora::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Flora> *ref = ptr.get<ESM4::Flora>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignFlora::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignFlora::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignFlora);

        registerClass (typeid (ESM4::Flora).name(), instance);
    }

    MWWorld::Ptr ForeignFlora::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Flora> *ref = ptr.get<ESM4::Flora>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Flora>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_FLOR);

        //return std::move(newPtr);
        return newPtr;
    }
}
