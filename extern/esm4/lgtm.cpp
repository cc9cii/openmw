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

  Also see https://tes5edit.github.io/fopdoc/ for FO3/FONV specific details.

*/
#include "lgtm.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::LightingTemplate::LightingTemplate() : mFormId(0), mFlags(0)
{
    mEditorId.clear();

    mLighting.ambient = 0;
    mLighting.directional = 0;
    mLighting.fogColor = 0;
    mLighting.fogNear = 0.f;
    mLighting.fogFar = 0.f;
    mLighting.rotationXY = 0;
    mLighting.rotationZ = 0;
    mLighting.fogDirFade = 0.f;
    mLighting.fogClipDist = 0.f;
    mLighting.fogPower = FLT_MAX; // hack way to detect TES4
}

ESM4::LightingTemplate::~LightingTemplate()
{
}

void ESM4::LightingTemplate::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId);  break;
            case ESM4::SUB_DATA:
            {
                if (subHdr.dataSize == 36) // TES4
                    reader.get(&mLighting, 36);
                if (subHdr.dataSize == 40) // FO3/FONV
                    reader.get(mLighting);
                else if (subHdr.dataSize == 92) // TES5
                {
                    reader.get(mLighting);
                    reader.skipSubRecordData(52); // FIXME
                }
                else
                    reader.skipSubRecordData(); // throw?

                break;
            }
            case ESM4::SUB_DALC: // TES5
            {
                //std::cout << "LGTM " << ESM4::printName(subHdr.typeId) << " skipping..."
                          //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::LGTM::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::LightingTemplate::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::LightingTemplate::blank()
//{
//}