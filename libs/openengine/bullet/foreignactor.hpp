#ifndef OENGINE_BULLET_FOREIGNACTOR_H
#define OENGINE_BULLET_FOREIGNACTOR_H

#include <string>

#include "physic.hpp"

namespace OEngine {
namespace Physic
{

    class ForeignActor: public PhysicActor
    {
    public:
        ForeignActor(const std::string& name, const std::string& model,
                const Ogre::Entity& skelBase, PhysicEngine *engine,
                const Ogre::Vector3& position, const Ogre::Quaternion& rotation, float scale);

        ~ForeignActor();

        virtual bool isForeign () const { return true; }

        virtual void setPosition(const Ogre::Vector3 &pos);
        virtual void setRotation (const Ogre::Quaternion& rotation);

        btCollisionObject *getRigidBody() const;

        bool enableRagdoll();

    private:

        ForeignActor(const ForeignActor&);
        ForeignActor& operator=(const ForeignActor&);

        OEngine::Physic::RigidBody* mForeignBody;
    };
}}

#endif
