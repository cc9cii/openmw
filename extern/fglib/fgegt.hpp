/*
  Copyright (C) 2019-2020 cc9cii

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
#ifndef FGLIB_FGEGT_H
#define FGLIB_FGEGT_H

#include <cstdint>
#include <vector>
#include <string>

#include <OgreVector3.h>

namespace FgLib
{
    class FgEgt
    {
        std::uint64_t mFileType;
        std::uint32_t mNumRows;
        std::uint32_t mNumColumns;
        std::uint32_t mNumSymTextureModes;  // should be 50
        std::uint32_t mNumAsymTextureModes; // should be 0
        std::uint32_t mTextureBasisVersion;

        std::vector<Ogre::Vector3> mSymTextureModes;  // image starts at top left corner
        std::vector<Ogre::Vector3> mAsymTextureModes; // should be empty
        std::vector<float> mSymTextureModeScales;     // should be 50
        std::vector<float> mAsymTextureModeScales;    // should be 0

        // default, copy and assignment not allowed
        FgEgt();
        FgEgt(const FgEgt& other);
        FgEgt& operator=(const FgEgt& other);

    public:
        // The parameter 'name' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        FgEgt(const std::string& name);
        ~FgEgt();

        inline std::uint32_t numRows() const { return mNumRows; }
        inline std::uint32_t numColumns() const { return mNumColumns; }
        inline std::uint32_t numSymTextureModes() const { return mNumSymTextureModes; }
      //inline std::uint32_t numAsymTextureModes() const { return mNumAsymTextureModes; }

        inline const std::vector<Ogre::Vector3>& symTextureModes() const { return mSymTextureModes; }
        inline const std::vector<Ogre::Vector3>& asymTextureModes() const { return mAsymTextureModes; }
    };
}

#endif // FGLIB_FGEGT_H
