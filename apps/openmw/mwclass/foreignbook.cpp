#include "foreignbook.hpp"

#include <extern/esm4/book.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/action.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignBook::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Book>()->mBase->mEditorId;
    }

    void ForeignBook::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignBook::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignBook::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        if (ref->mBase->mFullName != "")
            return ref->mBase->mFullName;
        else
            return ref->mBase->mEditorId; // FO3 BookGeneric
    }

    std::string ForeignBook::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        return ref->mBase->mEditorId;
    }

    bool ForeignBook::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        if (ref->mBase->mFullName != "")
            return true;
        else
            return (ref->mBase->mEditorId != ""); // FO3 BookGeneric
    }

    MWGui::ToolTipInfo ForeignBook::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());

            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Script *script = store.getForeign<ESM4::Script>().search(ref->mBase->mScriptId);
            if (script)
                text += MWGui::ToolTips::getMiscString(script->mScript.scriptSource, "Script");
        }

        //info.enchant = ref->mBase->mEnchant; // FIXME: need to look up FormId

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignBook::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string ForeignBook::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        if (ref->mBase->mScriptId)
            return ESM4::formIdToString(ref->mBase->mScriptId);
        else
            return "";
    }

    int ForeignBook::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    std::string ForeignBook::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();
        if (ref->mBase->mPickUpSound)
            return ESM4::formIdToString(ref->mBase->mPickUpSound);
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate scroll?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMBookUp"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("UIItemGenericUp"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignBook::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();
        if (ref->mBase->mDropSound)
            return ESM4::formIdToString(ref->mBase->mDropSound);
        else
        {
            // FIXME: another way to get the sound formid?
            // FIXME: how to differentiate scroll?
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMBookDown"); // TES4
            if (sound)
                return ESM4::formIdToString(sound->mFormId);
            else
            {
                sound = store.getForeign<ESM4::Sound>().search("UIItemGenericDown"); // FO3?
                if (sound)
                    return ESM4::formIdToString(sound->mFormId);
            }
        }

        return "";
    }

    std::string ForeignBook::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        if (ref->mBase->mIcon == "")
        {
            //std::cout << "Book ICON missing, using junk " << ref->mBase->mEditorId << std::endl; // FIXME
            return "interface\\icons\\pipboyimages\\items\\item_junk.dds"; // FIXME: FO3
        }

        return ref->mBase->mIcon;
    }

    std::string ForeignBook::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignBook::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignBook);

        registerClass (typeid (ESM4::Book).name(), instance);
    }

    MWWorld::Ptr ForeignBook::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Book>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_BOOK);

        //return std::move(newPtr);
        return newPtr;
    }
}
