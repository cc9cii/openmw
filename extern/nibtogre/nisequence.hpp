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
#ifndef NIBTOGRE_NISEQUENCE_H
#define NIBTOGRE_NISEQUENCE_H

#include <string>
#include <vector>
#include <cstdint>

#include <OgrePrerequisites.h>

#include "niobject.hpp"
#include "nimodelmanager.hpp"

namespace Ogre
{
    template<class T> class Controller;
}
// Based on NifTools/NifSkope/doc/index.html
//
// NiSequence
//     NiControllerSequence
namespace NiBtOgre
{
    class NiSequence : public NiObject
    {
    protected:
        struct ControllerLink
        {
            // up to 10.1.0.0
            std::string targetName;
            NiTimeControllerRef controllerRef;

            // from 10.1.0.106
            NiInterpolatorRef interpolatorRef;
            NiTimeControllerRef controller2Ref;
            unsigned char priority;

            // 10.1.0.106 and 20.1.0.3 onwards index from header string
            // 10.2.0.0 to 20.0.0.5 offset from string palette (can be -1)
            std::uint32_t nodeNameIndex;    // target node name
            std::uint32_t propertyTypeIndex;
            std::uint32_t controllerTypeIndex;
            std::uint32_t variable1Index;
            std::uint32_t variable2Index;

            NiStringPaletteRef stringPaletteRef;

            void read(NiStream *stream);
        };

        std::uint32_t mNameIndex;              // name of the animation e.g. "Idle"
        std::uint32_t mTextKeysNameOLDIndex;   // probably unused by TES3/4/5
        NiTextKeyExtraDataRef mTextKeysOLDRef; // probably unused by TES3/4/5
        std::vector<ControllerLink> mControlledBlocks;

    public:
        NiSequence(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    class NiControllerManager;
    struct NiDefaultAVObjectPalette;
    class NiAVObject;

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    class NiControllerSequence : public NiSequence
    {
        float mWeight;
        NiTextKeyExtraDataRef mTextKeysRef;
        std::uint32_t mCycleType;
        std::uint32_t mUnknown0;
        float mFrequency;
        float mStartTime;
        float mUnknownFloat2;
        float mStopTime;
        char  mUnknownByte;
        NiControllerManager *mManager; // Ptr
        std::uint32_t mTargetNameIndex;
        NiStringPaletteRef mStringPaletteRef;

        BuildData& mData;

        BSAnimNotesRef mAnimNotesRef; // TES5
        std::int16_t mUnknownShort1;  // TES5

    public:
        NiControllerSequence(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        void build(const NiDefaultAVObjectPalette *objects);

        void buildFO3(const NiDefaultAVObjectPalette& objects, std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<float> >& controllers);

        void build(Ogre::Entity *skelBase, NiModelPtr anim,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<float> >& controllers,
                const NiModel& targetModel, const std::map<std::string, NiAVObjectRef>& objects);

        inline float getFrequency() const { return mFrequency;     }
      //inline float getPhase()     const { return mUnknownFloat2; }
        inline float getStartTime() const { return mStartTime;     }
        inline float getStopTime()  const { return mStopTime;      }

        std::string getObjectName(std::uint32_t stringOffset) const;
        uint32_t getNameIndex() const { return NiSequence::mNameIndex; }
        float getTotalAnimLength() const { return mStopTime - mStartTime; }
    };
}

#endif // NIBTOGRE_NISEQUENCE_H
