#include "foreigntree.hpp"

#include <extern/esm4/tree.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignTree::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Tree>()->mBase->mEditorId;
    }

    void ForeignTree::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Tree> *ref = ptr.get<ESM4::Tree>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignTree::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignTree::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::Tree> *ref = ptr.get<ESM4::Tree>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string ForeignTree::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignTree::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignTree);

        registerClass (typeid (ESM4::Tree).name(), instance);
    }

    MWWorld::Ptr ForeignTree::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Tree> *ref = ptr.get<ESM4::Tree>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Tree>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_TREE);

        //return std::move(newPtr);
        return newPtr;
    }
}
