#include "ref.hpp"

#include <components/interpreter/runtime.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "interpretercontext.hpp"

MWWorld::Ptr MWScript::ExplicitRef::operator() (Interpreter::Runtime& runtime, bool required,
    bool activeOnly, bool actor) const
{
    std::string id = runtime.getStringLiteral(runtime[0].mInteger);
    runtime.pop();

    if (required)
        return MWBase::Environment::get().getWorld()->getPtr(id, activeOnly);
    else
        return MWBase::Environment::get().getWorld()->searchPtr(id, activeOnly);
}

MWWorld::Ptr MWScript::ExplicitTes4Ref::operator() (Interpreter::Runtime& runtime, bool required,
    bool activeOnly, bool actor) const
{
    // FIXME: sometimes refId is the EditorId of the REFR and other times it is a local ref variable
    //        how to distinguish?  getLocalRef() can throw and we can't be certain that the index is correct
    //
    // One ugly way is to search assuming that the index is for a literal string of an EditorId, then
    // if it fails to return a valid Ptr then assume it must be an index for a ref variable holding
    // a FormId

    std::string editorId = runtime.getStringLiteral(runtime[0].mInteger);

    // FIXME: duplicated searching, maybe cache it somewhere?
    MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaEditorId(editorId, activeOnly);
    if (ptr)
    {
        runtime.pop();
        return ptr;
    }

    unsigned int ref = runtime.getContext().getLocalRef(runtime[0].mInteger);
    runtime.pop();

    return MWBase::Environment::get().getWorld()->searchPtrViaFormId(ref, activeOnly);
}

MWWorld::Ptr MWScript::ImplicitRef::operator() (Interpreter::Runtime& runtime, bool required,
    bool activeOnly, bool actor) const
{
    MWScript::InterpreterContext& context
    = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

    if (actor)
        return context.getActor();
    else
        return context.getReference(required);
}
