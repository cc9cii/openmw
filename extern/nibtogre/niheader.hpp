/*
  Copyright (C) 2015-2020 cc9cii

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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#ifndef NIBTOGRE_NIHEADER_H
#define NIBTOGRE_NIHEADER_H

#include <string>
#include <vector>
#include <cstdint>

namespace NiBtOgre
{
    class NiStream;
    //class NiSkinInstance;
    class NiObject;

    // The header contains the NIF file version and a list of all the object names used in the file.
    // The object names are then used to create the corresponding classes.
    // TODO: factory pattern using a simple map keyed on NIF object names?
    //
    // NOTE: Any exceptions thrown by the stream reader are not caught in the constructor.  It is
    // expected that exeptions are caught and handed upstream (e.g. abort loading the whole file)
    class NiHeader
    {
        std::uint32_t mVer;
        std::uint32_t mUserVer;
        std::uint32_t mUserVer2;

        std::uint32_t mNumBlocks;

        std::string   mCreator;      // not used
        std::string   mExportInfo1;  // not used
        std::string   mExportInfo2;  // not used

        std::vector<std::string>   mBlockTypes;
        std::vector<std::int16_t>  mBlockTypeIndex;
        std::vector<std::uint32_t> mBlockSize; // TES5 only, probably for sanity check
        std::vector<std::string>   mStrings;   // originally TES5 only but now for all

        static std::string   mEmptyString;

        // default, copy and assignment not allowed
        NiHeader();
        NiHeader(const NiHeader& other);
        NiHeader& operator=(const NiHeader& other);

    public:
        NiHeader(NiStream *stream); // may throw std::runtime_error
        ~NiHeader() {}

        inline std::uint32_t nifVer() const { return mVer; }
        inline std::uint32_t userVer() const { return mUserVer; }
        inline std::uint32_t userVer2() const { return mUserVer2; }

        // FIXME: bounds check is a bit naive?
        const std::string& indexToString(std::int32_t index) const {
            try {
                return mStrings.at(index);
            }
            catch (...) { // out of bounds
                return mEmptyString;
            }
        }

        // returns index
        std::int32_t appendLongString(std::string&& str);

        // returns the number of NiObjects
        inline std::uint32_t numBlocks() const { return mNumBlocks; }

        // returns NiObject type name for creation via a factory
        const std::string& blockType(std::uint32_t index) const;

        //void getNiSkinInstances(std::vector<NiSkinInstance*>& skins,
                                //std::vector<std::unique_ptr<NiObject> >& objects);

        // returns -1 if none found
        std::int32_t searchStrings(const std::string& str) const;

        // for inserting dummy blocks (landscape LOD mesh)
        void addBlockType(const std::string& blockType);

        std::uint32_t addString(const std::string& str);
    };
}

#endif // NIBTOGRE_NIHEADER_H
