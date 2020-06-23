#ifndef CSM_FOREIGN_IDRECORD_H
#define CSM_FOREIGN_IDRECORD_H

#include <string>

#include <extern/esm4/formid.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    template<typename RecordT>
    struct IdRecord : public RecordT
    {
        static unsigned int sRecordId;

        IdRecord() { mId.clear(); }
        ~IdRecord() {}

        std::string mId;

        virtual void load(ESM4::Reader& reader);

        void blank() {}
    };

    template<typename RecordT>
    void IdRecord<RecordT>::load(ESM4::Reader& reader)
    {
        RecordT::load(reader);

        ESM4::formIdToString(RecordT::mFormId, mId);
    }
}
#endif // CSM_FOREIGN_IDRECORD_H
