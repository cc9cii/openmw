/*
  Copyright (C) 2015 cc9cii

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

*/
#include "land.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Land::Land()
{
}

ESM4::Land::~Land()
{
}

void ESM4::Land::LandData::transposeTextureData(const std::uint16_t *in, std::uint16_t *out)
{
    int readPos = 0; //bit ugly, but it works
    for ( int y1 = 0; y1 < 4; y1++ )
        for ( int x1 = 0; x1 < 4; x1++ )
            for ( int y2 = 0; y2 < 4; y2++)
                for ( int x2 = 0; x2 < 4; x2++ )
                    out[(y1*4+y2)*16+(x1*4+x2)] = in[readPos++];
}

//             overlap north
//
//         32
//         31
//         30
// overlap  .
//  west    .
//          .
//          2
//          1
//          0
//           0 1 2 ... 30 31 32
//
//             overlap south
//
void ESM4::Land::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    std::int8_t currentQuadrant = -1; // for VTXT following ATXT

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_DATA: // flags
            {
                reader.get(mLandFlags);
                break;
            }
            case ESM4::SUB_VNML: // vertex normals, 33x33x(1+1+1) = 3267
            {
                reader.get(mLandData.mVertNorm);
                break;
            }
            case ESM4::SUB_VHGT: // vertex height gradient, 4+33x33+3 = 4+1089+3 = 1096
            {
#if 0
                reader.get(mHeightMap.heightOffset);
                reader.get(mHeightMap.gradientData);
                reader.get(mHeightMap.unknown1);
                reader.get(mHeightMap.unknown2);
                // FIXME: debug only
                //std::cout << "mHeightMap offset " << mHeightMap.heightOffset << std::endl;
#endif
                static VHGT vhgt;
                reader.get(vhgt);

                //mLandData.mHeightOffset = vhgt.heightOffset; // FIXME: probably not used
                float rowOffset = vhgt.heightOffset;
                for (int y = 0; y < VERTS_SIDE; y++)
                {
                    rowOffset += vhgt.gradientData[y * VERTS_SIDE];

                    mLandData.mHeights[y * VERTS_SIDE] = rowOffset * HEIGHT_SCALE;

                    float colOffset = rowOffset;
                    for (int x = 1; x < VERTS_SIDE; x++)
                    {
                        colOffset += vhgt.gradientData[y * VERTS_SIDE + x];
                        mLandData.mHeights[x + y * VERTS_SIDE] = colOffset * HEIGHT_SCALE;
                    }
                }

                break;
            }
            case ESM4::SUB_VCLR: // vertex colours, 24bit RGB, 33x33x(1+1+1) = 3267
            {
                reader.get(mLandData.mVertColr);
                break;
            }
            case ESM4::SUA_BTXT:
            {
                BTXT base;
                if (reader.get(base))
                {
                    assert(base.quadrant < 4 && base.quadrant >= 0 && "base texture quadrant index error");

                    mLandData.mTextures[base.quadrant].base = base;  // FIXME: any way to avoid double-copying?
                    //std::cout << "Base Texture formid: 0x"
                        //<< std::hex << mTextures[base.quadrant].base.formId << std::endl;
                }
                break;
            }
            case ESM4::SUB_ATXT:
            {
                ATXT add;
                reader.get(add);
                assert(add.quadrant < 4 && add.quadrant >= 0 && "additional texture quadrant index error");

                mLandData.mTextures[add.quadrant].additional = add;  // FIXME: any way to avoid double-copying?
#if 0
                std::cout << "Additional Texture formId: 0x"
                    << std::hex << mTextures[add.quadrant].additional.formId << std::endl;
                std::cout << "Additional Texture layer: "
                    << std::dec << (int)mTextures[add.quadrant].additional.layer << std::endl;
#endif
                currentQuadrant = add.quadrant;
                break;
            }
            case ESM4::SUB_VTXT:
            {
                // Cannot assume that at least one VTXT record follows ATXT
                assert(currentQuadrant != -1 && "VTXT without ATXT found");

                int count = (int)reader.subRecordHeader().dataSize / sizeof(ESM4::Land::VTXT);
                int remainder = reader.subRecordHeader().dataSize % sizeof(ESM4::Land::VTXT);
                assert(remainder == 0 && "ESM4::Land::VTXT data size error");

                if (count)
                {
                    mLandData.mTextures[currentQuadrant].data.resize(count);
                    std::vector<ESM4::Land::VTXT>::iterator it = mLandData.mTextures[currentQuadrant].data.begin();
                    for (;it != mLandData.mTextures[currentQuadrant].data.end(); ++it)
                    {
                        reader.get(*it);
                        // FIXME: debug only
                        //std::cout << "pos: " << std::dec << (int)(*it).position << std::endl;
                    }
                }
                currentQuadrant = -1;
                break;
            }
            case ESM4::SUB_VTEX: // only in Oblivion?
            {
                int count = (int)reader.subRecordHeader().dataSize / sizeof(std::uint32_t);
                int remainder = reader.subRecordHeader().dataSize % sizeof(std::uint32_t);
                assert(remainder == 0 && "ESM4::Land::VTEX data size error");

                if (count)
                {
                    mLandData.mIds.resize(count);
                    for (std::vector<std::uint32_t>::iterator it = mLandData.mIds.begin(); it != mLandData.mIds.end(); ++it)
                    {
                        reader.get(*it);
                        // FIXME: debug only
                        //std::cout << "VTEX: " << std::hex << *it << std::endl;
                    }
                }
                break;
            }
            default:
                throw std::runtime_error("ESM4::LAND::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Land::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Land::blank()
//{
//}
