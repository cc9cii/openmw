/*
  Copyright (C) 2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#include "road.hpp"

#include <stdexcept>
#include <iostream> // FIXME: for debugging only
//#include <iomanip>  // FIXME: for debugging only

//#include <boost/scoped_array.hpp> // FIXME

#include "formid.hpp" // FIXME: for workaround
#include "reader.hpp"
//#include "writer.hpp"

ESM4::Road::Road() : mFormId(0), mFlags(0)
{
    mEditorId.clear();

    mNodes.clear();
    mLinks.clear();
}

ESM4::Road::~Road()
{
}

void ESM4::Road::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    mEditorId = formIdToString(mFormId); // FIXME: quick workaround to use existing code

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_PGRP:
            {
                std::size_t numNodes = subHdr.dataSize / sizeof(PGRP);

                mNodes.resize(numNodes);
                for (std::size_t i = 0; i < numNodes; ++i)
                {
                    reader.get(mNodes.at(i));
                }

                break;
            }
            case ESM4::SUB_PGRR:
            {
#if 0
                boost::scoped_array<unsigned char> mDataBuf(new unsigned char[subHdr.dataSize]);
                reader.get(&mDataBuf[0], subHdr.dataSize);

                std::ostringstream ss;
                ss << ESM4::formIdToString(mFormId) << " " << mEditorId << " "
                /*ss*/ << ESM4::printName(subHdr.typeId) << ":size " << subHdr.dataSize << "\n";
                for (unsigned int i = 0; i < subHdr.dataSize; ++i)
                {
                    //if (mDataBuf[i] > 64 && mDataBuf[i] < 91)
                        //ss << (char)(mDataBuf[i]) << " ";
                    //else
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
                    if ((i & 0x000f) == 0xf)
                        ss << "\n";
                    else if (i < subHdr.dataSize-1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;
#else
                std::cout << "ROAD " << ESM4::printName(subHdr.typeId) << " skipping..."
                        << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
#endif
                break;
            }
            default:
                reader.skipSubRecordData();
                //throw std::runtime_error("ESM4::ROAD::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
    //std::cout << "PGRP " << mNodes.size() << std::endl;
}

//void ESM4::Road::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Road::blank()
//{
//}
