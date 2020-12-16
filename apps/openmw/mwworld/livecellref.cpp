#include "livecellref.hpp"

#include <iostream>

#include <components/esm/objectstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "ptr.hpp"
#include "class.hpp"
#include "esmstore.hpp"

MWWorld::LiveCellRefBase::LiveCellRefBase(std::string type, const ESM::CellRef &cref)
  : mClass(&Class::get(type)), mRef(cref), mData(cref)
{
}

MWWorld::LiveCellRefBase::LiveCellRefBase(std::string type, const ESM4::Reference& cref)
  : mClass(&Class::get(type)), mRef(cref), mData(cref)
{
}

MWWorld::LiveCellRefBase::LiveCellRefBase(std::string type, const ESM4::ActorCreature& cref)
  : mClass(&Class::get(type)), mRef(cref), mData(cref)
{
}

MWWorld::LiveCellRefBase::LiveCellRefBase(std::string type, const ESM4::ActorCharacter& cref)
  : mClass(&Class::get(type)), mRef(cref), mData(cref)
{
}

void MWWorld::LiveCellRefBase::loadImp (const ESM::ObjectState& state)
{
    mRef = state.mRef;
    mData = RefData (state);

    Ptr ptr (this);

    if (state.mHasLocals)
    {
        // FIXME: scriptId can be a FormId string which must be converted back
        std::string scriptId = mClass->getScript (ptr);
        // Make sure we still have a script. It could have been coming from a content file that is no longer active.
        if (!scriptId.empty())
        {
            if (const ESM::Script* script = MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().search (scriptId))
            {
                try
                {
                    mData.setLocals (*script);
                    mData.getLocals().read (state.mLocals, scriptId);
                }
                catch (const std::exception& exception)
                {
                    std::cerr
                        << "failed to load state for local script " << scriptId
                        << " because an exception has been thrown: " << exception.what()
                        << std::endl;
                }
            }
            // FIXME: is this method used only for loading save files?
            else if (const ESM4::Script* script
                    = MWBase::Environment::get().getWorld()->getStore().getForeign<ESM4::Script>().search (ESM4::stringToFormId(scriptId)))
            {
                try
                {
                    mData.setForeignLocals (*script);
                    mData.getLocals().read (state.mLocals, scriptId);
                }
                catch (const std::exception& exception)
                {
                    std::cerr
                        << "failed to load state for local script " << script->mEditorId
                        << " because an exception has been thrown: " << exception.what()
                        << std::endl;
                }
            }
        }
    }

    mClass->readAdditionalState (ptr, state);
}

void MWWorld::LiveCellRefBase::saveImp (ESM::ObjectState& state) const
{
    mRef.writeState(state);

    /// \todo get rid of this cast once const-correct Ptr are available
    Ptr ptr (const_cast<LiveCellRefBase *> (this));

    mData.write (state, mClass->getScript (ptr));

    mClass->writeAdditionalState (ptr, state);
}

bool MWWorld::LiveCellRefBase::checkStateImp (const ESM::ObjectState& state)
{
    return true;
}
