#include "physicssystem.hpp"

#include <stdexcept>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <OgreViewport.h>
#include <OgreCamera.h>
#include <OgreTextureManager.h>
#include <OgreSceneNode.h>
#include <OgreSkeleton.h>
#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>

#include <extern/nibtogre/btrigidbodycimanager.hpp>

#include <openengine/bullet/trace.h>
#include <openengine/bullet/physic.hpp>
#include <openengine/bullet/BtOgrePG.h>
#include <openengine/bullet/BtOgreExtras.h>
#include <openengine/ogre/renderer.hpp>
#include <openengine/bullet/BulletShapeLoader.h>
#include <openengine/bullet/foreignactor.hpp>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/nifogre/skeleton.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <components/esm/loadgmst.hpp>

#include "../mwbase/world.hpp" // FIXME
#include "../mwbase/environment.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/animation.hpp"
#include "../mwrender/foreignnpcanimation.hpp"
#include "../mwrender/foreigncreatureanimation.hpp"


#include "ptr.hpp"
#include "class.hpp"

using namespace Ogre;

namespace
{

void animateCollisionShapes (std::map<OEngine::Physic::RigidBody*, OEngine::Physic::AnimatedShapeInstance>& map, btDynamicsWorld* dynamicsWorld)
{
    for (std::map<OEngine::Physic::RigidBody*, OEngine::Physic::AnimatedShapeInstance>::iterator it = map.begin();
         it != map.end(); ++it)
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->searchPtrViaHandle(it->first->mName);
        if (ptr.isEmpty()) // Shouldn't happen
            throw std::runtime_error("can't find Ptr");

        MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        if (!animation)
            continue;

        OEngine::Physic::AnimatedShapeInstance& instance = it->second;

        std::map<int, int>& shapes = instance.mAnimatedShapes;
        for (std::map<int, int>::iterator shapeIt = shapes.begin();
             shapeIt != shapes.end(); ++shapeIt)
        {

            const std::string& mesh = animation->getObjectRootName();
            int boneHandle = NifOgre::NIFSkeletonLoader::lookupOgreBoneHandle(mesh, shapeIt->first);
            Ogre::Node* bone = animation->getNode(boneHandle);

            if (bone == NULL)
                continue;

            btCompoundShape* compound = static_cast<btCompoundShape*>(instance.mCompound);

            btTransform trans;
            trans.setOrigin(BtOgre::Convert::toBullet(bone->_getDerivedPosition()) * compound->getLocalScaling());
            trans.setRotation(BtOgre::Convert::toBullet(bone->_getDerivedOrientation()));

            compound->getChildShape(shapeIt->second)->setLocalScaling(
                        compound->getLocalScaling() *
                        BtOgre::Convert::toBullet(bone->_getDerivedScale()));
            compound->updateChildTransform(shapeIt->second, trans);
        }

        // needed because we used btDynamicsWorld::setForceUpdateAllAabbs(false)
        dynamicsWorld->updateSingleAabb(it->first);
    }
}

}


namespace MWWorld
{

    static const float sMaxSlope = 49.0f;
    static const float sStepSizeUp = 34.0f;
    static const float sStepSizeDown = 62.0f;

    // Arbitrary number. To prevent infinite loops. They shouldn't happen but it's good to be prepared.
    static const int sMaxIterations = 8;

    class MovementSolver
    {
    private:
        static float getSlope(const Ogre::Vector3 &normal)
        {
            return normal.angleBetween(Ogre::Vector3(0.0f,0.0f,1.0f)).valueDegrees();
        }

        static bool stepMove(btCollisionObject *colobj, Ogre::Vector3 &position,
                             const Ogre::Vector3 &toMove, float &remainingTime,
                             OEngine::Physic::PhysicEngine *engine)
        {
            /*
             * Slide up an incline or set of stairs.  Should be called only after a
             * collision detection otherwise unnecessary tracing will be performed.
             *
             * NOTE: with a small change this method can be used to step over an obstacle
             * of height sStepSize.
             *
             * If successful return 'true' and update 'position' to the new possible
             * location and adjust 'remainingTime'.
             *
             * If not successful return 'false'.  May fail for these reasons:
             *    - can't move directly up from current position
             *    - having moved up by between epsilon() and sStepSize, can't move forward
             *    - having moved forward by between epsilon() and toMove,
             *        = moved down between 0 and just under sStepSize but slope was too steep, or
             *        = moved the full sStepSize down (FIXME: this could be a bug)
             *
             *
             *
             * Starting position.  Obstacle or stairs with height upto sStepSize in front.
             *
             *     +--+                          +--+       |XX
             *     |  | -------> toMove          |  |    +--+XX
             *     |  |                          |  |    |XXXXX
             *     |  | +--+                     |  | +--+XXXXX
             *     |  | |XX|                     |  | |XXXXXXXX
             *     +--+ +--+                     +--+ +--------
             *    ==============================================
             */

            /*
             * Try moving up sStepSize using stepper.
             * FIXME: does not work in case there is no front obstacle but there is one above
             *
             *     +--+                         +--+
             *     |  |                         |  |
             *     |  |                         |  |       |XX
             *     |  |                         |  |    +--+XX
             *     |  |                         |  |    |XXXXX
             *     +--+ +--+                    +--+ +--+XXXXX
             *          |XX|                         |XXXXXXXX
             *          +--+                         +--------
             *    ==============================================
             */
            OEngine::Physic::ActorTracer tracer, stepper;

            stepper.doTrace(colobj, position, position+Ogre::Vector3(0.0f,0.0f,sStepSizeUp), engine);
            if(stepper.mFraction < std::numeric_limits<float>::epsilon())
                return false; // didn't even move the smallest representable amount
                              // (TODO: shouldn't this be larger? Why bother with such a small amount?)

            /*
             * Try moving from the elevated position using tracer.
             *
             *                          +--+  +--+
             *                          |  |  |YY|   FIXME: collision with object YY
             *                          |  |  +--+
             *                          |  |
             *     <------------------->|  |
             *          +--+            +--+
             *          |XX|      the moved amount is toMove*tracer.mFraction
             *          +--+
             *    ==============================================
             */
            tracer.doTrace(colobj, stepper.mEndPos, stepper.mEndPos + toMove, engine);
            if(tracer.mFraction < std::numeric_limits<float>::epsilon())
                return false; // didn't even move the smallest representable amount

            /*
             * Try moving back down sStepSizeDown using stepper.
             * NOTE: if there is an obstacle below (e.g. stairs), we'll be "stepping up".
             * Below diagram is the case where we "stepped over" an obstacle in front.
             *
             *                                +--+
             *                                |YY|
             *                          +--+  +--+
             *                          |  |
             *                          |  |
             *          +--+            |  |
             *          |XX|            |  |
             *          +--+            +--+
             *    ==============================================
             */
            stepper.doTrace(colobj, tracer.mEndPos, tracer.mEndPos-Ogre::Vector3(0.0f,0.0f,sStepSizeDown), engine);
            if(stepper.mFraction < 1.0f && getSlope(stepper.mPlaneNormal) <= sMaxSlope)
            {
                // don't allow stepping up other actors
                if (stepper.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup == OEngine::Physic::CollisionType_Actor)
                    return false;
                // only step down onto semi-horizontal surfaces. don't step down onto the side of a house or a wall.
                // TODO: stepper.mPlaneNormal does not appear to be reliable - needs more testing
                // NOTE: caller's variables 'position' & 'remainingTime' are modified here
                position = stepper.mEndPos;
                remainingTime *= (1.0f-tracer.mFraction); // remaining time is proportional to remaining distance
                return true;
            }

            // moved between 0 and just under sStepSize distance but slope was too great,
            // or moved full sStepSize distance (FIXME: is this a bug?)
            return false;
        }


        ///Project a vector u on another vector v
        static inline Ogre::Vector3 project(const Ogre::Vector3 u, const Ogre::Vector3 &v)
        {
            return v * u.dotProduct(v);
        }

        ///Helper for computing the character sliding
        static inline Ogre::Vector3 slide(Ogre::Vector3 direction, const Ogre::Vector3 &planeNormal)
        {
            return direction - project(direction, planeNormal);
        }


    public:
        static Ogre::Vector3 traceDown(const MWWorld::Ptr &ptr, OEngine::Physic::PhysicEngine *engine, float maxHeight)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if (!physicActor)
                return position;

            OEngine::Physic::ActorTracer tracer;
            tracer.findGround(physicActor, position, position-Ogre::Vector3(0,0,maxHeight), engine);
            if(tracer.mFraction >= 1.0f)
            {
                physicActor->setOnGround(false);
                return position;
            }
            else
            {
                // Check if we actually found a valid spawn point (use an infinitely thin ray this time).
                // Required for some broken door destinations in Morrowind.esm, where the spawn point
                // intersects with other geometry if the actor's base is taken into account
                btVector3 from = BtOgre::Convert::toBullet(position);
                btVector3 to = from - btVector3(0,0,maxHeight);

                btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
                resultCallback1.m_collisionFilterGroup = 0xff;
                resultCallback1.m_collisionFilterMask = OEngine::Physic::CollisionType_World|OEngine::Physic::CollisionType_HeightMap;

                engine->mDynamicsWorld->rayTest(from, to, resultCallback1);
                if (resultCallback1.hasHit() &&
                        (BtOgre::Convert::toOgre(resultCallback1.m_hitPointWorld).distance(tracer.mEndPos) > 30
                        || getSlope(tracer.mPlaneNormal) > sMaxSlope))
                {
                    physicActor->setOnGround(getSlope(BtOgre::Convert::toOgre(resultCallback1.m_hitNormalWorld)) <= sMaxSlope);
                    return BtOgre::Convert::toOgre(resultCallback1.m_hitPointWorld) + Ogre::Vector3(0,0,1.f);
                }

                physicActor->setOnGround(getSlope(tracer.mPlaneNormal) <= sMaxSlope);

                return tracer.mEndPos;
            }
        }

        static Ogre::Vector3 move(const MWWorld::Ptr &ptr, const Ogre::Vector3 &movement, float time,
                                  bool isFlying, float waterlevel, float slowFall, OEngine::Physic::PhysicEngine *engine
                                  , std::map<std::string, std::string>& collisionTracker
                                  , std::map<std::string, std::string>& standingCollisionTracker)
        {
            const ESM::Position &refpos = ptr.getRefData().getPosition();
            Ogre::Vector3 position(refpos.pos);

            // Early-out for totally static creatures
            // (Not sure if gravity should still apply?)
            if (!ptr.getClass().isMobile(ptr))
                return position;

            OEngine::Physic::PhysicActor *physicActor = engine->getCharacter(ptr.getRefData().getHandle());
            if (!physicActor)
                return position;

            // Reset per-frame data
            physicActor->setWalkingOnWater(false);
            // Anything to collide with?
            if(!physicActor->getCollisionMode())
            {
                return position +  (Ogre::Quaternion(Ogre::Radian(refpos.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) *
                                    Ogre::Quaternion(Ogre::Radian(refpos.rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X))
                                * movement * time;
            }

            btCollisionObject *colobj = physicActor->getCollisionBody(); // FIXME: TES4
            Ogre::Vector3 halfExtents = physicActor->getHalfExtents();
            position.z += halfExtents.z;

            static const float fSwimHeightScale = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                    .find("fSwimHeightScale")->getFloat();
            float swimlevel = waterlevel + halfExtents.z - (halfExtents.z * 2 * fSwimHeightScale);

            OEngine::Physic::ActorTracer tracer;
            Ogre::Vector3 inertia = physicActor->getInertialForce();
            Ogre::Vector3 velocity;

            if(position.z < swimlevel || isFlying)
            {
                velocity = (Ogre::Quaternion(Ogre::Radian(refpos.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z)*
                            Ogre::Quaternion(Ogre::Radian(refpos.rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X)) * movement;
            }
            else
            {
                velocity = Ogre::Quaternion(Ogre::Radian(refpos.rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z) * movement;

                if (velocity.z > 0.f)
                    inertia = velocity;
                if(!physicActor->getOnGround())
                {
                    velocity = velocity + physicActor->getInertialForce();
                }
            }
            ptr.getClass().getMovementSettings(ptr).mPosition[2] = 0;

            // Now that we have the effective movement vector, apply wind forces to it
            if (MWBase::Environment::get().getWorld()->isInStorm())
            {
                Ogre::Vector3 stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
                Ogre::Degree angle = stormDirection.angleBetween(velocity);
                static const float fStromWalkMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("fStromWalkMult")->getFloat();
                velocity *= 1.f-(fStromWalkMult * (angle.valueDegrees()/180.f));
            }

            Ogre::Vector3 origVelocity = velocity;

            Ogre::Vector3 newPosition = position;
            /*
             * A loop to find newPosition using tracer, if successful different from the starting position.
             * nextpos is the local variable used to find potential newPosition, using velocity and remainingTime
             * The initial velocity was set earlier (see above).
             */
            float remainingTime = time;
            for(int iterations = 0; iterations < sMaxIterations && remainingTime > 0.01f; ++iterations)
            {
                Ogre::Vector3 nextpos = newPosition + velocity * remainingTime;

                // If not able to fly, don't allow to swim up into the air
                if(newPosition.z < swimlevel &&
                   !isFlying &&  // can't fly
                   nextpos.z > swimlevel &&     // but about to go above water
                   newPosition.z <= swimlevel)
                {
                    const Ogre::Vector3 down(0,0,-1);
                    Ogre::Real movelen = velocity.normalise();
                    Ogre::Vector3 reflectdir = velocity.reflect(down);
                    reflectdir.normalise();
                    velocity = slide(reflectdir, down)*movelen;
                    // NOTE: remainingTime is unchanged before the loop continues
                    continue; // velocity updated, calculate nextpos again
                }

                if(newPosition.squaredDistance(nextpos) > 0.0001)
                {
                    // trace to where character would go if there were no obstructions
                    tracer.doTrace(colobj, newPosition, nextpos, engine);

                    // check for obstructions
                    if(tracer.mFraction >= 1.0f)
                    {
                        newPosition = tracer.mEndPos; // ok to move, so set newPosition
                        break;
                    }
                    else
                    {
                        const btCollisionObject* standingOn = tracer.mHitObject;
                        if (const OEngine::Physic::RigidBody* body = dynamic_cast<const OEngine::Physic::RigidBody*>(standingOn))
                        {
                            collisionTracker[ptr.getRefData().getHandle()] = body->mName;
                        }
                    }
                }
                else
                {
                    // The current position and next position are nearly the same, so just exit.
                    // Note: Bullet can trigger an assert in debug modes if the positions
                    // are the same, since that causes it to attempt to normalize a zero
                    // length vector (which can also happen with nearly identical vectors, since
                    // precision can be lost due to any math Bullet does internally). Since we
                    // aren't performing any collision detection, we want to reject the next
                    // position, so that we don't slowly move inside another object.
                    break;
                }


                Ogre::Vector3 oldPosition = newPosition;
                // We hit something. Try to step up onto it. (NOTE: stepMove does not allow stepping over)
                // NOTE: stepMove modifies newPosition if successful
                bool result = stepMove(colobj, newPosition, velocity*remainingTime, remainingTime, engine);
                if (!result) // to make sure the maximum stepping distance isn't framerate-dependent or movement-speed dependent
                    result = stepMove(colobj, newPosition, velocity.normalisedCopy()*10.f, remainingTime, engine);
                if(result)
                {
                    // don't let pure water creatures move out of water after stepMove
                    if (ptr.getClass().isPureWaterCreature(ptr)
                            && newPosition.z + halfExtents.z > waterlevel)
                        newPosition = oldPosition;
                }
                else
                {
                    // Can't move this way, try to find another spot along the plane
                    Ogre::Vector3 direction = velocity;
                    Ogre::Real movelen = direction.normalise();
                    Ogre::Vector3 reflectdir = velocity.reflect(tracer.mPlaneNormal);
                    reflectdir.normalise();

                    Ogre::Vector3 newVelocity = slide(reflectdir, tracer.mPlaneNormal)*movelen;
                    if ((newVelocity-velocity).squaredLength() < 0.01)
                        break;
                    if (velocity.dotProduct(origVelocity) <= 0.f)
                        break;

                    velocity = newVelocity;

                    // Do not allow sliding upward if there is gravity. Stepping will have taken
                    // care of that.
                    if(!(newPosition.z < swimlevel || isFlying))
                        velocity.z = std::min(velocity.z, 0.0f);
                }
            }

            bool isOnGround = false;
            if (!(inertia.z > 0.f) && !(newPosition.z < swimlevel))
            {
                Ogre::Vector3 from = newPosition;
                Ogre::Vector3 to = newPosition - (physicActor->getOnGround() ?
                             Ogre::Vector3(0,0,sStepSizeDown+2.f) : Ogre::Vector3(0,0,2.f));
                tracer.doTrace(colobj, from, to, engine);
                if(tracer.mFraction < 1.0f && getSlope(tracer.mPlaneNormal) <= sMaxSlope
                        && tracer.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup != OEngine::Physic::CollisionType_Actor)
                {
                    const btCollisionObject* standingOn = tracer.mHitObject;
                    if (const OEngine::Physic::RigidBody* body = dynamic_cast<const OEngine::Physic::RigidBody*>(standingOn))
                    {
                        standingCollisionTracker[ptr.getRefData().getHandle()] = body->mName;
                    }
                    if (standingOn->getBroadphaseHandle()->m_collisionFilterGroup == OEngine::Physic::CollisionType_Water)
                        physicActor->setWalkingOnWater(true);

                    if (!isFlying)
                        newPosition.z = tracer.mEndPos.z + 1.0f;

                    isOnGround = true;
                }
                else
                {
                    // standing on actors is not allowed (see above).
                    // in addition to that, apply a sliding effect away from the center of the actor,
                    // so that we do not stay suspended in air indefinitely.
                    if (tracer.mFraction < 1.0f && tracer.mHitObject->getBroadphaseHandle()->m_collisionFilterGroup == OEngine::Physic::CollisionType_Actor)
                    {
                        if (Ogre::Vector3(velocity.x, velocity.y, 0).squaredLength() < 100.f*100.f)
                        {
                            btVector3 aabbMin, aabbMax;
                            tracer.mHitObject->getCollisionShape()->getAabb(tracer.mHitObject->getWorldTransform(), aabbMin, aabbMax);
                            btVector3 center = (aabbMin + aabbMax) / 2.f;
                            inertia = Ogre::Vector3(position.x - center.x(), position.y - center.y(), 0);
                            inertia.normalise();
                            inertia *= 100;
                        }
                    }

                    isOnGround = false;
                }
            }

            if(isOnGround || newPosition.z < swimlevel || isFlying)
                physicActor->setInertialForce(Ogre::Vector3(0.0f));
            else
            {
                inertia.z += time * -627.2f;
                if (inertia.z < 0)
                    inertia.z *= slowFall;
                physicActor->setInertialForce(inertia);
            }
            physicActor->setOnGround(isOnGround);

            newPosition.z -= halfExtents.z; // remove what was added at the beginning
            return newPosition;
        }
    };


    PhysicsSystem::PhysicsSystem(OEngine::Render::OgreRenderer &_rend) :
        mRender(_rend), mEngine(0), mTimeAccum(0.0f), mWaterHeight(0), mWaterEnabled(false)
    {
        NiBtOgre::BtRigidBodyCIManager *btLoader = new NiBtOgre::BtRigidBodyCIManager();
        // Create physics. shapeLoader is deleted by the physic engine
        NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
        mEngine = new OEngine::Physic::PhysicEngine(shapeLoader);
    }

    PhysicsSystem::~PhysicsSystem()
    {
        if (mWaterCollisionObject.get())
            mEngine->mDynamicsWorld->removeCollisionObject(mWaterCollisionObject.get());
        delete mEngine;
        delete OEngine::Physic::BulletShapeManager::getSingletonPtr();
        delete NiBtOgre::BtRigidBodyCIManager::getSingletonPtr();
    }

    OEngine::Physic::PhysicEngine* PhysicsSystem::getEngine()
    {
        return mEngine;
    }

    std::pair<float, std::string> PhysicsSystem::getFacedHandle(float queryDistance)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(0.5, 0.5);

        Ogre::Vector3 origin_ = ray.getOrigin();
        btVector3 origin(origin_.x, origin_.y, origin_.z);
        Ogre::Vector3 dir_ = ray.getDirection().normalisedCopy();
        btVector3 dir(dir_.x, dir_.y, dir_.z);

        btVector3 dest = origin + dir * queryDistance;
        std::pair <std::string, float> result = mEngine->rayTest(origin, dest);
        result.second *= queryDistance;

        return std::make_pair (result.second, result.first);
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedHandles (float queryDistance)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(0.5, 0.5);

        Ogre::Vector3 origin_ = ray.getOrigin();
        btVector3 origin(origin_.x, origin_.y, origin_.z);
        Ogre::Vector3 dir_ = ray.getDirection().normalisedCopy();
        btVector3 dir(dir_.x, dir_.y, dir_.z);

        btVector3 dest = origin + dir * queryDistance;
        std::vector < std::pair <float, std::string> > results;
        /* auto */ results = mEngine->rayTest2(origin, dest);
        std::vector < std::pair <float, std::string> >::iterator i;
        for (/* auto */ i = results.begin (); i != results.end (); ++i)
            i->first *= queryDistance;
        return results;
    }

    std::vector < std::pair <float, std::string> > PhysicsSystem::getFacedHandles (float mouseX, float mouseY, float queryDistance)
    {
        Ray ray = mRender.getCamera()->getCameraToViewportRay(mouseX, mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(queryDistance);

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::vector < std::pair <float, std::string> > results;
        /* auto */ results = mEngine->rayTest2(_from,_to);
        std::vector < std::pair <float, std::string> >::iterator i;
        for (/* auto */ i = results.begin (); i != results.end (); ++i)
            i->first *= queryDistance;
        return results;
    }

    std::pair<std::string,Ogre::Vector3> PhysicsSystem::getHitContact(const std::string &name,
                                                                      const Ogre::Vector3 &origin,
                                                                      const Ogre::Quaternion &orient,
                                                                      float queryDistance)
    {
        const MWWorld::Store<ESM::GameSetting> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        btConeShape shape(Ogre::Degree(store.find("fCombatAngleXY")->getFloat()/2.0f).valueRadians(),
                          queryDistance);
        shape.setLocalScaling(btVector3(1, 1, Ogre::Degree(store.find("fCombatAngleZ")->getFloat()/2.0f).valueRadians() /
                                              shape.getRadius()));

        // The shape origin is its center, so we have to move it forward by half the length. The
        // real origin will be provided to getFilteredContact to find the closest.
        Ogre::Vector3 center = origin + (orient * Ogre::Vector3(0.0f, queryDistance*0.5f, 0.0f));

        btCollisionObject object;
        object.setCollisionShape(&shape);
        object.setWorldTransform(btTransform(btQuaternion(orient.x, orient.y, orient.z, orient.w),
                                             btVector3(center.x, center.y, center.z)));

        std::pair<const OEngine::Physic::RigidBody*,btVector3> result = mEngine->getFilteredContact(
                name, btVector3(origin.x, origin.y, origin.z), &object);
        if(!result.first)
            return std::make_pair(std::string(), Ogre::Vector3(&result.second[0]));
        return std::make_pair(result.first->mName, Ogre::Vector3(&result.second[0]));
    }


    bool PhysicsSystem::castRay(const Vector3& from, const Vector3& to, bool raycastingObjectOnly,bool ignoreHeightMap)
    {
        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to, raycastingObjectOnly,ignoreHeightMap);
        return !(result.first == "");
    }

    std::pair<bool, Ogre::Vector3>
    PhysicsSystem::castRay(const Ogre::Vector3 &orig, const Ogre::Vector3 &dir, float len)
    {
        Ogre::Ray ray = Ogre::Ray(orig, dir);
        Ogre::Vector3 to = ray.getPoint(len);

        btVector3 btFrom = btVector3(orig.x, orig.y, orig.z);
        btVector3 btTo = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> test = mEngine->rayTest(btFrom, btTo);
        if (test.second == -1) {
            return std::make_pair(false, Ogre::Vector3());
        }
        return std::make_pair(true, ray.getPoint(len * test.second));
    }

    std::pair<bool, Ogre::Vector3> PhysicsSystem::castRay(float mouseX, float mouseY, Ogre::Vector3* normal, std::string* hit)
    {
        Ogre::Ray ray = mRender.getCamera()->getCameraToViewportRay(
            mouseX,
            mouseY);
        Ogre::Vector3 from = ray.getOrigin();
        Ogre::Vector3 to = ray.getPoint(200); /// \todo make this distance (ray length) configurable

        btVector3 _from, _to;
        _from = btVector3(from.x, from.y, from.z);
        _to = btVector3(to.x, to.y, to.z);

        std::pair<std::string, float> result = mEngine->rayTest(_from, _to, true, false, normal);

        if (result.first == "")
            return std::make_pair(false, Ogre::Vector3());
        else
        {
            if (hit != NULL)
                *hit = result.first;
            return std::make_pair(true, ray.getPoint(200*result.second));  /// \todo make this distance (ray length) configurable
        }
    }

    std::vector<std::string> PhysicsSystem::getCollisions(const Ptr &ptr, int collisionGroup, int collisionMask)
    {
        return mEngine->getCollisions(ptr.getRefData().getBaseNode()->getName(), collisionGroup, collisionMask);
    }

    Ogre::Vector3 PhysicsSystem::traceDown(const MWWorld::Ptr &ptr, float maxHeight)
    {
        return MovementSolver::traceDown(ptr, mEngine, maxHeight);
    }

    void PhysicsSystem::addHeightField (const float* heights,
                int x, int y, float yoffset,
                float triSize, float sqrtVerts)
    {
        mEngine->addHeightField(heights, x, y, yoffset, triSize, sqrtVerts);
    }

    void PhysicsSystem::removeHeightField (int x, int y)
    {
        mEngine->removeHeightField(x, y);
    }

    void PhysicsSystem::addObject (const Ptr& ptr, const std::string& mesh, bool placeable)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        handleToMesh[node->getName()] = mesh; // FIXME: ever growing...

        // FIXME: this part is a horrid hack
        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(ptr);
        MWRender::ObjectAnimation *objAnim = dynamic_cast<MWRender::ObjectAnimation*>(anim);
        if (objAnim && !objAnim->getPhysicsNodeMap().empty())  // FIXME: this is such a bad hack
        {
            OEngine::Physic::RigidBody * body = mEngine->createAndAdjustRagdollBody(
                mesh, node->getName(), objAnim->getPhysicsNodeMap(), *objAnim->getSkelBase(),
                ptr.getCellRef().getScale(), node->getPosition(), node->getOrientation(),
                0, 0, false, placeable);

#if 0
            //if (objAnim->disableHavokAtStart())
            {
                //body->setLinearFactor(btVector3(0, 0, 0));
                //body->setAngularFactor(btVector3(0, 0, 0));
                // FIXME: save getInvInertiaDiagLocal() and getInvMass() for later?
                //body->setMassProps(0.0, btVector3(0.f, 0.f, 0.f)); // this does not work
                static_cast<BtOgre::RigidBodyState*>(body->getMotionState())->mEnabled = false;

                std::multimap<std::string, OEngine::Physic::RigidBody*>::iterator it;
                for (it = body->mChildren.begin(); it != body->mChildren.end(); ++it)
                {
                    //it->second->setLinearFactor(btVector3(0, 0, 0));
                    //it->second->setAngularFactor(btVector3(0, 0, 0));
                    //it->second->setMassProps(0.0, btVector3(0.f, 0.f, 0.f));
                    static_cast<BtOgre::RigidBodyState*>(it->second->getMotionState())->mEnabled = false;
                }
            }
#endif
            // FIXME: really want to get rid of doing this twice
            //mEngine->createAndAdjustRagdollBody(
                //mesh, node->getName(), objAnim->getPhysicsNodeMap(), ptr.getCellRef().getScale(),
                //node->getPosition(), node->getOrientation(), 0, 0, true/*raycasting*/, placeable);

            return;
        }

        mEngine->createAndAdjustRigidBody(mesh, node->getName(), ptr.getCellRef().getScale(),
                node->getPosition(), node->getOrientation(), 0, 0, false, placeable);
        mEngine->createAndAdjustRigidBody(mesh, node->getName(), ptr.getCellRef().getScale(),
                node->getPosition(), node->getOrientation(), 0, 0, true/*raycasting*/, placeable);
    }

    void PhysicsSystem::addActor (const Ptr& ptr, const std::string& mesh)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        //TODO:optimize this. Searching the std::map isn't very efficient i think.
        mEngine->addCharacter(node->getName(), mesh, node->getPosition(), node->getScale().x, node->getOrientation());
    }

    void PhysicsSystem::addForeignActor (const Ptr& ptr, const std::string& model, const Ogre::Entity& skelBase)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();

        // FIXME: we're gonna need different parameters
        mEngine->addForeignCharacter(node->getName(), model, skelBase,
            node->getPosition(), node->getScale().x, node->getOrientation());
    }

    void PhysicsSystem::removeObject (const std::string& handle)
    {
        mEngine->removeCharacter(handle);
        mEngine->removeRigidBody(handle);
        mEngine->deleteRigidBody(handle);
    }

    void PhysicsSystem::moveObject (const Ptr& ptr)
    {
        Ogre::SceneNode *node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();
        const Ogre::Vector3 &position = node->getPosition();

        if(OEngine::Physic::RigidBody *body = mEngine->getRigidBody(handle))
        {
            if (!body->mIsForeign)
            {
                body->getWorldTransform().setOrigin(btVector3(position.x,position.y,position.z));
            }
            else
            {
                if (body->getCollisionShape()->getUserIndex() == 4)
                {
                    body->getWorldTransform().setOrigin(btVector3(position.x,position.y,position.z));
                }
                else
                {
                    Ogre::Vector3 pos = body->mLocalTransform * position;
                    body->getWorldTransform().setOrigin(btVector3(pos.x,pos.y,pos.z));

                    std::multimap<std::string, OEngine::Physic::RigidBody*>::iterator it;
                    for (it = body->mChildren.begin(); it != body->mChildren.end(); ++it)
                    {
                        pos = it->second->mLocalTransform * position;
                        it->second->getWorldTransform().setOrigin(btVector3(pos.x,pos.y,pos.z));

                        mEngine->mDynamicsWorld->updateSingleAabb(it->second);
                    }
                }
            }

            mEngine->mDynamicsWorld->updateSingleAabb(body);
        }

        if(OEngine::Physic::RigidBody *body = mEngine->getRigidBody(handle, true))
        {
            if (!body->mIsForeign)
            {
                body->getWorldTransform().setOrigin(btVector3(position.x,position.y,position.z));
            }
            else
            {
                if (body->getCollisionShape()->getUserIndex() == 4)
                {
                    body->getWorldTransform().setOrigin(btVector3(position.x,position.y,position.z));
                }
                else
                {
                    Ogre::Vector3 pos = body->mLocalTransform * position;
                    body->getWorldTransform().setOrigin(btVector3(pos.x,pos.y,pos.z));

                    std::multimap<std::string, OEngine::Physic::RigidBody*>::iterator it;
                    for (it = body->mChildren.begin(); it != body->mChildren.end(); ++it)
                    {
                        pos = it->second->mLocalTransform * position;
                        it->second->getWorldTransform().setOrigin(btVector3(pos.x,pos.y,pos.z));

                        mEngine->mDynamicsWorld->updateSingleAabb(it->second);
                    }
                }
            }

            mEngine->mDynamicsWorld->updateSingleAabb(body);
        }

        // Actors update their AABBs every frame (DISABLE_DEACTIVATION), so no need to do it manually
        if(OEngine::Physic::PhysicActor *physact = mEngine->getCharacter(handle))
        {
            physact->setPosition(position);

            // HACK: piggy back on setPosition() to update collision shapes
            if (physact->isForeign()
                && !ptr.getClass().getCreatureStats(ptr).isDead()) // FIXME: testing ragdoll
                updateForeignActor(ptr, physact);
        }
    }

    bool PhysicsSystem::enableRagdoll (const MWWorld::Ptr& actor)
    {
        Ogre::SceneNode *node = actor.getRefData().getBaseNode();
        const std::string& handle = node->getName();

        if(OEngine::Physic::PhysicActor *physact = mEngine->getCharacter(handle))
        {
            OEngine::Physic::ForeignActor *foreignActor = static_cast<OEngine::Physic::ForeignActor*>(physact);

            // FIXME: need to remember the current bone positions and if possible any
            // forces/momentum vector
            return foreignActor->enableRagdoll();
        }

        return false;
    }

    void PhysicsSystem::rotateObject (const Ptr& ptr)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();
        const Ogre::Quaternion &rotation = node->getOrientation();

        // TODO: map to MWWorld::Ptr for faster access
        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
        {
            act->setRotation(rotation);
        }
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            if (!body->mIsForeign)
            {
                if(body->getCollisionShape()->getName() != "Box")
                    body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                else
                    mEngine->boxAdjustExternal(handleToMesh[handle], body, node->getScale().x, node->getPosition(), rotation);
            }
            else
            {
                Ogre::Quaternion rot;
                if (body->getCollisionShape()->getUserIndex() == 4)
                    body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                else
                {
                    rot = body->mLocalTransform.extractQuaternion();
                    rot = rotation * rot;
                    body->getWorldTransform().setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
                }

                std::multimap<std::string, OEngine::Physic::RigidBody*>::iterator it;
                for (it = body->mChildren.begin(); it != body->mChildren.end(); ++it)
                {
                    if (it->second->getCollisionShape()->getUserIndex() == 4)
                        it->second->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                    else
                    {
                        rot = it->second->mLocalTransform.extractQuaternion();
                        rot = rotation * rot;
                        it->second->getWorldTransform().setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));
                    }

                    mEngine->mDynamicsWorld->updateSingleAabb(it->second);
                }
            }

            mEngine->mDynamicsWorld->updateSingleAabb(body);
        }
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle, true/*raycasting*/))
        {
            if (!body->mIsForeign)
            {
                if(body->getCollisionShape()->getName() != "Box")
                    body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                else
                    mEngine->boxAdjustExternal(handleToMesh[handle], body, node->getScale().x, node->getPosition(), rotation);
            }
            else
            {
                Ogre::Quaternion rot = body->mLocalTransform.extractQuaternion();
                rot = rotation * rot;
                if (body->getCollisionShape()->getUserIndex() == 4)
                    body->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                else
                    body->getWorldTransform().setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

                std::multimap<std::string, OEngine::Physic::RigidBody*>::iterator it;
                for (it = body->mChildren.begin(); it != body->mChildren.end(); ++it)
                {
                    rot = it->second->mLocalTransform.extractQuaternion();
                    rot = rotation * rot;
                    if (it->second->getCollisionShape()->getUserIndex() == 4)
                        it->second->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
                    else
                        it->second->getWorldTransform().setRotation(btQuaternion(rot.x, rot.y, rot.z, rot.w));

                    mEngine->mDynamicsWorld->updateSingleAabb(it->second);
                }
            }

            mEngine->mDynamicsWorld->updateSingleAabb(body);
        }
    }

    // doors and activators
    void PhysicsSystem::moveSubObject (const MWWorld::Ptr& ptr, const std::string& boneName, const Ogre::Vector3& position, const Ogre::Quaternion& rotation)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();

        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            // move parent
            if (body->mTargetName == boneName &&
                body->getCollisionShape()->getUserIndex() != 4) // ignore static
            {
                // rotation is the local rotation of the bone
                // position is the local translation of the bone
                // body->mBindingOrientation is the world orientation of the body
                // body->mBindingPosition is the world position of the body
                btQuaternion qb = body->mBindingOrientation;
                Ogre::Quaternion q(qb.w(), qb.x(), qb.y(), qb.z());
                q = q * rotation;
                Ogre::Vector3 pos = q * position;

                body->getWorldTransform().setOrigin(
                    btVector3(pos.x, pos.y, pos.z) + body->mBindingPosition);

                mEngine->mDynamicsWorld->updateSingleAabb(body);
            }

            // move children; there may be more than one rigidbody moved by the same bone
            std::multimap<std::string, OEngine::Physic::RigidBody*>::const_iterator iter
                = body->mChildren.begin();
            for (; iter != body->mChildren.end(); ++iter)
            {
                if (iter->first == boneName &&
                    iter->second->getCollisionShape()->getUserIndex() != 4) // ignore static
                {
                    btQuaternion qb = iter->second->mBindingOrientation;
                    Ogre::Quaternion q(qb.w(), qb.x(), qb.y(), qb.z());
                    q = q * rotation;
                    Ogre::Vector3 pos = q * position;

                    iter->second->getWorldTransform().setOrigin(
                        btVector3(pos.x, pos.y, pos.z) + iter->second->mBindingPosition);

                    mEngine->mDynamicsWorld->updateSingleAabb(iter->second);
                }
            }
        }

        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle, true/*raycasting*/))
        {
            // move parent
            if (body->mTargetName == boneName &&
                body->getCollisionShape()->getUserIndex() != 4) // ignore static
            {
                btQuaternion qb = body->mBindingOrientation;
                Ogre::Quaternion q(qb.w(), qb.x(), qb.y(), qb.z());
                q = q * rotation;
                Ogre::Vector3 pos = q * position;

                body->getWorldTransform().setOrigin(
                    btVector3(pos.x, pos.y, pos.z) + body->mBindingPosition);

                mEngine->mDynamicsWorld->updateSingleAabb(body);
            }

            // move children; there may be more than one rigidbody moved by the same bone
            std::multimap<std::string, OEngine::Physic::RigidBody*>::const_iterator iter
                = body->mChildren.begin();
            for (; iter != body->mChildren.end(); ++iter)
            {
                if (iter->first == boneName &&
                    iter->second->getCollisionShape()->getUserIndex() != 4) // ignore static
                {
                    btQuaternion qb = iter->second->mBindingOrientation;
                    Ogre::Quaternion q(qb.w(), qb.x(), qb.y(), qb.z());
                    q = q * rotation;
                    Ogre::Vector3 pos = q * position;

                    iter->second->getWorldTransform().setOrigin(
                        btVector3(pos.x, pos.y, pos.z) + iter->second->mBindingPosition);

                    mEngine->mDynamicsWorld->updateSingleAabb(iter->second);
                }
            }
        }
    }

    // for doors and activators
    void PhysicsSystem::rotateSubObject (const MWWorld::Ptr& ptr, const std::string& boneName, const Ogre::Quaternion& rotation)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();

        // Dungeons\Sewers\sewerTunnelDoor01.NIF has static parent but its child "Gate" is not
        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle))
        {
            // rotate parent
            if (body->mTargetName == boneName &&
                body->getCollisionShape()->getUserIndex() != 4) // ignore static
            {
                body-> getWorldTransform().setRotation(
                    body->mBindingOrientation
                    *
                    btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w)
                    );

                mEngine->mDynamicsWorld->updateSingleAabb(body);
            }

            // rotate children; there may be more than one rigidbody rotated by the same bone
            std::multimap<std::string, OEngine::Physic::RigidBody*>::const_iterator iter
                = body->mChildren.begin();
            for (; iter != body->mChildren.end(); ++iter)
            {
                if (iter->first == boneName &&
                    iter->second->getCollisionShape()->getUserIndex() != 4) // ignore static
                {
                    // FIXME: BenirusDoor01.NIF has 3 collision shapes that won't rotate
                    //        properly.  Don't know how to fix it, so just work around for now.
                    if (boneName == "gear 12" || boneName == "gear 13" || boneName == "gear 16")
                    {
                        iter->second->getWorldTransform().setRotation(
                            btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w)
                            *
                            iter->second->mBindingOrientation
                            );
                    }
                    else
                    {
                        iter->second->getWorldTransform().setRotation(
                            iter->second->mBindingOrientation
                            *
                            btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w)
                            );
                    }

                    mEngine->mDynamicsWorld->updateSingleAabb(iter->second);
                }
            }
        }

        if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle, true/*raycasting*/))
        {
            // rotate parent
            if (body->mTargetName == boneName &&
                body->getCollisionShape()->getUserIndex() != 4) // ignore static
            {
                body-> getWorldTransform().setRotation(
                    body->mBindingOrientation
                    *
                    btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w)
                    );

                mEngine->mDynamicsWorld->updateSingleAabb(body);
            }

            // rotate children; there may be more than one rigidbody rotated by the same bone
            std::multimap<std::string, OEngine::Physic::RigidBody*>::const_iterator iter
                = body->mChildren.begin();

            for (; iter != body->mChildren.end(); ++iter)
            {
                if (iter->first == boneName &&
                    iter->second->getCollisionShape()->getUserIndex() != 4) // ignore static
                {
                    // FIXME: BenirusDoor01.NIF has 3 collision shapes that won't rotate
                    //        properly.  Don't know how to fix it, so just work around for now.
                    if (boneName == "gear 12" || boneName == "gear 13" || boneName == "gear16")
                    {
                        iter->second->getWorldTransform().setRotation(
                            btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w)
                            *
                            iter->second->mBindingOrientation
                            );
                    }
                    else
                    {
                        iter->second->getWorldTransform().setRotation(
                            iter->second->mBindingOrientation
                            *
                            btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w)
                            );
                    }

                    mEngine->mDynamicsWorld->updateSingleAabb(iter->second);
                }
            }
        }
    }

    // update the NPC skeleton's rigid bodies according to the bone transforms of the current animation
    void PhysicsSystem::updateForeignActor (const MWWorld::Ptr& ptr, OEngine::Physic::PhysicActor *physact)
    {
        OEngine::Physic::ForeignActor *actor = static_cast<OEngine::Physic::ForeignActor*>(physact);
        OEngine::Physic::RigidBody *body
                = static_cast<OEngine::Physic::RigidBody*>(actor->getRigidBody());
        if (!body)
            return;

        if (MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(ptr))
        {
            Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
            Ogre::Matrix4 sceneNodeTrans;
            sceneNodeTrans.makeTransform(node->getPosition(), node->getScale(), node->getOrientation());

            Ogre::Entity *skelBase = anim->getSkelBase();
            Ogre::SkeletonInstance* skelInst = skelBase->getSkeleton();

            const Ogre::Skeleton::BoneList& bones = skelInst->getBones();
            for (std::size_t i = 0; i < bones.size(); ++i)
            {

                std::string boneName = bones[i]->getName();

                Ogre::Matrix4 boneTrans;
                boneTrans.makeTransform(bones[i]->_getDerivedPosition(),
                    node->getScale(), bones[i]->_getDerivedOrientation()); // NOTE: using node scales

                Ogre::Matrix4 m = sceneNodeTrans * boneTrans;
                Ogre::Vector3 p = m.getTrans();
                Ogre::Quaternion q = m.extractQuaternion();
                btTransform bt(btQuaternion(q.x, q.y, q.z, q.w), btVector3(p.x, p.y, p.z));

                if (body->mTargetName == boneName) // parent
                {
                    body->setWorldTransform(bt);
                    mEngine->mDynamicsWorld->updateSingleAabb(body);
                }
                else // children
                {
                    // here we make an assumption that for skeleton's rigid bodies one bone
                    // moves only one rigid body (unlike objects)
                    std::multimap<std::string, OEngine::Physic::RigidBody*>::const_iterator cit
                        = body->mChildren.find(boneName);
                    if (cit != body->mChildren.end())
                    {
                        cit->second->setWorldTransform(bt);
                        mEngine->mDynamicsWorld->updateSingleAabb(body);
                    }
                }
            }
        }
    }

    // FIXME: local transform
    void PhysicsSystem::scaleObject (const Ptr& ptr)
    {
        Ogre::SceneNode* node = ptr.getRefData().getBaseNode();
        const std::string &handle = node->getName();
        if(handleToMesh.find(handle) != handleToMesh.end())
        {
            std::string model = ptr.getClass().getModel(ptr);
            model = Misc::ResourceHelpers::correctActorModelPath(model); // FIXME: scaling shouldn't require model

            bool placeable = false;
            if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle,true/*raycasting*/))
                placeable = body->mPlaceable;
            else if (OEngine::Physic::RigidBody* body = mEngine->getRigidBody(handle,false))
                placeable = body->mPlaceable;
            removeObject(handle);
            addObject(ptr, model, placeable);
        }

        if (OEngine::Physic::PhysicActor* act = mEngine->getCharacter(handle))
        {
            float scale = ptr.getCellRef().getScale();
            if (!ptr.getClass().isNpc())
                // NOTE: Ignoring Npc::adjustScale (race height) on purpose. This is a bug in MW and must be replicated for compatibility reasons
                ptr.getClass().adjustScale(ptr, scale);
            act->setScale(scale);
        }
    }

    bool PhysicsSystem::toggleCollisionMode()
    {
        for(std::map<std::string,OEngine::Physic::PhysicActor*>::iterator it = mEngine->mActorMap.begin(); it != mEngine->mActorMap.end();++it)
        {
            if (it->first=="player")
            {
                OEngine::Physic::PhysicActor* act = it->second;

                bool cmode = act->getCollisionMode();
                if(cmode)
                {
                    act->enableCollisionMode(false);
                    return false;
                }
                else
                {
                    act->enableCollisionMode(true);
                    return true;
                }
            }
        }

        throw std::logic_error ("can't find player");
    }

    bool PhysicsSystem::getObjectAABB(const MWWorld::Ptr &ptr, Ogre::Vector3 &min, Ogre::Vector3 &max)
    {
        std::string model = ptr.getClass().getModel(ptr);
        model = Misc::ResourceHelpers::correctActorModelPath(model);
        if (model.empty()) {
            return false;
        }
        btVector3 btMin, btMax;
        float scale = ptr.getCellRef().getScale();
        mEngine->getObjectAABB(model, scale, btMin, btMax);

        min.x = btMin.x();
        min.y = btMin.y();
        min.z = btMin.z();

        max.x = btMax.x();
        max.y = btMax.y();
        max.z = btMax.z();

        return true;
    }


    void PhysicsSystem::queueObjectMovement(const Ptr &ptr, const Ogre::Vector3 &movement)
    {
        PtrVelocityList::iterator iter = mMovementQueue.begin();
        for(;iter != mMovementQueue.end();++iter)
        {
            if(iter->first == ptr)
            {
                iter->second = movement;
                return;
            }
        }

        mMovementQueue.push_back(std::make_pair(ptr, movement));
    }

    void PhysicsSystem::clearQueuedMovement()
    {
        mMovementQueue.clear();
        mCollisions.clear();
        mStandingCollisions.clear();
    }

    const PtrVelocityList& PhysicsSystem::applyQueuedMovement(float dt)
    {
        mMovementResults.clear();

        mTimeAccum += dt;
        if(mTimeAccum >= 1.0f/60.0f)
        {
            // Collision events should be available on every frame
            mCollisions.clear();
            mStandingCollisions.clear();

            const MWBase::World *world = MWBase::Environment::get().getWorld();
            PtrVelocityList::iterator iter = mMovementQueue.begin();
            for(;iter != mMovementQueue.end();++iter)
            {
                float waterlevel = -std::numeric_limits<float>::max();
                const MWWorld::CellStore *cell = iter->first.getCell();
                if(cell->getCell()->hasWater())
                    waterlevel = cell->getWaterLevel();

                float oldHeight = iter->first.getRefData().getPosition().pos[2];

                const MWMechanics::MagicEffects& effects = iter->first.getClass().getCreatureStats(iter->first).getMagicEffects();

                bool waterCollision = false;
                if (effects.get(ESM::MagicEffect::WaterWalking).getMagnitude()
                        && cell->getCell()->hasWater()
                        && !world->isUnderwater(iter->first.getCell(),
                                               Ogre::Vector3(iter->first.getRefData().getPosition().pos)))
                    waterCollision = true;

                OEngine::Physic::PhysicActor *physicActor = mEngine->getCharacter(iter->first.getRefData().getHandle());
                if (!physicActor) // actor was already removed from the scene
                    continue;
                physicActor->setCanWaterWalk(waterCollision);

                // Slow fall reduces fall speed by a factor of (effect magnitude / 200)
                float slowFall = 1.f - std::max(0.f, std::min(1.f, effects.get(ESM::MagicEffect::SlowFall).getMagnitude() * 0.005f));

                Ogre::Vector3 newpos = MovementSolver::move(iter->first, iter->second, mTimeAccum,
                                                            world->isFlying(iter->first),
                                                            waterlevel, slowFall, mEngine, mCollisions, mStandingCollisions);

                float heightDiff = newpos.z - oldHeight;

                if (heightDiff < 0)
                    iter->first.getClass().getCreatureStats(iter->first).addToFallHeight(-heightDiff);

                mMovementResults.push_back(std::make_pair(iter->first, newpos));
            }

            mTimeAccum = 0.0f;
        }
        mMovementQueue.clear();

        return mMovementResults;
    }

    void PhysicsSystem::stepSimulation(float dt)
    {
        animateCollisionShapes(mEngine->mAnimatedShapes, mEngine->mDynamicsWorld);
        animateCollisionShapes(mEngine->mAnimatedRaycastingShapes, mEngine->mDynamicsWorld);

        mEngine->stepSimulation(dt);
    }

    bool PhysicsSystem::isActorStandingOn(const Ptr &actor, const Ptr &object) const
    {
        const std::string& actorHandle = actor.getRefData().getHandle();
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mStandingCollisions.begin();
             it != mStandingCollisions.end(); ++it)
        {
            if (it->first == actorHandle && it->second == objectHandle)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsStandingOn(const Ptr &object, std::vector<std::string> &out) const
    {
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mStandingCollisions.begin();
             it != mStandingCollisions.end(); ++it)
        {
            if (it->second == objectHandle)
                out.push_back(it->first);
        }
    }

    bool PhysicsSystem::isActorCollidingWith(const Ptr &actor, const Ptr &object) const
    {
        const std::string& actorHandle = actor.getRefData().getHandle();
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mCollisions.begin();
             it != mCollisions.end(); ++it)
        {
            if (it->first == actorHandle && it->second == objectHandle)
                return true;
        }
        return false;
    }

    void PhysicsSystem::getActorsCollidingWith(const Ptr &object, std::vector<std::string> &out) const
    {
        const std::string& objectHandle = object.getRefData().getHandle();

        for (std::map<std::string, std::string>::const_iterator it = mCollisions.begin();
             it != mCollisions.end(); ++it)
        {
            if (it->second == objectHandle)
                out.push_back(it->first);
        }
    }

    void PhysicsSystem::disableWater()
    {
        if (mWaterEnabled)
        {
            mWaterEnabled = false;
            updateWater();
        }
    }

    void PhysicsSystem::enableWater(float height)
    {
        if (!mWaterEnabled || mWaterHeight != height)
        {
            mWaterEnabled = true;
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::setWaterHeight(float height)
    {
        if (mWaterHeight != height)
        {
            mWaterHeight = height;
            updateWater();
        }
    }

    void PhysicsSystem::updateWater()
    {
        if (mWaterCollisionObject.get())
        {
            mEngine->mDynamicsWorld->removeCollisionObject(mWaterCollisionObject.get());
        }

        if (!mWaterEnabled)
            return;

        mWaterCollisionObject.reset(new btCollisionObject());
        mWaterCollisionShape.reset(new btStaticPlaneShape(btVector3(0,0,1), mWaterHeight));
        mWaterCollisionObject->setCollisionShape(mWaterCollisionShape.get());
        mEngine->mDynamicsWorld->addCollisionObject(mWaterCollisionObject.get(), OEngine::Physic::CollisionType_Water,
                                                    OEngine::Physic::CollisionType_Actor);
    }
}
