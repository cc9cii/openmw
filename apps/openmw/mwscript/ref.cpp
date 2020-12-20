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
    //std::string refId = runtime.getStringLiteral(runtime[0].mInteger);
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
