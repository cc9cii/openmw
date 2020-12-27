#include "foreignkey.hpp"

#include <extern/esm4/keym.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignKey::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Key>()->mBase->mEditorId;
    }

    void ForeignKey::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignKey::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignKey::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        return ref->mBase->mFullName;
    }

    std::string ForeignKey::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        return ref->mBase->mEditorId;
    }

    bool ForeignKey::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignKey::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

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

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignKey::activate (const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    int ForeignKey::getValue (const MWWorld::Ptr& ptr) const
    {
        return 0;
    }

    std::string ForeignKey::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mPickUpSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMKeyUp"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("ITMKeyUp"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignKey::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound); // FONV
        else
        {
            // FIXME: another way to get the sound formid?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMKeyDown"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("ITMKeyDown"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignKey::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        if (ref->mBase->mMiniIcon != "")
            return ref->mBase->mMiniIcon;
        else
            return ref->mBase->mIcon;
    }

    std::string ForeignKey::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignKey::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignKey);

        registerClass (typeid (ESM4::Key).name(), instance);
    }

    MWWorld::Ptr ForeignKey::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Key>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_KEYM);

        //return std::move(newPtr);
        return newPtr;
    }
}
