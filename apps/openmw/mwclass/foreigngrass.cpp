#include "foreigngrass.hpp"

#include <extern/esm4/gras.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignGrass::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Grass>()->mBase->mEditorId;
    }

    void ForeignGrass::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Grass> *ref = ptr.get<ESM4::Grass>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignGrass::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignGrass::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Grass> *ref = ptr.get<ESM4::Grass>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignGrass::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignGrass::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignGrass);

        registerClass (typeid (ESM4::Grass).name(), instance);
    }

    MWWorld::Ptr ForeignGrass::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Grass> *ref = ptr.get<ESM4::Grass>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Grass>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_GRAS);

        //return std::move(newPtr);
        return newPtr;
    }
}
