#include "foreignobject.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>

#include "../../model/world/data.hpp"
#include "../../model/world/ref.hpp"
#include "../../model/world/refidcollection.hpp"

#include "../world/physicssystem.hpp"

#include "elements.hpp"

void CSVRender::ForeignObject::clearSceneNode (Ogre::SceneNode *node)
{
    for (Ogre::SceneNode::ObjectIterator iter = node->getAttachedObjectIterator();
        iter.hasMoreElements(); )
    {
        Ogre::MovableObject* object = dynamic_cast<Ogre::MovableObject*> (iter.getNext());
        node->getCreator()->destroyMovableObject (object);
    }

    for (Ogre::SceneNode::ChildNodeIterator iter = node->getChildIterator();
        iter.hasMoreElements(); )
    {
        Ogre::SceneNode* childNode = dynamic_cast<Ogre::SceneNode*> (iter.getNext());
        clearSceneNode (childNode);
        node->getCreator()->destroySceneNode (childNode);
   }
}

void CSVRender::ForeignObject::clear()
{
    mObject.setNull();

    if (mBase)
        clearSceneNode (mBase);
}

void CSVRender::ForeignObject::update()
{
    if(!mObject.isNull())
        mPhysics->removePhysicsObject(mBase->getName());

    clear();

    std::string model;
    int error = 0; // 1 referemceanöe does not exist, 2 referenceable does not specify a mesh

    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index==-1)
        error = 1;
    else
    {
        /// \todo check for Deleted state (error 1)

        model = referenceables.getData (index,
            referenceables.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).
            toString().toUtf8().constData();

        if (model.empty())
            error = 2;
    }

    if (error)
    {
        Ogre::Entity* entity = mBase->getCreator()->createEntity (Ogre::SceneManager::PT_CUBE);
        entity->setMaterialName("BaseWhite"); /// \todo adjust material according to error
        entity->setVisibilityFlags (Element_Reference);

        mBase->attachObject (entity);
    }
    else
    {
        mObject = NifOgre::Loader::createObjects (mBase, "Meshes\\" + model);
        mObject->setVisibilityFlags (Element_Reference);

        if (mPhysics && !(mReferenceId == 0))
        {
            const CSMWorld::CellRef& reference = getReference();

            // position
            Ogre::Vector3 position;
            if (!mForceBaseToZero)
                position = Ogre::Vector3(reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]);

            // orientation
            Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);
            Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
            Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

            // FIXME
            //mPhysics->addObject("meshes\\" + model, mBase->getName(), mReferenceId, reference.mScale, position, xr*yr*zr);
        }
    }
}

void CSVRender::ForeignObject::adjust()
{
    if (mReferenceId == 0)
        return;

    const CSMWorld::CellRef& reference = getReference();

    // position
    if (!mForceBaseToZero)
        mBase->setPosition (Ogre::Vector3 (
            reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]));

    // orientation
    Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);

    Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);

    Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

    mBase->setOrientation (xr*yr*zr);

    // scale
    mBase->setScale (reference.mScale, reference.mScale, reference.mScale);
}

const CSMWorld::CellRef& CSVRender::ForeignObject::getReference() const
{
    if (mReferenceId == 0)
        throw std::logic_error ("object does not represent a reference");

    return mData.getReferences().getRecord (mReferenceId).get();
}

CSVRender::ForeignObject::ForeignObject (const CSMWorld::Data& data, Ogre::SceneNode *cellNode,
    ESM4::FormId id, bool referenceable, boost::shared_ptr<CSVWorld::PhysicsSystem> physics,
    bool forceBaseToZero)
: mData (data), mBase (0), mForceBaseToZero (forceBaseToZero), mPhysics(physics)
{
    mBase = cellNode->createChildSceneNode();

    mReferenceId = id;
#if 0
    if (referenceable)
    {
        mReferenceableId = id;
    }
    else
    {
        mReferenceId = id;
        mReferenceableId = getReference().mRefID;
    }
#endif

    update();
    adjust();
}

CSVRender::ForeignObject::~ForeignObject()
{
    clear();

    if (mBase)
    {
        if(mPhysics) // preview may not have physics enabled
            mPhysics->removeObject(mBase->getName());

        mBase->getCreator()->destroySceneNode (mBase);
    }
}

bool CSVRender::ForeignObject::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        update();
        adjust();
        return true;
    }

    return false;
}

bool CSVRender::ForeignObject::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=start && index<=end)
    {
        // Deletion of referenceable-type objects is handled outside of Object.
        if (!(mReferenceId == 0))
        {
            update();
            adjust();
            return true;
        }
    }

    return false;
}

bool CSVRender::ForeignObject::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mReferenceId == 0)
        return false;

    const CSMForeign::RefCollection& references = mData.getForeignReferences();

    int index = references.searchId(mReferenceId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        int columnIndex =
            references.findColumnIndex (CSMWorld::Columns::ColumnId_ReferenceableId);

        if (columnIndex>=topLeft.column() && columnIndex<=bottomRight.row())
        {
            mReferenceableId =
                references.getData (index, columnIndex).toString().toUtf8().constData();
        }

        update();
        adjust();
        return true;
    }

    return false;
}

std::string CSVRender::ForeignObject::getReferenceId() const
{
    return "";// mReferenceId;
}

std::string CSVRender::ForeignObject::getReferenceableId() const
{
    return mReferenceableId;
}
