#include "tes4extensions.hpp"

#include <stdexcept>

#include <boost/format.hpp>

#include <MyGUI_LanguageManager.h>

#include <extern/esm4/qust.hpp>

#include <components/compiler/extensions.hpp>
#include <components/compiler/opcodes.hpp>
//#include <components/tes4compiler/extensions.hpp>
#include <components/tes4compiler/opcodes.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/runtime.hpp>
#include <components/interpreter/opcodes.hpp> // classes Opcode0, Opcode1 and Opcode2

#include <components/misc/stringops.hpp>

#include <components/esm/loadskil.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/animation.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

namespace MWScript
{
    namespace Tes4
    {
        template<class R>
        class OpIsActionRef : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    // ptr is IDGate01 in CGAmbushAGateScript but what we really want is
                    // the object/npc/player trying to open the door.
                    //
                    // Maybe we need to extend MWWorld::Ptr or MWScript::InterpreterContext so
                    // that it can return the reference of the object taking the action.
                    //
                    // NOTE: based on OMW::Engine::activate() it appears that in TES3 NPC's
                    //       can't activate (probably because input manager initiates it)
                    // NOTE: MWMechanics::AiActivate::execute() seems to allow calling
                    //       MWWorld::activate()
                    MWWorld::Ptr ptr = R()(runtime); // ImplicitRef

                    // get the actor stored by MWWorld::activate()
                    // NOTE: assumes actor is always valid
                    MWWorld::Ptr actor = R()(runtime, true/*required*/, false/*activeOnly*/, true/*actor*/);

                    //std::cout << "IsActionRef: actor " << actor.getCellRef().getRefId() << std::endl;

                    // here the literal string is "player" in CGAmbushAGateScript
                    std::string editorId = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    // FIXME: "playerref" needs support as well?
                    // FIXME: searchPtr needs to search TES4 objects' EditorId
                    MWWorld::Ptr object
                        = MWBase::Environment::get().getWorld()->searchPtr(editorId, false/*activeOnly*/);

                    if (actor == object)
                    {
                        std::cout << "ActionRef is " << editorId << std::endl; // FIXME: temp testing

                        runtime.push (true);
                    }
                    else
                        runtime.push (false);
                }
        };

        template<class R>
        class OpGetStage : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    // assumed that there is a mandatory string argument
                    std::string editorId = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                    const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().search(editorId);

                    // FIXME


                    std::cout << "GetStage: " << editorId << std::endl; // FIXME: temp testing

                    runtime.push (0); // FIXME: just a dummy for testing
                }
        };

        template<class R>
        class OpGetLocked : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    //Interpreter::Type_Integer lockLevel = ptr.getCellRef().getLockLevel();
                    bool isLocked = ptr.getCellRef().isLocked();

                    std::cout << "GetLocked: " << (isLocked ? "true" : "false") << std::endl; // FIXME: temp testing

                    runtime.push (isLocked);
                }
        };

        template<class R>
        class OpActivate : public Interpreter::Opcode1
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime, unsigned int arg0)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    // handle the arguments
                    MWWorld::Ptr actor;
                    Interpreter::Type_Integer flag = 0;
                    if (arg0 >= 1) // explicit
                    {
                        std::string name = runtime.getStringLiteral(runtime[0].mInteger);
                        std::cout << "arg " << name << std::endl;

                        // FIXME: not 100% sure if below method is correct
                        unsigned int ref = runtime.getContext().getLocalRef(runtime[0].mInteger);
                        actor = MWBase::Environment::get().getWorld()->searchPtrViaFormId(ref, true/*activeOnly*/);

                        runtime.pop();
                    }
                    else // implicit
                    {
                        // get the actor stored by MWWorld::activate()
                        actor = R()(runtime, true/*required*/, false/*activeOnly*/, true/*actor*/);
                    }

                    // FIXME: currently below flag is not used (prob. need a new flag in executeActivation)
                    // see https://cs.elderscrolls.com/index.php?title=Activate
                    // If the RunOnActivateFlag is set to 1, then the OnActivate block of the object (if any) will be
                    // run instead of the default activation. (In other words, act just as if ActivatorID activated it
                    // directly -- NPC used it, Player moused over and clicked on it, etc.)
                    if (arg0 >= 2)
                    {
                        // FIXME: not 100% sure if below method is correct
                        flag = runtime[0].mInteger;
                        std::cout << "arg " << flag << std::endl;
                        runtime.pop();
                    }

#if 1  // FIXME: temp testing
                    if (ptr && actor)
                        std::cout << ptr.getCellRef().getRefId() << ".Activate: "
                                  << actor.getCellRef().getRefId() << std::endl;
                    else
                        std::cout << "Activate" << std::endl;
#endif
                    InterpreterContext& context = static_cast<InterpreterContext&> (runtime.getContext());

// FIXME: not quite working, commented out for now
#if 0
                    if (flag)
                        context.executeActivationScript(ptr, actor);
                    else
#endif
                        context.executeActivation(ptr, actor);
                }
        };

        template<class R>
        class OpPlayGroup : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    // FIXME: how to get the 2 arguments?
                    std::string playgroupId = runtime.getStringLiteral(runtime[0].mInteger);
                    runtime.pop();

                    Interpreter::Type_Integer count = runtime[0].mInteger;
                    runtime.pop();

                    // FIXME: need a new method with MWClass

                    std::cout << "PlayGroup: " << playgroupId << " " << count << std::endl; // FIXME: temp testing

                    MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
                    if (anim->hasAnimation("Forward") || anim->hasAnimation("Backward"))
                    {
                        std::cout << "has animation" << std::endl;
                    }


                }
        };

        template<class R>
        class OpGetSelf : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    ESM4::FormId formId = ptr.getCellRef().getFormId();

                    std::cout << "GetSelf: " << ESM4::formIdToString(formId) << std::endl; // FIXME: temp testing

                    InterpreterContext& context = static_cast<InterpreterContext&> (runtime.getContext());
                    std::cout << "mTargetFormId: " << ESM4::formIdToString(context.getTargetFormId()) << std::endl;

                    runtime.push (formId);
                }
        };

        template<class R>
        class OpGetParentRef : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);

                    ESM4::FormId formId = ptr.getCellRef().getParentFormId();

                    std::cout << "GetParerentRef: " << ESM4::formIdToString(formId) << std::endl; // FIXME: temp testing

                    runtime.push (formId);
                }
        };

        template<class R>
        class OpIsAnimPlaying : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {

                    MWWorld::Ptr ptr = R()(runtime);


                    //std::cout << "IsAnimPlaying: " << std::endl; // FIXME: temp testing


                    runtime.push (0); // FIXME: just a dummy for testing
                }
        };

        void installOpcodes (Interpreter::Interpreter& interpreter)
        {
            // IsActionRef has 1 non-optional parameter so the opcode can be segment 5
            // However, Interpreter::installSegment5 only accepts pointers to base class Opcode0.
            // i.e. in order to have 1 argument we need to have the opcode in segment 3
            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeIsActionRef, new OpIsActionRef<ImplicitRef>);
            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeIsActionRefExplicit, new OpIsActionRef<ExplicitRef>);

            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeGetStage, new OpGetStage<ImplicitRef>);
            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeGetStageExplicit, new OpGetStage<ExplicitRef>);

            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeGetLocked, new OpGetLocked<ImplicitRef>);

            // See Extensions::registerInstruction()
            // optional arguments imply segment 3 or segment 4
            interpreter.installSegment3 (Tes4Compiler::Tes4::opcodeActivate, new OpActivate<ImplicitRef>);
            interpreter.installSegment3 (Tes4Compiler::Tes4::opcodeActivateExplicit, new OpActivate<ExplicitTes4Ref>);

            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodePlayGroup, new OpPlayGroup<ImplicitRef>);

            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeGetSelf, new OpGetSelf<ImplicitRef>);

            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeGetParentRef, new OpGetParentRef<ImplicitRef>);

            interpreter.installSegment5 (Tes4Compiler::Tes4::opcodeIsAnimPlaying, new OpIsAnimPlaying<ImplicitRef>);
        }
    }
}
