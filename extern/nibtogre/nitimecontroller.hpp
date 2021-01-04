/*
  Copyright (C) 2015-2021 cc9cii

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
#ifndef NIBTOGRE_NITIMECONTROLLER_H
#define NIBTOGRE_NITIMECONTROLLER_H

#include <string>
#include <vector>
#include <cstdint>

#include <OgreVector3.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiTimeController
//     BSFrustumFOVController
//     BSLagBoneController
//     NiBSBoneLODController
//     NiControllerManager
//     NiInterpController <------------------------------- /* NiTimeController */
//         NiGeomMorpherController
//         NiMultiTargetTransformController
//         NiSingleInterpController
//             NiBoolInterpController <------------------- /* not implemented */
//                 NiVisController
//             NiExtraDataController <-------------------- /* not implemented */
//                 NiFloatExtraDataController
//             NiFloatInterpController
//                 BSRefractionStrengthController
//                 BSEffectShaderPropertyColorController
//                 BSEffectShaderPropertyFloatController
//                 BSLightingShaderPropertyColorController
//                 BSLightingShaderPropertyFloatController
//                 NiAlphaController
//                     BSNiAlphaPropertyTestRefController  /* NiAlphaController */
//                 NiFlipController
//                 NiTextureTransformController
//             NiKeyframeController
//                 BSKeyframeController
//                 NiTransformController <---------------- /* NiKeyframeController */
//             NiPSysModifierCtlr
//                 NiPSysEmitterCtlr
//                 NiPSysModifierBoolCtlr <--------------- /* not implemented */
//                     NiPSysModifierActiveCtlr <--------- /* NiPSysModfierCtlr */
//                 NiPSysModifierFloatCtlr
//                     NiPSysEmitterInitialRadiusCtlr <--- /* NiPSysModifierFloatCtlr */
//                     NiPSysEmitterLifeSpanCtlr <-------- /* NiPSysModifierFloatCtlr */
//                     NiPSysEmitterSpeedCtlr <----------- /* NiPSysModifierFloatCtlr */
//                     NiPSysGravityStrengthCtlr <-------- /* NiPSysModifierFloatCtlr */
//             NiPoint3InterpController <----------------- /* not implemented */
//                 NiMaterialColorController
//     NiPSysUpdateCtlr <--------------------------------- /* NiTimeController */
//     NiParticleSystemController
//         NiBSPArrayController <------------------------- /* NiParticleSystemController */
//     NiPathController
//     NiUVController
//     bhkBlendController
//     BSRefractionFirePeriodController
namespace NiBtOgre
{
    class NiObjectNET;
    struct NiTransformInterpolator;
    struct NiInterpolator;
    class NiControllerSequence;

    class NiTimeController : public NiObject
    {
    public:
        NiTimeControllerRef mNextControllerRef;
        // 0x0??? ????
        //    ^^^ ^^^^
        //    ||| |||+- Anim type, 0=APP_TIME 1=APP_INIT
        //    ||| |++-- Cycle type, 00=Loop 01=Reverse 10=Clamp
        //    ||| +---- Active
        //    ||+------ Play backwards
        //    |+------- Is manager controlled
        //    +-------- Always seems to be set in Skyrim and Fallout NIFs, unknown function
        std::uint16_t mFlags;
        float mFrequency;
        float mPhase;
        float mStartTime;
        float mStopTime;
        // lights/candlefat01.nif (TES4) shows that some of the Ptr refer to objects not yet
        // loaded.  Change to Ref instead.
        //NiObjectNET *mTarget; // Ptr
        NiObjectNETRef mTargetRef;

        NiTimeController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        // used by NiGeomMorpherController only?  (what about UV controller?)
        virtual NiTimeControllerRef build(Ogre::Mesh *mesh);

        virtual NiTimeControllerRef build(std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<float> >& controllers);

        // used by NiGeomMorpherController only?
        //virtual void setInterpolator(const std::string& frameName, NiInterpolator *interpolator);
    };

    // Seen in NIF version 20.2.0.7
    class BSFrustumFOVController : public NiTimeController
    {
    public:
        NiFloatInterpolatorRef mInterpolatorRef;

        BSFrustumFOVController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    class BSLagBoneController : public NiTimeController
    {
    public:
        float mLinearVelocity;
        float mLinearRotation;
        float mMaximumDistance;

        BSLagBoneController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    class NiNode;
    struct NiGeometry;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiBSBoneLODController : public NiTimeController
    {
        struct NodeGroup
        {
            std::uint32_t        numNodes;
            // _male/skeleton.nif (TES4) shows that some of the Ptr refer to objects not yet loaded.
            // Change to Ref instead.
            //std::vector<NiNode*> nodes; // Ptr
            std::vector<NiNodeRef> nodeRefs;

            void read(NiStream *stream, const NiModel& model, BuildData& data);
        };

        struct SkinShapeGroup
        {
            struct SkinShape
            {
                NiGeometry       *shape; // Ptr
                NiSkinInstanceRef skinInstanceRef;
            };

            std::uint32_t          numLinkPairs;
            std::vector<SkinShape> linkPairs;

            void read(NiStream *stream, const NiModel& model, BuildData& data);
        };

        std::vector<NodeGroup>         mNodeGroups;
        std::vector<SkinShapeGroup>    mShapeGroups;
        std::vector<NiTriBasedGeomRef> mShapeGroups2;

    public:
        NiBSBoneLODController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiControllerManager : public NiTimeController
    {
    public:
        bool mCumulative;
        std::vector<NiControllerSequenceRef> mControllerSequences;
        NiDefaultAvObjectPaletteRef mObjectPaletteRef;

        NiControllerManager(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        NiTimeControllerRef build(std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<float> >& controllers);

        void getControllerSequenceMap(std::map<std::string, NiControllerSequence*>& result) const;
    };

    typedef NiTimeController NiInterpController;

    class NiAVObject;
    struct NiDefaultAVObjectPalette;
    class NiControllerSequence;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiMultiTargetTransformController : public NiInterpController
    {
        BuildData& mData; // FIXME: should be const
        bool mExtraTargetsBuilt;
        std::vector<NiAVObject*> mExtraTargets;

        std::vector<std::pair<Ogre::Bone*, const NiInterpolator*> > mTargetInterpolators;
        const NiControllerSequence *mControllerSequence;

    public:
        std::uint16_t mNumExtraTargets;
        // Clutter\MinotaurHead01.NIF (TES4) shows that some of the Ptr refer to objects not yet loaded.
        // Change to Ref instead.
        //std::vector<NiAVObject*> mExtraTargets; // Ptr
        std::vector<NiAVObjectRef> mExtraTargetRefs;

        NiMultiTargetTransformController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        NiTimeControllerRef build(const std::vector<NiControllerSequenceRef>& animRefs,
                const NiDefaultAVObjectPalette& objects, std::vector<Ogre::Controller<float> >& controllers);

        // register from NiControllerSequence
        void registerTarget(const NiControllerSequence* sequence,
                const std::string& targetName, const NiTransformInterpolator *interpolator);

        // actual build, chained following NiControllerManager
        NiTimeControllerRef build(std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<float> >& controllers);

        // node animation
        void build(int32_t nameIndex, const NiAVObject* target,
                NiTransformInterpolator *interpolator, float startTime, float stopTime);
    };

    class NiSingleInterpController : public NiInterpController
    {
    public:
        NiInterpolatorRef mInterpolatorRef;

        NiSingleInterpController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    class NiFloatExtraDataController : public NiSingleInterpController
    {
    public:
        std::uint32_t mControllerDataRef;

        NiFloatExtraDataController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiSingleInterpController BSMaterialEmittanceMultController;
    typedef NiSingleInterpController NiFloatInterpController;
    typedef NiFloatInterpController BSRefractionStrengthController;

    // Seen in NIF version 20.2.0.7
    class BSEffectShaderPropertyColorController : public NiSingleInterpController
    {
    public:
        std::uint32_t mUnknownInt1;

        BSEffectShaderPropertyColorController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // FIXME looks identical to BSEffectShaderPropertyColorController
    // Seen in NIF version 20.2.0.7
    class BSEffectShaderPropertyFloatController : public NiSingleInterpController
    {
    public:
        std::uint32_t mTargetVariable;

        BSEffectShaderPropertyFloatController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // FIXME looks identical to BSEffectShaderPropertyColorController
    // Seen in NIF version 20.2.0.7
    class BSLightingShaderPropertyColorController : public NiSingleInterpController
    {
        std::uint32_t mTargetVariable;

    public:
        BSLightingShaderPropertyColorController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // FIXME looks identical to BSEffectShaderPropertyColorController
    // Seen in NIF version 20.2.0.7
    class BSLightingShaderPropertyFloatController : public NiSingleInterpController
    {
        std::uint32_t mTargetVariable;

    public:
        BSLightingShaderPropertyFloatController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiTextureTransformController : public NiSingleInterpController
    {
    public:
        std::uint32_t mTextureSlot;
        std::uint32_t mOperation;
#if 0
        NiFloatDataRef mDataRef;
#endif

        NiTextureTransformController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    class NiLightColorController : public NiSingleInterpController
    {
    public:
        std::uint16_t mTargetColor;

        NiLightColorController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    typedef NiTimeController NiPSysUpdateCtlr; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    class NiPathController : public NiTimeController
    {
    public:
        NiPosDataRef   mPosDataRef;
        NiFloatDataRef mFloatDataRef;

        NiPathController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class bhkBlendController : public NiTimeController
    {
    public:
        std::uint32_t mUnknown;

        bhkBlendController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF ver 20.2.0.7 (FONV)
    class BSRefractionFirePeriodController : public NiTimeController
    {
    public:
        NiInterpolatorRef mInterpolatorRef;

        BSRefractionFirePeriodController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);
    };
}

#endif // NIBTOGRE_NITIMECONTROLLER_H
