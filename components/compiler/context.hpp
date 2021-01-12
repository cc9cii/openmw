#ifndef COMPILER_CONTEXT_H_INCLUDED
#define COMPILER_CONTEXT_H_INCLUDED

#include <cstdint>
#include <string>

namespace ESM4
{
    typedef std::uint32_t FormId; // avoid dependency to extern/esm4/formid.hpp
}

namespace Compiler
{
    class Extensions;

    class Context
    {
            const Extensions *mExtensions;

        public:

            Context() : mExtensions (0) {}

            virtual ~Context() {}

            virtual bool canDeclareLocals() const = 0;
            ///< Is the compiler allowed to declare local variables?

            void setExtensions (const Extensions *extensions = 0)
            {
                mExtensions = extensions;
            }

            const Extensions *getExtensions() const
            {
                return mExtensions;
            }

            virtual char getGlobalType (const std::string& name) const = 0;
            ///< 'l: long, 's': short, 'f': float, 'r': ref, ' ': does not exist.

            virtual std::pair<char, bool> getMemberType (const std::string& name,
                const std::string& id, std::string *scriptId = nullptr) const = 0;
            ///< Return type of member variable \a name in script \a id or in script of reference of
            /// \a id
            /// \return first: 'l: long, 's': short, 'f': float, 'r': ref, ' ': does not exist.
            /// second: true: script of reference

            virtual bool isId (const std::string& name) const = 0;
            ///< Does \a name match an ID, that can be referenced?

            virtual bool isJournalId (const std::string& name) const = 0;
            ///< Does \a name match a journal ID?

            //virtual bool isEditorId (const std::string& name) const = 0;

            //virtual bool isQuestId (const std::string& name) const = 0;

            virtual ESM4::FormId getReference (const std::string& editorId) const = 0;
            ///< Return the \a FormId of an object reference, identified by its reference
            /// \a EditorId, in currently active cells.  Return 0 if none found.

            virtual int32_t getAIPackage (const std::string& lowerEditorId) const = 0;
    };
}

#endif
