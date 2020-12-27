#include "foreigningredient.hpp"

#include <extern/esm4/ingr.hpp>

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
    std::string ForeignIngredient::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Ingredient>()->mBase->mEditorId;
    }

    void ForeignIngredient::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        //MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>(); // currently unused

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignIngredient::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignIngredient::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        return ref->mBase->mFullName;
    }

    std::string ForeignIngredient::getEditorId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        return ref->mBase->mEditorId;
    }

    int ForeignIngredient::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    bool ForeignIngredient::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        return (ref->mBase->mFullName != "");
    }

    // FIXME
    MWGui::ToolTipInfo ForeignIngredient::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        MWGui::ToolTipInfo info;

        //const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore(); // currently unused

        //int count = ptr.getRefData().getCount(); // currently unused

        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIconMale;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignIngredient::activate (const MWWorld::Ptr& ptr,
            const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    std::string ForeignIngredient::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMIngredientUp");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);

        return "";
    }

    std::string ForeignIngredient::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        // FIXME: another way to get the sound formid?
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search("ITMIngredientDown");
        if (sound)
            return ESM4::formIdToString(sound->mFormId);

        return "";
    }

    std::string ForeignIngredient::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        return ref->mBase->mIcon;
    }

    std::string ForeignIngredient::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    void ForeignIngredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignIngredient);

        registerClass (typeid (ESM4::Ingredient).name(), instance);
    }

    MWWorld::Ptr ForeignIngredient::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        MWWorld::Ptr newPtr(cell.getForeign<ESM4::Ingredient>().insert(*ref), &cell);
        cell.updateLookupMaps(newPtr.getBase()->mRef.getFormId(), ref, ESM4::REC_INGR);

        //return std::move(newPtr);
        return newPtr;
    }
}
