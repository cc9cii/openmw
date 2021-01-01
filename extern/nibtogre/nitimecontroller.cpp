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
#include "nitimecontroller.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <boost/algorithm/string.hpp>

#include <OgreSkeleton.h>
#include <OgreKeyFrame.h>
#include <OgreBone.h>
#include <OgreController.h>
#include <OgreControllerManager.h>
#include <OgrePrerequisites.h>

#include "nistream.hpp"
#include "nigeometry.hpp"  // static_cast NiGeometry
#include "nimodel.hpp"
#include "nisequence.hpp"
#include "nidata.hpp" // NiDefaultAVObjectPalette
#include "niavobject.hpp"
#include "niinterpolator.hpp"
#include "ninode.hpp"
#include "transformcontroller.hpp"
#include "nimodelmanager.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// below code is copied from (or based on) NifSkope gl/glcontroller.cpp
namespace
{
template<typename T>
T interpolateKey (const NiBtOgre::KeyGroup<T>& keyGroup, /*uint32_t cycleType,*/ float time)
{
    if(time <= keyGroup.indexMap.begin()->first)
        return keyGroup.keys[keyGroup.indexMap.begin()->second].value;

    std::map<float, int>::const_iterator nextIt = keyGroup.indexMap.lower_bound(time); // >=
    if (nextIt != keyGroup.indexMap.end())
    {
        std::map<float, int>::const_iterator lastIt = nextIt;
        --lastIt; // point to prev key

        float nextTime = nextIt->first;
        float lastTime = lastIt->first;
        float x = (time - lastTime) / (nextTime - lastTime);

        if (keyGroup.interpolation == 5)      // CONST
        {
            if ( x < 0.5f ) // step up at half way (or should it be step up at 1.0f instead?)
                return keyGroup.keys[lastIt->second].value;
            else
                return keyGroup.keys[nextIt->second].value;
        }
        else if (keyGroup.interpolation == 2) // QUADRATIC
        {
            const T& t1 = keyGroup.keys[lastIt->second].backward;
            const T& t2 = keyGroup.keys[nextIt->second].forward;
            const T& v1 = keyGroup.keys[lastIt->second].value;
            const T& v2 = keyGroup.keys[nextIt->second].value;
            float x2 = x * x;
            float x3 = x2 * x;

            // Cubic Hermite spline
            //    x(t) = (2t^3 - 3t^2 + 1)P1  + (-2t^3 + 3t^2)P2 + (t^3 - 2t^2 + t)T1 + (t^3 - t^2)T2
            return  v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);
        }
        else if (keyGroup.interpolation == 1) // LINEAR
        {
            const T& v1 = keyGroup.keys[lastIt->second].value;
            const T& v2 = keyGroup.keys[nextIt->second].value;

            return v1 + (v2 - v1) * x;
        }
        else if (keyGroup.interpolation == 3) // TBC
        {
            // oblivion\seige\siegecrawlerdeath.nif
            // FIXME: use linear for now
            const T& v1 = keyGroup.keys[lastIt->second].value;
            const T& v2 = keyGroup.keys[nextIt->second].value;

            return v1 + (v2 - v1) * x;
        }
        else
            throw std::runtime_error("Unsupported interpolation type");
    }
    else // no key has >= time; i.e. time is greater than any of the keys'
    {
        // FIXME: do we ever get here?
        //if (cycleType == 2) // CYCLE_CLAMP
            return keyGroup.keys[keyGroup.indexMap.rbegin()->second].value;
        //else
            //return keyGroup.keys[keyGroup.indexMap.begin()->second].value; // wrap around
    }
}

template<typename T>
bool timeIndex( float time, const std::vector<NiBtOgre::Key<T> >& frames, int & last, int & next, float & x )
{
    int count = static_cast<int>(frames.size());

    if (count > 0)
    {
        if ( time <= frames[0].time) {         // <= first key
            last = next = 0;
            x = 0.0f;

            return true;
        }

        //  hmmm... shouldn't this wrap around instead?
        if ( time >= frames[count - 1].time) { // >= last key
            last = next = count - 1;
            x = 0.0f;

            return true;
        }

        if ( last < 0 || last >= count )
            last = 0;

        float tI = frames[last].time;

        if ( time > tI ) {
            next = last + 1;
            float tJ;

            while ( time >= ( tJ = frames[next].time ) ) {
                last  = next++;
                tI = tJ;
            }

            x = ( time - tI ) / ( tJ - tI );

            return true;
        } else if ( time < tI ) {
            next = last - 1;
            float tJ;

            while ( time <= ( tJ = frames[next].time ) ) {
                last  = next--;
                tI = tJ;
            }

            x = ( time - tI ) / ( tJ - tI );

            // Quadratic Bug Fix
            // Invert x
            //    Previously, this branch was causing x to decrement from 1.0.
            //    (This works fine for linear interpolation apparently)
            x = 1.0f - x;

            // Swap I and J
            //    With x inverted, we must swap I and J or the animation will reverse.
            auto tmpI = last;
            last = next;
            next = tmpI;
            // End Bug Fix

            return true;
        }

        next = last;
        x = 0.0f;

        return true;
    }

    return false;
}

template <typename T>
bool interpolate( T & value, const NiBtOgre::KeyGroup<T>& keyGroup, float time, int & last )
{
    std::vector<NiBtOgre::Key<T> > frames = keyGroup.keys;
    int next;
    float x;

    if ( timeIndex( time, frames, last, next, x ) ) {
        T v1 = frames[last].value;
        T v2 = frames[next].value;

        switch ( keyGroup.interpolation ) {

        case 2:
        {
            // Quadratic
            /*
                In general, for keyframe values v1 = 0, v2 = 1 it appears that
                setting v1's corresponding "Backward" value to 1 and v2's
                corresponding "Forward" to 1 results in a linear interpolation.
            */

            // Tangent 1
            T t1 = frames[last].backward;
            // Tangent 2
            T t2 = frames[next].forward;

            float x2 = x * x;
            float x3 = x2 * x;

            // Cubic Hermite spline
            //    x(t) = (2t^3 - 3t^2 + 1)P1  + (-2t^3 + 3t^2)P2 + (t^3 - 2t^2 + t)T1 + (t^3 - t^2)T2

            value = v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);

        }    return true;

        case 5:
            // Constant
            if ( x < 0.5 )
                value = v1;
            else
                value = v2;

            return true;
        default:
            value = v1 + ( v2 - v1 ) * x;
            return true;
        }
    }

    return false;
}

// FIXME: exact same code in TransformController::Value - make it part of a utility class?
Ogre::Quaternion interpQuatKey(const NiBtOgre::KeyGroup<Ogre::Quaternion>& keyGroup, uint32_t cycleType, float time)
{
    if (time <= keyGroup.indexMap.begin()->first)
        return keyGroup.keys[keyGroup.indexMap.begin()->second].value;

    std::map<float, int>::const_iterator nextIt = keyGroup.indexMap.lower_bound(time); // >=
    if (nextIt != keyGroup.indexMap.end())
    {
        std::map<float, int>::const_iterator lastIt = nextIt;
        --lastIt; // point to prev key

        float lastTime = lastIt->first;
        float nextTime = nextIt->first;
        float x = (time - lastTime) / (nextTime - lastTime);

        if (keyGroup.interpolation == 5)      // CONST
        {
            if ( x < 0.5f ) // step up at half way (or should it be step up at 1.0f instead?)
                return keyGroup.keys[lastIt->second].value;
            else
                return keyGroup.keys[nextIt->second].value;
        }
        else if (keyGroup.interpolation == 1) // LINEAR
        {
            const Ogre::Quaternion& v1 = keyGroup.keys[lastIt->second].value;
            const Ogre::Quaternion& v2 = keyGroup.keys[nextIt->second].value;

            Ogre::Quaternion v3 = v1;
            if (v1.Dot(v2) < 0)
                v3 = v1.Inverse();
            return Ogre::Quaternion::nlerp(x, v3, v2, true/*shortestPath*/);
            //return Ogre::Quaternion::Slerp(x, v3, v2); // TODO: test this
        }
        else if (keyGroup.interpolation == 3) // TBC
        {
            // FIXME: use linear for now
            const Ogre::Quaternion& v1 = keyGroup.keys[lastIt->second].value;
            const Ogre::Quaternion& v2 = keyGroup.keys[nextIt->second].value;

            return Ogre::Quaternion::nlerp(x, v1, v2);
        }
        else // quadratic interpolation not supported for QuatKey
            throw std::runtime_error("interpQuatKey - Unsupported interpolation type "
                    +std::to_string(keyGroup.interpolation)+" for Quaternion keys");
    }
    else // no key has >= time; i.e. time is greater than any of the keys' (TODO: should this happen?)
    {
        // FIXME: do we ever get here?
        if (cycleType == 2) // CYCLE_CLAMP
            return keyGroup.keys[keyGroup.indexMap.rbegin()->second].value;
        else
            return keyGroup.keys[keyGroup.indexMap.begin()->second].value; // wrap around
    }
}
} // anon namespace

NiBtOgre::NiTimeController::NiTimeController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    // GOG Creatures\Goblin\GoblinHead.NIF
    if (stream->nifVer() == 0x0a01006a && stream->userVer() == 10 && stream->userVer2() == 5)
        stream->skip(sizeof(std::int32_t)); // unknown integer

    stream->read(mNextControllerRef);
    stream->read(mFlags);
    stream->read(mFrequency);
    stream->read(mPhase);
    stream->read(mStartTime);
    stream->read(mStopTime);

    stream->read(mTargetRef);
}

// baseclass does nothing?
NiBtOgre::NiTimeControllerRef NiBtOgre::NiTimeController::build(Ogre::Mesh *mesh)
{
    //std::cerr << "controller not implemented: " << NiObject::mModel.blockType(NiObject::selfRef()) << std::endl;

    return mNextControllerRef;
}

// baseclass does nothing?
NiBtOgre::NiTimeControllerRef NiBtOgre::NiTimeController::build(std::multimap<float, std::string>& textKeys, std::vector<Ogre::Controller<float> >& controllers)
{
    //std::cerr << "controller not implemented: " << NiObject::mModel.blockType(NiObject::selfRef()) << std::endl;

    return mNextControllerRef;
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSFrustumFOVController::BSFrustumFOVController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    stream->read(mInterpolatorRef);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLagBoneController::BSLagBoneController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    stream->read(mLinearVelocity);
    stream->read(mLinearRotation);
    stream->read(mMaximumDistance);
}

void NiBtOgre::NiBSBoneLODController::NodeGroup::read(NiStream *stream, const NiModel& model, BuildData& data)
{
    stream->read(numNodes);

    nodeRefs.resize(numNodes);
    for (unsigned int i = 0; i < numNodes; ++i)
        stream->read(nodeRefs.at(i));
}

void NiBtOgre::NiBSBoneLODController::SkinShapeGroup::read(NiStream *stream, const NiModel& model, BuildData& data)
{
    std::int32_t rIndex = -1;
    stream->read(numLinkPairs);

    linkPairs.resize(numLinkPairs);
    for (unsigned int i = 0; i < numLinkPairs; ++i)
    {
        //stream->getPtr<NiGeometry>(linkPairs.at(i).shape, model.objects());
        rIndex = -1;
        stream->read(rIndex);
        linkPairs.at(i).shape = model.getRef<NiGeometry>(rIndex);
        stream->read(linkPairs.at(i).skinInstanceRef);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSBoneLODController::NiBSBoneLODController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    stream->skip(sizeof(std::uint32_t)); // Unknown Int 1
    std::uint32_t numNodeGroups;
    stream->read(numNodeGroups);
    std::uint32_t numNodeGroups2;
    stream->read(numNodeGroups2);

    mNodeGroups.resize(numNodeGroups);
    for (unsigned int i = 0; i < numNodeGroups; ++i)
        mNodeGroups[i].read(stream, model, data);

    //if (stream->nifVer() >= 0x04020200) // from 4.2.2.0
    //{
#if 0 // seems to be user version controlled
        std::uint32_t numShapeGroups;
        stream->read(numShapeGroups);
        mShapeGroups.resize(numShapeGroups);
        for (unsigned int i = 0; i < numShapeGroups; ++i)
            mShapeGroups[i].read(index, stream, model, data);

        std::uint32_t numShapeGroups2;
        stream->read(numShapeGroups2);
        mShapeGroups2.resize(numShapeGroups2);
        for (unsigned int i = 0; i < numShapeGroups2; ++i)
            stream->read(mShapeGroups2.at(i));
#endif
    //}
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiControllerManager::NiControllerManager(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    mCumulative = stream->getBool();

    std::uint32_t numControllerSequences;
    stream->read(numControllerSequences);
    mControllerSequences.resize(numControllerSequences);
    for (unsigned int i = 0; i < numControllerSequences; ++i)
        stream->read(mControllerSequences.at(i));

    stream->read(mObjectPaletteRef);
}

// Each NiControllerSequence is a "playable" animation. The animation in Ogre implementation
// maybe skeletal, vertex, SceneNode, etc.  If the name of the animation if "Idle" it is
// automatically played (and most likely to be CYCLE_LOOP).
//
// Each NiControllerSquence specifies the following animation attributes:
//
//   * target node name
//   * cycle type - loop, run once, reverse <---- ignore the ones in NiControllerManager
//   * start/stop time, frequency <-------------- ignore the ones in NiControllerManager
//   * string palette
//   * text keys (e.g. 'start', 'end'), etc
//
// FIXME: how to decide whether to read in the 'kf' files in the directory?
//
NiBtOgre::NiTimeControllerRef NiBtOgre::NiControllerManager::build(std::multimap<float, std::string>& textKeys,
        std::vector<Ogre::Controller<float> >& controllers)
{
    // object palette appears to be a lookup table to map the target name string to the block number
    // that NiSequenceController can use to get to the target objects
    const NiDefaultAVObjectPalette* objects = mModel.getRef<NiDefaultAVObjectPalette>(mObjectPaletteRef);

    //if (mModel.nifVer() >= 0x14020007) // FO3 onwards
    if (0)//mModel.getName().find("geardoor") != std::string::npos)
    {
        //if (mNextControllerRef != -1 && mModel.blockType(mNextControllerRef) == "NiMultiTargetTransformeController")
        //{
            //NiMultiTargetTransformController *controller
                //= mModel.getRef<NiMultiTargetTransformController>(mNextControllerRef);

            //return controller->build(mControllerSequences, *objects, controllers);

            // FIXME: shoud update a map in 'inst' so that the animation can be played
            // (? how to get the entity for mapping against the animation name?)
            for (std::uint32_t i = 0; i < mControllerSequences.size(); ++i)
                mModel.getRef<NiControllerSequence>(mControllerSequences[i])->buildFO3(*objects, textKeys, controllers);
        //}
        //else
            //throw std::runtime_error("NiControllerManager: NiMultiTargetTransformController not found");
    }
    else
    {
        for (std::uint32_t i = 0; i < mControllerSequences.size(); ++i)
            mModel.getRef<NiControllerSequence>(mControllerSequences[i])->build(objects);
    }

    return mNextControllerRef;
}

void NiBtOgre::NiControllerManager::getControllerSequenceMap(std::map<std::string, NiControllerSequence*>& result) const
{
    for (std::size_t i = 0; i < mControllerSequences.size(); ++i)
    {
        if (NiControllerSequence *sequence = mModel.getRef<NiControllerSequence>(mControllerSequences[i]))
        {
            std::string animName = mModel.indexToString(sequence->getNameIndex());
            result.insert(std::make_pair(animName, sequence));
        }
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiMultiTargetTransformController::NiMultiTargetTransformController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data), mData(data) // for accessing mSkeleton later
    , mExtraTargetsBuilt(false)
    , mControllerSequence(nullptr) // only set sometimes
{
    stream->read(mNumExtraTargets);
    mExtraTargetRefs.resize(mNumExtraTargets);
    bool hasValidExtraTarget = false; // FO3 MetTurnstile01.nif doesn't really have a node animation
    for (unsigned int i = 0; i < mNumExtraTargets; ++i)
    {
        stream->read(mExtraTargetRefs.at(i));
        // checking for NiNode children not possible since most are yet to be created
#if 1
        if (mExtraTargetRefs.at(i) != -1) // Furniture\FXspiderWebKitDoorSpecial.nif (TES5 BleakFallsBarrow)
        {
            data.addBoneTreeLeafIndex(mExtraTargetRefs[i]);
            data.addControllerTargetIndex(mExtraTargetRefs[i]); // FO3 Dungeons\Office\Doors\OffDoorSmL02.NIF
            hasValidExtraTarget = true;
        }
#else
        // FIXME: is there a better way than doing a string comparison each time?
        if (mExtraTargetRefs.at(i) != -1 && model.blockType(mExtraTargetRefs.at(i)) == "NiNode")
        {
            data.addBoneTreeLeafIndex(mExtraTargetRefs.at(i));

            NiNode *node = model.getRef<NiNode>(mExtraTargetRefs.at(i));
            const std::vector<NiAVObjectRef>& children = node->getNiNodeChildren();

            //if (mExtraTargetRefs.at(i) == 182)
                //std::cout << "stop" << std::endl;

            for (unsigned int j = 0; j < children.size(); ++j)
            {
                if (children[j] != -1 && model.blockType(children[j]) == "NiNode")
                    data.addBoneTreeLeafIndex(children[j]);
            }
        }
#endif
    }

    // FO3 Dungeons\Office\Doors\OffDoorMetalSmL01.NIF has only 1 valid ExtraTargetRef
    if (NiTimeController::mTargetRef != -1 && hasValidExtraTarget)
    {
        if (model.blockType(NiTimeController::mTargetRef) == "NiNode" ||
            model.blockType(NiTimeController::mTargetRef) == "BSFadeNode")
        {
            data.addBoneTreeLeafIndex(mTargetRef);
            data.addControllerTargetIndex(mTargetRef);
        }
    }
}

// create a Ogre::Controller with multiple targets for each animation (i.e. NiControllerSequence)
NiBtOgre::NiTimeControllerRef NiBtOgre::NiMultiTargetTransformController::build(const std::vector<NiControllerSequenceRef>& animRefs, const NiDefaultAVObjectPalette& objects, std::vector<Ogre::Controller<float> >& controllers)
{
    if ((NiTimeController::mFlags & 0x8) == 0)
        return mNextControllerRef; // not active

    // build the target list (can't build in the constructor since the objects don't exist yet)
    if (!mExtraTargetsBuilt)
    {
        for (size_t i = 0; i < mNumExtraTargets; ++i)
        {
            if (mExtraTargetRefs[i] != -1)
                mExtraTargets.push_back(mModel.getRef<NiAVObject>(mExtraTargetRefs[i]));
        }

        mExtraTargetsBuilt = true;
    }

    for (size_t i = 0; i < animRefs.size(); ++i)
    {
        //mModel.getRef<NiControllerSequence>(animRefs[i])->buildFO3(objects, mExtraTargets, controllers);
    }

    return mNextControllerRef;
}

// actual build
NiBtOgre::NiTimeControllerRef NiBtOgre::NiMultiTargetTransformController::build(std::multimap<float, std::string>& textKeys, std::vector<Ogre::Controller<float> >& controllers)
{
    if ((NiTimeController::mFlags & 0x8) == 0)
        return mNextControllerRef; // not active

    if (!mControllerSequence)
        return mNextControllerRef; // FIXME: must have implemented the required controller yet

    Ogre::SharedPtr<NiModel> model
        = NiBtOgre::NiModelManager::getSingleton().getByName(mModel.getName(), "General");

    Ogre::ControllerValueRealPtr srcval;
    Ogre::ControllerValueRealPtr dstval(OGRE_NEW MultiTargetTransformController::Value(model, mTargetInterpolators));
    Ogre::ControllerFunctionRealPtr func(OGRE_NEW MultiTargetTransformController::Function(mControllerSequence, false));

    controllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));

    return mNextControllerRef;
}

// called from NiControllerSequence::buildFO3
// register only, assume it will be built following NiContollerManager
void NiBtOgre::NiMultiTargetTransformController::registerTarget(const NiControllerSequence *sequence, const std::string& targetName, const NiTransformInterpolator *interpolator)
{
    mControllerSequence = sequence;

    Ogre::SkeletonPtr skeleton = mModel.getSkeleton();
    if (!skeleton || !skeleton->hasBone(targetName)) // FIXME: should not happen (not hasBone())
        return;

    Ogre::Bone *bone = skeleton->getBone(targetName);
    if (bone)
        mTargetInterpolators.push_back(std::make_pair(bone, interpolator));
}

// FIXME: below comment block is out of date

/*
 * **************************** DO NOT USE AS IS *******************************
 *
 * This method and NiBtOgre::NiControllerSequence::build() was designed to use
 * Ogre's node animation code.  However, they only work for simple animations
 * e.g. Oblivion's doors.  Complex animations that include child nodes without
 * controllers (some doors and activators in FO3/FONV and the activator for the
 * enterance to Benirus Manor's sealed portal).
 *
 * *****************************************************************************
 */

// node animation
// nameIndex is the name of NiControllerSequence, i.e. the animation name
void NiBtOgre::NiMultiTargetTransformController::build(int32_t nameIndex, const NiAVObject* target, NiTransformInterpolator *interpolator, float startTime, float stopTime)
{
    if ((NiTimeController::mFlags & 0x8) == 0) // not active
        return;

    // FIXME: TES5 clutter\ruins\ruins_smallchest.nif does not have a skeleton
    if (!mModel.hasSkeleton())
        return;

    // NOTE: assume that NiControllerManager and NiControllerSequence is built already
    //assert(mNiControllerSequeceBuilt == true); // throw instead?

    //if (mModel.indexToString(nameIndex) == "close") // FIXME: testing
        //return;

    // TES4 scripts may use lowercase string to identify animations (e.g. playgroup forward 1)
    std::string animationId = boost::to_lower_copy(mModel.indexToString(nameIndex));
    float totalAnimationLength = stopTime - startTime; // use the ones from the controller sequence

    // FIXME: animation should be created by NiSequenceController
    Ogre::Animation *animation;
    if (mModel.getSkeleton()->hasAnimation(animationId))
        animation = mModel.getSkeleton()->getAnimation(animationId);
    else
    {
        animation = mModel.getSkeleton()->createAnimation(animationId, totalAnimationLength);
        animation->setInterpolationMode(Ogre::Animation::IM_SPLINE);
    }

    // lookup bones
    std::string boneName = mModel.indexToString(target->getNameIndex());
    Ogre::Bone *bone = mModel.getSkeleton()->getBone(boneName);

    if (animation->hasNodeTrack(bone->getHandle()))
        return; // HACK: stop creation of the same object multiple times

    // use the interpolator block index and the track handle
    Ogre::NodeAnimationTrack* track = animation->createNodeTrack(bone->getHandle(), bone);

    NiTransformData *data = mModel.getRef<NiTransformData>(interpolator->mDataRef);

    // setup data for later
    mData.addAnimBoneName(animationId, boneName);

    // get all the unique time keys
    std::vector<float> timeKeys;

    // FIXME: handle other types
    if (data->mRotationType == 4) // XYZ
    {
        std::vector<Key<float> > frames = data->mXRotations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i)
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
        frames = data->mYRotations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i) // FIXME: repeated code
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
        frames = data->mZRotations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i) // FIXME: repeated code
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
    }
    else if (data->mRotationType == 1/*LINEAR*/ || data->mRotationType == 3/*TBC*/)
    {
        std::vector<Key<Ogre::Quaternion> > frames = data->mQuaternionKeys.keys;
        for (unsigned int i = 0; i < frames.size(); ++i)
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
        //std::cout << "quat keys " << boneName << " " << frames.size() << std::endl; // FIXME
    }

    if (data->mTranslations.keys.size() > 0)
    {
        std::vector<Key<Ogre::Vector3> > frames = data->mTranslations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i)
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
    }

    // FIXME: find the time keys for scale

    // from avakar's answer on https://stackoverflow.com/questions/2758080/how-to-sort-an-stl-vector
    std::sort(timeKeys.begin(), timeKeys.end(), [] (float const& a, float const& b) { return a < b; });

    for (unsigned int i = 0; i < timeKeys.size(); ++i)
    {
        float time = timeKeys[i];
        Ogre::TransformKeyFrame *kf = track->createNodeKeyFrame(time);

        if (data->mRotationType == 4) // XYZ
        {
            float value;
#if 0
            int last;
            if (!interpolate<float>(value, data->mXRotations, time, last)) value = 0.f;
            Ogre::Quaternion xr(Ogre::Radian(value), Ogre::Vector3::UNIT_X);
            if (!interpolate<float>(value, data->mYRotations, time, last)) value = 0.f;
            Ogre::Quaternion yr(Ogre::Radian(value), Ogre::Vector3::UNIT_Y);
            if (!interpolate<float>(value, data->mZRotations, time, last)) value = 0.f;
            Ogre::Quaternion zr(Ogre::Radian(value), Ogre::Vector3::UNIT_Z);
#else
            value = interpolateKey<float>(data->mXRotations, /*0,*/ time);
            Ogre::Quaternion xr(Ogre::Radian(value), Ogre::Vector3::UNIT_X);
            value = interpolateKey<float>(data->mYRotations, /*0,*/ time);
            Ogre::Quaternion yr(Ogre::Radian(value), Ogre::Vector3::UNIT_Y);
            value = interpolateKey<float>(data->mZRotations, /*0,*/ time);
            Ogre::Quaternion zr(Ogre::Radian(value), Ogre::Vector3::UNIT_Z);
#endif
            Ogre::Quaternion q(zr*yr*xr);

            // NOTE: I have no idea why the inverse is needed
            //
            // (COC "ImperialDungeon01") Dungeons\Chargen\IDGate01.NIF (0003665B)
            // (COW "AnvilWorld" -46 -8) Architecture\StoneWall\StoneWallGateDoor01.NIF (0002595F)
            // values are 0xff7fffff
            //if (interpolator->mRotation.w == -3.40282e+38 || interpolator->mRotation.x == -3.40282e+38 ||
            //    interpolator->mRotation.y == -3.40282e+38 || interpolator->mRotation.z == -3.40282e+38)
            if (interpolator->mRotation.x == -FLT_MAX)
                kf->setRotation(q);
            else
                kf->setRotation(interpolator->mRotation.Inverse() * q);
        }
        else if (data->mRotationType == 1/*LINEAR*/ || data->mRotationType == 3/*TBC*/)
        {
            if (!data->mQuaternionKeys.keys.empty())
            {
                Ogre::Quaternion q = interpQuatKey(data->mQuaternionKeys, 2/*CYCLE_CLAMP*/, time);

                if (interpolator->mRotation.x == -FLT_MAX)
                    kf->setRotation(q);
                else
                    kf->setRotation(interpolator->mRotation.Inverse() * q);
            }
        }
        else
            std::cout << "rotation type not supported " << boneName << " " << data->mRotationType << std::endl;

        //if (boneName == "VDoorDoor01") // FIXME: testing only
            //std::cout << interpolator->mTranslation.x << std::endl;

        if (data->mTranslations.keys.size() > 0)
        {
            Ogre::Vector3 value;
#if 0
            int last;
            if (!interpolate<Ogre::Vector3>(value, data->mTranslations, time, last)) value = Ogre::Vector3(0.f);
#else
            value = interpolateKey<Ogre::Vector3>(data->mTranslations, /*0,*/ time);
#endif

            // NOTE: wrong position & rotation fixed by taking away interpolator transform
            //
            // (COC "OblivionMqKvatchCitadelHall01" or COW "MS13CheydinhalOblivionWorld" 1 -1)
            // Oblivion\Architecture\Citadel\Interior\CitadelHall\CitadelHallDoor01Anim.NIF (000182E8)
            //
            // Some interpolator transforms may be invalid
            // mTranslation = {x=-3.40282347e+38 y=-3.40282347e+38 z=-3.40282347e+38 }
            if (interpolator->mTranslation.x == -FLT_MAX)
                kf->setTranslate(value);
            else
                kf->setTranslate(-interpolator->mTranslation + value);
        }

        // FIXME
        //kf->setScale();
    }
}

NiBtOgre::NiSingleInterpController::NiSingleInterpController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    if (stream->nifVer() >= 0x0a020000) // from 10.2.0.0
        stream->read(mInterpolatorRef);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiFloatExtraDataController::NiFloatExtraDataController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    if (stream->nifVer() >= 0x0a020000) // from 10.2.0.0
        stream->readLongString(mControllerDataRef);

    if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
    {
        unsigned char numExtraBytes;
        stream->read(numExtraBytes);
        stream->skip(sizeof(char)*7);
        for (unsigned int i = 0; i < numExtraBytes; ++i)
            stream->skip(sizeof(char));
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->read(mUnknownInt1);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->read(mTargetVariable);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->read(mTargetVariable);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->read(mTargetVariable);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTextureTransformController::NiTextureTransformController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->skip(sizeof(char)); // Unknown2
    stream->read(mTextureSlot);
    stream->read(mOperation);

#if 0 // commented out since this object is not seen in TES3
    if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream->read(mDataRef);
#endif
}

NiBtOgre::NiLightColorController::NiLightColorController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->read(mTargetColor);
}

NiBtOgre::NiPathController::NiPathController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
        stream->skip(sizeof(std::uint16_t)); // Unknown Short 2

    stream->skip(sizeof(std::uint32_t)); // Unknown Int 1
    stream->skip(sizeof(float)*2);       // Unknown Float 2, 3
    stream->skip(sizeof(std::uint16_t)); // Unknown Short

    stream->read(mPosDataRef);
    stream->read(mFloatDataRef);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBlendController::bhkBlendController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    stream->read(mUnknown);
}

// Seen in NIF ver 20.2.0.7 (FONV)
NiBtOgre::BSRefractionFirePeriodController::BSRefractionFirePeriodController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTimeController(index, stream, model, data)
{
    stream->read(mInterpolatorRef);
}
