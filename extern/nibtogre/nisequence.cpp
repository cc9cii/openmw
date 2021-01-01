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
#include "nisequence.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME

#include <boost/algorithm/string.hpp>

#include <OgreController.h>
#include <OgreSkeletonInstance.h>
#include <OgreControllerManager.h>
#include <OgreBone.h>
#include <OgreEntity.h>

#include "nistream.hpp"
#include "nitimecontroller.hpp" // static_cast NiControllerManager
#include "nigeommorphercontroller.hpp"
#include "nikeyframecontroller.hpp"
#include "nimodel.hpp"
#include "nidata.hpp" // NiStringPalette
#include "niavobject.hpp" // getTargetObject (see: http://www.cplusplus.com/forum/beginner/134129/#msg717505)
#include "niinterpolator.hpp"
#include "transformcontroller.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

// NOTE: assumed that this is not used in TES3 (seems to use NiSequenceStreamHelper),
//       10.2.0.0 (TES4) seems to be the earliest example
void NiBtOgre::NiSequence::ControllerLink::read(NiStream *stream)
{
    if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
    {
        stream->readSizedString(targetName);
        stream->read(controllerRef);
    }

    if (stream->nifVer() >= 0x0a01006a) // from 10.1.0.106
    {
        stream->read(interpolatorRef);
        stream->read(controller2Ref);
        if (stream->nifVer() == 0x0a01006a) // 10.1.0.106
        {
            stream->skip(sizeof(std::uint32_t)); // Unknown Link 2
            stream->skip(sizeof(std::uint16_t)); // Unknown Short 0
        }
        stream->read(priority); // TODO userVer >= 10
    }

    if (stream->nifVer() == 0x0a01006a || stream->nifVer() >= 0x14010003) // from header
    {
        stringPaletteRef = -1;

        stream->readLongString(nodeNameIndex);
        stream->readLongString(propertyTypeIndex);
        stream->readLongString(controllerTypeIndex);
        stream->readLongString(variable1Index);
        stream->readLongString(variable2Index);
    }
    else if (stream->nifVer() >= 0x0a020000 && stream->nifVer() <= 0x14000005) // from string palette
    {
        stream->read(stringPaletteRef); // block index

        stream->read(nodeNameIndex);
        stream->read(propertyTypeIndex);
        stream->read(controllerTypeIndex);
        stream->read(variable1Index); // Controller ID Offset
        stream->read(variable2Index); // Interpolator ID Offset (frame name)
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiSequence::NiSequence(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->readLongString(mNameIndex);

    // probably unused, skip instead?
    if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
    {
        stream->readLongString(mTextKeysNameOLDIndex);
        stream->read(mTextKeysOLDRef);
    }

    std::uint32_t numControlledBlocks;
    stream->read(numControlledBlocks);
    if (stream->nifVer() >= 0x0a01006a) // from 10.1.0.106
        stream->skip(sizeof(std::uint32_t));

    mControlledBlocks.resize(numControlledBlocks);
    for (unsigned int i = 0; i < numControlledBlocks; ++i)
        mControlledBlocks[i].read(stream);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiControllerSequence::NiControllerSequence(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSequence(index, stream, model, data), mData(data)
{
    //if (stream->nifVer() >= 0x0a01006a) // from 10.1.0.106
    //{
        stream->read(mWeight);
        stream->read(mTextKeysRef);
        stream->read(mCycleType);

        if (stream->nifVer() == 0x0a01006a) // 10.1.0.106
            stream->read(mUnknown0);

        stream->read(mFrequency);
        stream->read(mStartTime);

        if (stream->nifVer() >= 0x0a020000 && stream->nifVer() <= 0x0a040001)
            stream->read(mUnknownFloat2);

        stream->read(mStopTime);

        if (stream->nifVer() == 0x0a01006a) // 10.1.0.106
            stream->read(mUnknownByte);

        //stream->getPtr<NiControllerManager>(mManager, model.objects());
        std::int32_t rIndex = -1;
        stream->read(rIndex);
        mManager = model.getRef<NiControllerManager>(rIndex);

        stream->readLongString(mTargetNameIndex);

        if (stream->nifVer() >= 0x0a020000 && stream->nifVer() <= 0x14000005)
            stream->read(mStringPaletteRef);

        if (stream->nifVer() >= 0x14020007 && stream->userVer() >= 11 && (stream->userVer2() >= 24 && stream->userVer2() <= 28))
            stream->read(mAnimNotesRef);

        if (stream->nifVer() >= 0x14020007 && stream->userVer2() > 28)
            stream->read(mUnknownShort1);
    //}
}

// FIXME: below comment blocks are out of date

/*
 * **************************** DO NOT USE AS IS *******************************
 *
 * This method and NiMultiTargetTransformController::build() was designed to use
 * Ogre's vertex animation code.  However, they only work for simple animations
 * e.g. Oblivion's doors.  Complex animations that include child nodes without
 * controllers (some doors and activators in FO3/FONV and the activator for the
 * enterance to Benirus Manor's sealed portal).
 *
 * *****************************************************************************
 */

// Each of the NiControlSequences are "playable" animations.
//
// mTextKeysRef holds text keys such as 'start' 'end' or sound triggers: 'sound: TRPGearsClaws' in
// oblivion/architecture/citadel/interior/citadelsmalltower/oblivionsmalltowergatelatch01.nif
// (probably needs to have actions done through 'inst')
//
// FIXME: how to get the sound object EditorID lookup? probably need to implement something
//        similar to getting world using EditorID, i.e. mStore.getForeign<ForeignWorld>().getFormId(world);
//
// Each of the Controlled Blocks refer to the target node name via the string palette which
// in turn needs to be looked up via the object palette in NiControllerManager.  But how to
// make sense of the palette?  It appears as if the string palette is just a list of all the
// unique strings that need to be looked up:
//
//     strings in the palette     value             index  (deduced from)
//     -------------------------- ----------------- ------ -----------------
//     MainMast02:0               nodeName          118    object palette
//     NiGeomMorpherController    controllerType      7    controller2Ref
//     Base                       interpolatorId           NiMorphData::mFrameName (i.e. pose)
//     MainMastMorphA             interpolatorId           NiMorphData::mFrameName (i.e. pose)
//     MainMastMorphB             interpolatorId           NiMorphData::mFrameName (i.e. pose)
//     MainMast02:3               nodeName          124    object palette
//     MainMast02:4               nodeName          129    object palette
//     MainMast02:6               nodeName          134    object palette
//     MainMast02:7               nodeName          139    object palette
//     MainMast02:8               nodeName          144    object palette
//     MainMast02:15              nodeName          149    object palette
//     MainMast02:23              nodeName          156    object palette
//     MainMast02:25              nodeName          161    object palette
//     MainMast02:41              nodeName          167    object palette
//     MainMast02                 nodeName            0    object palette
//     NiTransformController      controllerType      4    controller2Ref
//     MainMast02 NonAccum        nodeName          111    object palette
//
void NiBtOgre::NiControllerSequence::build(const NiDefaultAVObjectPalette *objects)
{
    // FIXME: process mTextKeysRef here

    // Each of the controlled block maps to an Ogre::AnimationTrack.  i.e. each controller
    // block is for one sub-mesh. There can be more than one block for the same sub-mesh.
    // (one for each pose)
    //
    // The interpolator for each block provides the data for Ogre::KeyFrame.
    //
    // NiGeomMorpherController   Ogre::VertexAnimationTrack   Ogre::VertexPoseKeyFrame
    // NiTransformController     Ogre::NodeAnimationTrack(?)  Ogre::TranformKeyFrame
    //
    for (unsigned int i = 0; i < mControlledBlocks.size(); ++i)
    {
        // FIXME: for testing only
        if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
            assert(mStringPaletteRef == mControlledBlocks[i].stringPaletteRef); // should be the same

        if (mModel.nifVer() < 0x0a01006a) // 10.1.0.106
            throw std::logic_error("NiControllerSequence less than version 10.1.0.106");

        std::int32_t interpolatorRef = mControlledBlocks[i].interpolatorRef;
        if (interpolatorRef < 0) // -1
            continue;

        std::string ctlrTypeName = getObjectName(mControlledBlocks[i].controllerTypeIndex);
        if (ctlrTypeName =="")
            continue;

        // targets are usually NiTriBasedGeom or NiNode
        std::string targetName = getObjectName(mControlledBlocks[i].nodeNameIndex);
        NiAVObject *target = mModel.getRef<NiAVObject>(objects->getObjectRef(targetName));

        if (!target)
            continue;

        //target->setHasAnim();

        if (ctlrTypeName == "NiGeomMorpherController")
        {
            if (mModel.blockType(interpolatorRef) != "NiFloatInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorRef));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiFloatInterpolator *interpolator = mModel.getRef<NiFloatInterpolator>(interpolatorRef);
            if (interpolator->mDataRef < 0) // -1
                continue;

            // targetNode can be nullptr if nodeNameIndex == -1
            if (mControlledBlocks[i].nodeNameIndex == -1)
                continue;

            // Each interpolator type probably has its own animation method get the controller to
            // do the building; pass the interpolator data for the keys
            //
            // NOTE: ingore the chain of controllers here since NiControllerSequence is managing
            // them all?
            //
            // Most of the targets wouldn't have been built yet so just place the data to be picked
            // up later.
            //NiObject::mModel.getRef<NiTimeController>(mControlledBlocks[i].controllerRef)
                //->buildFromNiControllerSequence(controllers, targetIndex, data);
            //
            // For NiGeomMorpherController the target would be a NiTriBasedGeom which corresponds
            // to an Ogre::SubMesh (which can get to its parent Ogre::Mesh).
            //
            // So we need access to a map<NiGeometyRef, Ogre::SubMesh*> probably from BuildData.
            //

            std::string frameName = getObjectName(mControlledBlocks[i].variable2Index);
            if (frameName =="")
                continue;

            NiGeomMorpherController *controller
                = mModel.getRef<NiGeomMorpherController>(mControlledBlocks[i].controller2Ref);

            if (!controller)
                continue;

            // setup the controller for the build later when the sub-mesh gets built on demand
            controller->setInterpolator(this, frameName, interpolator);
        }
        else if (ctlrTypeName == "NiTransformController")
        {
            if (mModel.blockType(interpolatorRef) != "NiTransformInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorRef));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiTransformInterpolator *interpolator = mModel.getRef<NiTransformInterpolator>(interpolatorRef);
            if (interpolator->mDataRef < 0) // -1
                continue;

            // targetNode can be nullptr if nodeNameIndex == -1
            if (mControlledBlocks[i].nodeNameIndex == -1)
                continue;

            NiMultiTargetTransformController *controller
                = mModel.getRef<NiMultiTargetTransformController>(mControlledBlocks[i].controller2Ref);

            if (!controller)
                continue;

            // FIXME: build it again
            controller->build(mNameIndex, target, interpolator, mStartTime, mStopTime);
        }
        //else
            // NiAlphaController
            // NiFlipController
            // NiMaterialColorController      Dungeons\AyleidRuins\Exterior\ARWellGrate01.NIF
            // NiPSysEmitterCtlr              Oblivion\Environment\OblivionSmokeEmitter01.NIF
            // NiPSysEmitterInitialRadiusCtlr
            // NiPSysEmitterLifeSpanCtlr
            // NiPSysEmitterSpeedCtlr
            // NiPSysGravityStrengthCtlr
            // NiTextureTransformController
            // NiVisController
            //std::cout << "unknown controller type " << ctlrTypeName << std::endl;
    }
}

/*
 * **************************** DO NOT USE AS IS *******************************
 *
 * This method was never tested.
 *
 * *****************************************************************************
 */

// the fake skinned animation hack doesn't seem to work on some newer animations
// (e.g. Dungeons\Vault\Halls\VDoor01.NIF, Dungeons\Vault\RoomU\VGearDoor01.NIF)
//
// object               : source                              : lookup from
// -------------------------------------------------------------------------------------
// target name          : controlledBlock.nodeNameIndex       : NiStringPalette
// target NiAVObject    : target name                         : NiDefaultAVObjectPalette
// controller type name : controlledBlock.controllertypeIndex : NiStringPalette
// controller object    : controlledBlock.controller2Ref      : mModel
// interpolator object  : controlledBlock.interpolatorRef     : mModel
// controller id?       : controlledBlock.variable1Index      : NiStringPalette
// frame name           : controlledBlock.variable2Index      : NiStringPalette
void NiBtOgre::NiControllerSequence::buildFO3(const NiDefaultAVObjectPalette& objects, std::multimap<float, std::string>& textKeys, std::vector<Ogre::Controller<float> >& controllers)
{
    if (mTextKeysRef > 0) // -1
    {
        const NiTextKeyExtraData *data = mModel.getRef<NiTextKeyExtraData>(mTextKeysRef);
        for (unsigned int i = 0; i < data->mTextKeys.size(); ++i)
        {
            float time = data->mTextKeys[i].time;
            textKeys.insert(std::make_pair(time, mModel.indexToString(data->mTextKeys[i].text)));
        }
    }

    // TES4 scripts may use lowercase string to identify animations, e.g.:
    //     playgroup forward 1
    // therefore we use should use the lowercase animation id throughout
    // (have not checked if FO3 scripts do the same, but assumed they would)
    std::string animName = boost::to_lower_copy(mModel.indexToString(mNameIndex));
    for (size_t i = 0; i < mControlledBlocks.size(); ++i)
    {
        if (mModel.nifVer() < 0x0a01006a) // 10.1.0.106
            throw std::logic_error("NiControllerSequence less than version 10.1.0.106");

        std::int32_t interpolatorRef = mControlledBlocks[i].interpolatorRef;
        if (interpolatorRef < 0) // -1
            continue;

        std::string ctlrTypeName = getObjectName(mControlledBlocks[i].controllerTypeIndex);
        if (ctlrTypeName =="")
            continue;

        // targets are usually NiTriBasedGeom or NiNode
        std::string targetName = getObjectName(mControlledBlocks[i].nodeNameIndex);
        NiAVObject *target = mModel.getRef<NiAVObject>(objects.getObjectRef(targetName));

        if (!target)
            continue;
        else
            mData.addAnimBoneName(animName, targetName);

        if (ctlrTypeName == "NiGeomMorpherController")
        {
        }
        else if (ctlrTypeName == "NiTransformController")
        {
            if (mModel.blockType(interpolatorRef) != "NiTransformInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorRef));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiTransformInterpolator *interpolator = mModel.getRef<NiTransformInterpolator>(interpolatorRef);
            if (interpolator->mDataRef < 0) // -1
                continue;

            // targetNode can be nullptr if nodeNameIndex == -1
            if (mControlledBlocks[i].nodeNameIndex == -1)
                continue;

            std::int32_t controllerRef = mControlledBlocks[i].controller2Ref;
            std::string controllerType =mModel.blockType(controllerRef);
            if (controllerType == "NiTransformController")
            {
                NiTransformController *controller
                    = mModel.getRef<NiTransformController>(controllerRef);

                if (controller)
                    controller->build(target, *interpolator, controllers); // build
            }
            else if (controllerType == "NiMultiTargetTransformController")
            {
                NiMultiTargetTransformController *controller
                    = mModel.getRef<NiMultiTargetTransformController>(controllerRef);

                if (controller)
                    controller->registerTarget(this, targetName, interpolator); // register
            }
            else
                throw std::logic_error("NiTransformController expected but not found");
        }
    }
}

// FIXME: duplicated code
// FIXME: if the target doesn't exist should throw or return 'false' so that the animation is
// not registered - this can happen if the character does not have a bow equipped but a bow
// animation is selected
//
// Unlike above, this method builds Ogre::Controller based animation for another NiModel and
// Skeleton (e.g. skeleton.nif) i.e. the 'kf' animation for characters/creatures
//
// That means the target NiAVObjects and NiTimeControllers are in the other model, not the 'kf'
// animation file.  The animation NiModel supplies the interpolators and their data.
void NiBtOgre::NiControllerSequence::build(Ogre::Entity *skelBase, NiModelPtr animModel,
        std::multimap<float, std::string>& textKeys,
        std::vector<Ogre::Controller<float> >& controllers,
        const NiModel& targetModel, const std::map<std::string, NiAVObjectRef>& objRefMap)
{

// find "Accum Root Name" (mTargetNameIndex) and ensure that there is a corresponding node in
// the supplied skeleton

    // TES4 scripts can use lowercase names when specifying animations
    std::string animName = boost::to_lower_copy(mModel.indexToString(mNameIndex));

    const NiTextKeyExtraData *data = mModel.getRef<NiTextKeyExtraData>(mTextKeysRef);
    for (unsigned int i = 0; i < data->mTextKeys.size(); ++i)
    {
        // meshes\dungeons\chargen\cgprisoncellgate01.nif in ImperialDungeon01 has a TextKey without a name value
        if (data->mTextKeys[i].text == -1)
            continue;

        float time = data->mTextKeys[i].time;

        if (animModel.get() != &targetModel)
            textKeys.insert(std::make_pair(time, mModel.indexToString(data->mTextKeys[i].text)));
        else
            // need to put the TextKeys in TES3 format for now
            textKeys.insert(std::make_pair(time, animName + ": " + mModel.indexToString(data->mTextKeys[i].text)));
    }

    for (unsigned int i = 0; i < mControlledBlocks.size(); ++i)
    {
        // FIXME: for testing only
        if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
            assert(mStringPaletteRef == mControlledBlocks[i].stringPaletteRef); // should be the same

        if (mModel.nifVer() < 0x0a01006a) // 10.1.0.106
            throw std::logic_error("NiControllerSequence less than version 10.1.0.106");

        std::int32_t interpolatorRef = mControlledBlocks[i].interpolatorRef;
        if (interpolatorRef < 0) // -1
            continue;

        std::string ctlrTypeName = getObjectName(mControlledBlocks[i].controllerTypeIndex);
        if (ctlrTypeName == "")
            continue;

        // targets are either bones (i.e. NiNode) for NiTransformControllers or
        // NiTriBasedGeom for NiGeomMorpherControllers
        NiAVObject* target;
        std::string targetName = getObjectName(mControlledBlocks[i].nodeNameIndex);
        std::map<std::string, NiAVObjectRef>::const_iterator it = objRefMap.find(targetName);
        if (it != objRefMap.cend())
            target = targetModel.getRef<NiAVObject>(it->second);  // NOTE: target is in another model
        else
            continue;

        if (ctlrTypeName == "NiGeomMorpherController")
        {
            if (mModel.blockType(interpolatorRef) != "NiFloatInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorRef));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiFloatInterpolator *interpolator = mModel.getRef<NiFloatInterpolator>(interpolatorRef);
            if (interpolator->mDataRef < 0) // -1
                continue;

            // For NiGeomMorpherController the target would be a NiTriBasedGeom which corresponds
            // to an Ogre::SubMesh (which can get to its parent Ogre::Mesh).
            //
            // So we need access to a map<NiGeometyRef, Ogre::SubMesh*> probably from BuildData.
            //

            std::string frameName = getObjectName(mControlledBlocks[i].variable2Index);
            if (frameName =="")
                continue;

            NiGeomMorpherController *controller;
            // NOTE: if animModel is the same as targetModel (e.g. activators) then below check does not apply
            if (mControlledBlocks[i].controller2Ref != -1 && animModel.get() != &targetModel)
                //controller = mModel.getRef<NiGeomMorpherController>(mControlledBlocks[i].controller2Ref);
                throw std::logic_error("anim file shouldn't have controllers: "
                        +mModel.blockType(mControlledBlocks[i].controller2Ref));
            else
                controller = static_cast<NiGeomMorpherController*>(target->findController(ctlrTypeName));

            if (!controller)
                continue;

            // for bows only?  how to get the bow model?
            //
            // Some animations only make sense when weapons are attached to skeleton.nif.
            // e.g. NiControllerSequence "AttackBow" has 2 controlled blocks named "Bow:0"
            //        with frame names "Base" and "BowMorph"
            //      Weapons\Iron\Bow.NIF has NiTriStrips named "Bow:0" whose NiTimeController is a
            //        NiGeomMorpherController and its NiMorphData has two morphs with frame names
            //        "Base" and "BowMorph"
            //
            // This implies that some animations should only be loaded once the weapon is equipped.
            // (different bows have different mesh and hence will have different poses)
            //
            // setup the controller for the build later when the sub-mesh gets built on demand
            //controller->setInterpolator(this, frameName, interpolator);
        }
        else if (ctlrTypeName == "NiTransformController")
        {
            if (mModel.blockType(interpolatorRef) != "NiTransformInterpolator" &&
                mModel.blockType(interpolatorRef) != "NiBSplineCompTransformInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorRef));

            // NOTE: Some interpolators do not have any data!
            //       go ahead and build the controllers but they'll take no action
            NiInterpolator *interpolator;
            if (mModel.blockType(interpolatorRef) == "NiTransformInterpolator")
            {
                interpolator = mModel.getRef<NiTransformInterpolator>(interpolatorRef);
                if (0)//static_cast<NiTransformInterpolator*>(interpolator)->mDataRef < 0) // -1
                    continue;
            }
            else if (mModel.blockType(interpolatorRef) == "NiBSplineCompTransformInterpolator")
            {
                interpolator = mModel.getRef<NiBSplineCompTransformInterpolator>(interpolatorRef);
                if (0)//static_cast<NiBSplineCompTransformInterpolator*>(interpolator)->mSplineDataIndex < 0 ||
                    //static_cast<NiBSplineCompTransformInterpolator*>(interpolator)->mBasisDataIndex < 0) // -1
                    continue;
            }

            // find the NiTransformController attached to the target bone
            // to do that need NiModel of skeleton.nif and a lookup map (i.e. object palette)
            // NOTE: if animModel is the same as targetModel (e.g. activators) then below check does not apply
            NiTransformController *controller;
            if (mControlledBlocks[i].controller2Ref != -1 && animModel.get() != &targetModel)
                throw std::logic_error("anim file shouldn't have controllers: "
                        +mModel.blockType(mControlledBlocks[i].controller2Ref));
            else
                controller = static_cast<NiTransformController*>(target->findController(ctlrTypeName));

            // FIXME: TES5 skeletons do not have controllers
            if (0)//!controller)
            {
                std::cout << "NiSequence: missing controller " << ctlrTypeName << std::endl;
                continue;
            }

            // target names are the same as bones
//#if 0
            if (targetName == "")
                throw std::runtime_error("no bone name");
//#endif
            Ogre::SkeletonInstance *skeleton = skelBase->getSkeleton();
            if (!skeleton->hasBone(targetName))
            {
                std::cout << "NiSequence: missing bone " << targetName << std::endl;
                continue;
            }

            Ogre::Bone *bone = skeleton->getBone(targetName);

            // FIXME: test if we need to supply this
            Ogre::ControllerValueRealPtr srcval;// = Ogre::ControllerManager::getSingleton().getFrameTimeSource();
            Ogre::ControllerValueRealPtr dstval(OGRE_NEW TransformController::Value(bone, animModel, interpolator));
            Ogre::ControllerFunctionRealPtr func(OGRE_NEW TransformController::Function(this, false));

            controllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
        }
        else if (ctlrTypeName == "NiFloatExtraDataController")
        {
            if (mModel.blockType(interpolatorRef) != "NiFloatInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorRef));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiFloatInterpolator *interpolator = mModel.getRef<NiFloatInterpolator>(interpolatorRef);
            if (interpolator->mDataRef < 0) // -1
                continue;

            NiFloatExtraDataController *controller;
            if (mControlledBlocks[i].controller2Ref != -1)
                //controller = mModel.getRef<NiGeomMorpherController>(mControlledBlocks[i].controller2Ref);
                throw std::logic_error("anim file shouldn't have controllers: "
                        +mModel.blockType(mControlledBlocks[i].controller2Ref));
            else
                controller = static_cast<NiFloatExtraDataController*>(target->findController(ctlrTypeName));

            if (!controller)
                std::cout << ctlrTypeName << " " << targetName << " has no controller" << std::endl;

            // FIXME: not sure what to do with this (i.e. how to implement head tracking with this)
            //        maybe rotation angle in radians?
            continue;

            // FO3 animName = "meshes\\characters\\_male\\locomotion\\mtidle.kf"
            // controlled block "Bip01 Head" - should be target NiNode block 121 in skeleton.nif
            // (which has block 125 NifloatExtraDataController)
            // controller id "HeadTrack"
        }
        else if (ctlrTypeName == "NiVisController")
        {
            // FIXME: probably turns visibility on/off, but what exactly?
            continue;

            // FO3 animName = "meshes\\characters\\_male\\locomotion\\mtidle.kf"
            // controlled block "HeadAnims" block 134
            // NiBoolInterpolator
            // (the skeleton has the data and keys? see blocks 138 and 139)
        }
        else
            std::cout << "unknown controller type " << ctlrTypeName << std::endl;
    }
}

std::string NiBtOgre::NiControllerSequence::getObjectName(std::uint32_t stringOffset) const
{
    if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
    {
        if (stringOffset == 0xffffffff) // -1, horrible hack
            return "";

        NiStringPalette *palette = mModel.getRef<NiStringPalette>(mStringPaletteRef);
        //if (!palette)
            //return ""; // morroblivion\flora\bushes\corkbulb01anim.nif

        size_t len = palette->mPalette.find_first_of('\0', stringOffset) - stringOffset;
        return palette->mPalette.substr(stringOffset, len);
    }
    else //if (mModel.nifVer() == 0x0a01006a || mModel.nifVer() >= 0x14010003)
    {
        return mModel.indexToString(stringOffset); // for these versions stringOffset is objectIndex
    }
}
