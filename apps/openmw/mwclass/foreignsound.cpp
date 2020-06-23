#include "foreignsound.hpp"

#include <extern/esm4/soun.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignSound::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Sound>()->mBase->mEditorId;
    }

    void ForeignSound::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Sound> *ref = ptr.get<ESM4::Sound>(); // currently unused

        // FIXME: do SOUN ever have a model?
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignSound::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignSound::getModel(const MWWorld::Ptr &ptr) const
    {
#if !defined NDEBUG
        MWWorld::LiveCellRef<ESM4::Sound> *ref = ptr.get<ESM4::Sound>();
#endif
        assert(ref->mBase != NULL);

// No model for sounds
#if 0
        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string ForeignSound::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignSound::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignSound);

        registerClass (typeid (ESM4::Sound).name(), instance);
    }

    MWWorld::Ptr ForeignSound::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Sound> *ref = ptr.get<ESM4::Sound>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Sound>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_SOUN);

        //return std::move(newPtr);
        return newPtr;
    }
}
