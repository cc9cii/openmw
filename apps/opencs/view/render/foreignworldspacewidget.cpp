#include "foreignworldspacewidget.hpp"

#include <sstream>

#include <QMouseEvent>

#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlayManager.h>
#include <OgreRoot.h>
#include <OgreSceneQuery.h>
#include <OgreSceneNode.h>
#include <OgreViewport.h>

#include <components/esm/loadland.hpp>
#include "textoverlay.hpp"
#include "overlaymask.hpp"

#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/pathgridcommands.hpp"

#include "../widget/scenetooltoggle.hpp"
#include "../widget/scenetoolmode.hpp"
#include "../widget/scenetooltoggle2.hpp"
#include "../world/physicssystem.hpp"

#include "pathgridpoint.hpp"
#include "editmode.hpp"
#include "elements.hpp"

bool CSVRender::ForeignWorldspaceWidget::adjustCells()
{
    bool modified = false;
    bool setCamera = false;

    const CSMForeign::CellCollection& cells = mDocument.getData().getForeignCells();

    {
        // remove (or name/region modified)
        std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());

        while (iter != mCells.end())
        {
            std::string cellId = iter->first.getId (mWorldspace);
            std::istringstream stream (cellId.c_str());
            char ignore;
            int x = 0;
            int y = 0;
            stream >> ignore >> x >> y;

            int32_t formId = cells.searchFormId (x, y); // FIXME: assumes Tamriel
            int index = cells.searchId (formId);


            if (!mSelection.has (iter->first) || index == -1 ||
                cells.getRecord(index).mState == CSMWorld::RecordBase::State_Deleted)
            {
                // delete overlays
                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator itOverlay = mTextOverlays.find(iter->first);
                if(itOverlay != mTextOverlays.end())
                {
                    delete itOverlay->second;
                    mTextOverlays.erase(itOverlay);
                }

                // destroy manual objects
                getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));

                delete iter->second;
                mCells.erase (iter++);

                modified = true;
            }
            else
            {
                // check if name or region field has changed
                // FIXME: config setting
                std::string name = cells.getRecord(index).get().mCellId;
                std::string region = cells.getRecord(index).get().mRegion;

                std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.find(iter->first);
                if(it != mTextOverlays.end())
                {
                    if(it->second->getDesc() != "") // previously had name
                    {
                        if(name != it->second->getDesc()) // new name
                        {
                            if(name != "")
                                it->second->setDesc(name);
                            else // name deleted, use region
                                it->second->setDesc(region);
                            it->second->update();
                        }
                    }
                    else if(name != "") // name added
                    {
                        it->second->setDesc(name);
                        it->second->update();
                    }
                    else if(region != it->second->getDesc()) // new region
                    {
                        it->second->setDesc(region);
                        it->second->update();
                    }
                    modified = true;
                }
                ++iter;
            }
        }
    }

    if (mCells.begin()==mCells.end())
        setCamera = true;

    // add
    for (CSMWorld::CellSelection::Iterator iter (mSelection.begin()); iter!=mSelection.end();
        ++iter)
    {
        //int index = cells.searchId (iter->getId (mWorldspace));

        std::string cellId = iter->getId (mWorldspace);
        std::istringstream stream (cellId.c_str());
        char ignore;
        int x = 0;
        int y = 0;
        stream >> ignore >> x >> y;

        int32_t formId = cells.searchFormId (x, y); // FIXME: assumes Tamriel
        int index = cells.searchId (formId);

        if (index > 0 && cells.getRecord (index).mState != CSMWorld::RecordBase::State_Deleted &&
            mCells.find (*iter)==mCells.end())
        {
            ForeignCell *cell = new ForeignCell (mDocument, getSceneManager(), formId, mDocument.getPhysics());

            //connect (cell->getSignalHandler(), SIGNAL(flagAsModified()), this, SLOT(flagAsModSlot()));
            mCells.insert (std::make_pair (*iter, cell));

            float height = cell->getTerrainHeightAt(Ogre::Vector3(
                              ESM4::Land::REAL_SIZE * iter->getX() + ESM4::Land::REAL_SIZE/2,
                              ESM4::Land::REAL_SIZE * iter->getY() + ESM4::Land::REAL_SIZE/2,
                              0));
            if (setCamera)
            {
                setCamera = false;
                getCamera()->setPosition (
                              ESM4::Land::REAL_SIZE * iter->getX() + ESM4::Land::REAL_SIZE/2,
                              ESM4::Land::REAL_SIZE * iter->getY() + ESM4::Land::REAL_SIZE/2,
                              height);
                // better camera position at the start
                getCamera()->move(getCamera()->getDirection() * -6000); // FIXME: config setting
            }

            Ogre::ManualObject* manual =
                    getSceneManager()->createManualObject("manual" + iter->getId(mWorldspace));
            manual->begin("BaseWhite", Ogre::RenderOperation::OT_LINE_LIST);
            // define start and end point (x, y, z)
            manual-> position(ESM4::Land::REAL_SIZE * iter->getX() + ESM4::Land::REAL_SIZE/2,
                              ESM4::Land::REAL_SIZE * iter->getY() + ESM4::Land::REAL_SIZE/2,
                              height);
            manual-> position(ESM4::Land::REAL_SIZE * iter->getX() + ESM4::Land::REAL_SIZE/2,
                              ESM4::Land::REAL_SIZE * iter->getY() + ESM4::Land::REAL_SIZE/2,
                              height+200); // FIXME: config setting
            manual->end();
            manual->setBoundingBox(Ogre::AxisAlignedBox(
                              ESM4::Land::REAL_SIZE * iter->getX() + ESM4::Land::REAL_SIZE/2,
                              ESM4::Land::REAL_SIZE * iter->getY() + ESM4::Land::REAL_SIZE/2,
                              height,
                              ESM4::Land::REAL_SIZE * iter->getX() + ESM4::Land::REAL_SIZE/2,
                              ESM4::Land::REAL_SIZE * iter->getY() + ESM4::Land::REAL_SIZE/2,
                              height+200));
            getSceneManager()->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
            manual->setVisible(false);

            CSVRender::TextOverlay *textDisp =
                    new CSVRender::TextOverlay(manual, getCamera(), iter->getId(mWorldspace));
            textDisp->enable(true);
            textDisp->setCaption(iter->getId(mWorldspace));
            std::string desc = cells.getRecord(index).get().mCellId;
            if(desc == "") desc = cells.getRecord(index).get().mRegion;
            textDisp->setDesc(desc); // FIXME: config setting
            textDisp->update();
            mTextOverlays.insert(std::make_pair(*iter, textDisp));
            if(!mOverlayMask)
            {
                mOverlayMask = new OverlayMask(mTextOverlays, getViewport());
                addRenderTargetListener(mOverlayMask);
            }

            modified = true;
        }
    }

    return modified;
}

void CSVRender::ForeignWorldspaceWidget::mousePressEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter = mTextOverlays.begin();
        for(; iter != mTextOverlays.end(); ++iter)
        {
            if(mDisplayCellCoord &&
               iter->second->isEnabled() && iter->second->container().contains(event->x(), event->y()))
            {
                return;
            }
        }
    }
    WorldspaceWidget::mousePressEvent(event);
}

void CSVRender::ForeignWorldspaceWidget::mouseReleaseEvent (QMouseEvent *event)
{
    if(event->button() == Qt::RightButton)
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter = mTextOverlays.begin();
        for(; iter != mTextOverlays.end(); ++iter)
        {
            if(mDisplayCellCoord &&
               iter->second->isEnabled() && iter->second->container().contains(event->x(), event->y()))
            {
                std::cout << "clicked: " << iter->second->getCaption() << std::endl;
                return;
            }
        }
    }
    WorldspaceWidget::mouseReleaseEvent(event);
}

void CSVRender::ForeignWorldspaceWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
    WorldspaceWidget::mouseDoubleClickEvent(event);
}

void CSVRender::ForeignWorldspaceWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle2 *tool)
{
    WorldspaceWidget::addVisibilitySelectorButtons (tool);
    tool->addButton (Element_Terrain, "Terrain");
    tool->addButton (Element_Fog, "Fog", "", true);
}

void CSVRender::ForeignWorldspaceWidget::addEditModeSelectorButtons (
    CSVWidget::SceneToolMode *tool)
{
    WorldspaceWidget::addEditModeSelectorButtons (tool);

    /// \todo replace EditMode with suitable subclasses
    tool->addButton (
        new EditMode (this, QIcon (":placeholder"), Element_Reference, "Terrain shape editing"),
        "terrain-shape");
    tool->addButton (
        new EditMode (this, QIcon (":placeholder"), Element_Reference, "Terrain texture editing"),
        "terrain-texture");
    tool->addButton (
        new EditMode (this, QIcon (":placeholder"), Element_Reference, "Terrain vertex paint editing"),
        "terrain-vertex");
    tool->addButton (
        new EditMode (this, QIcon (":placeholder"), Element_Reference, "Terrain movement"),
        "terrain-move");
}

void CSVRender::ForeignWorldspaceWidget::updateOverlay()
{
    if(getCamera()->getViewport())
    {
        if((uint32_t)getCamera()->getViewport()->getVisibilityMask()
                                & (uint32_t)CSVRender::Element_CellMarker)
            mDisplayCellCoord = true;
        else
            mDisplayCellCoord = false;
    }

    if(!mTextOverlays.empty())
    {
        std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator it = mTextOverlays.begin();
        for(; it != mTextOverlays.end(); ++it)
        {
            it->second->enable(mDisplayCellCoord);
            it->second->update();
        }
    }
}

void CSVRender::ForeignWorldspaceWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::referenceableAboutToBeRemoved (
    const QModelIndex& parent, int start, int end)
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::referenceableAdded (const QModelIndex& parent,
    int start, int end)
{
    CSMWorld::IdTable& referenceables = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        QModelIndex topLeft = referenceables.index (start, 0);
        QModelIndex bottomRight =
            referenceables.index (end, referenceables.columnCount());

        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
    }
}

void CSVRender::ForeignWorldspaceWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::referenceAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::referenceAdded (const QModelIndex& parent, int start,
    int end)
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        if (iter->second->referenceAdded (parent, start, end))
            flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::pathgridDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
        iter->second->pathgridDataChanged (topLeft, bottomRight);
}

CSVRender::ForeignCell *CSVRender::ForeignWorldspaceWidget::findCell(const std::string &cellId)
{
    const CSMWorld::IdCollection<CSMWorld::Cell>& cells = mDocument.getData().getCells();

    std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
    for(; iter!= mCells.end(); ++iter)
    {
        int index = cells.searchId(cellId);

        if (index != -1 && cellId == iter->first.getId (mWorldspace))
        {
            return iter->second;
        }
    }

    return NULL;
}

// NOTE: allow placing pathgrid points above objects and terrain
void CSVRender::ForeignWorldspaceWidget::pathgridInserted (const std::string &referenceId, const Ogre::Vector3 &pos)
{
    QString id = QString(referenceId.c_str());

    bool terrain = id.startsWith("HeightField_");
    bool object = QString(referenceId.c_str()).startsWith("ref#");
    // don't allow placing another one on top of a pathgrid point
    if (id.isEmpty() || (!terrain && !object))
        return;

    std::string cellId;
    if(terrain)
    {
        QRegExp nameRe("^HeightField_([\\d-]+)_([\\d-]+)$");
        if (nameRe.indexIn(id) == -1)
            return;

        int cellX = nameRe.cap(1).toInt();
        int cellY = nameRe.cap(2).toInt();

        std::ostringstream stream;
        stream << "#" << cellX << " " << cellY;
        cellId = stream.str();
    }
    else
    {
        const CSMWorld::RefCollection& references = mDocument.getData().getReferences();
        int index = references.searchId(referenceId);
        if(index == -1)
            return;

        cellId = references.getData(index, references.findColumnIndex(CSMWorld::Columns::ColumnId_Cell))
            .toString().toUtf8().constData();
    }

    ForeignCell *cell = findCell(cellId);
    if(cell)
    {
        cell->pathgridPointAdded(pos);
        flagAsModified();

        return;
    }
}

void CSVRender::ForeignWorldspaceWidget::pathgridMoved (const std::string &pgName, const Ogre::Vector3 &pos)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(pgName);

    ForeignCell *cell = findCell(result.first);
    if(cell)
    {
        cell->pathgridPointMoved(pgName, pos);
        flagAsModified();

        return;
    }
}

void CSVRender::ForeignWorldspaceWidget::pathgridAboutToBeRemoved (const std::string &pgName)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(pgName);

    ForeignCell *cell = findCell(result.first);
    if(cell)
    {
        cell->pathgridPointRemoved(pgName);
        flagAsModified();

        return;
    }
}

std::string CSVRender::ForeignWorldspaceWidget::getStartupInstruction()
{
    Ogre::Vector3 position = getCamera()->getPosition();

    std::ostringstream stream;

    stream
        << "player->position "
        << position.x << ", " << position.y << ", " << position.z
        << ", 0";

    return stream.str();
}

CSVRender::ForeignWorldspaceWidget::ForeignWorldspaceWidget (QWidget* parent, CSMDoc::Document& document)
: WorldspaceWidget (document, parent), mDocument (document), mWorldspace ("sys::foreign"),
  mControlElements(NULL), mDisplayCellCoord(true), mOverlayMask(NULL)
{
    QAbstractItemModel *cells =
        document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells);

    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (cells, SIGNAL (rowsRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (cellAdded (const QModelIndex&, int, int)));
}

CSVRender::ForeignWorldspaceWidget::~ForeignWorldspaceWidget()
{
    for (std::map<CSMWorld::CellCoordinates, ForeignCell *>::iterator iter (mCells.begin());
        iter!=mCells.end(); ++iter)
    {
        delete iter->second;

        getSceneManager()->destroyManualObject("manual"+iter->first.getId(mWorldspace));
    }

    for (std::map<CSMWorld::CellCoordinates, TextOverlay *>::iterator iter (mTextOverlays.begin());
        iter != mTextOverlays.end(); ++iter)
    {
        delete iter->second;
    }

    if(mOverlayMask)
    {
    removeRenderTargetListener(mOverlayMask);
    delete mOverlayMask;
}
}

void CSVRender::ForeignWorldspaceWidget::useViewHint (const std::string& hint)
{
    if (!hint.empty())
    {
        CSMWorld::CellSelection selection;

        if (hint[0]=='c')
        {
            // syntax: c:#x1 y1; #x2 y2 (number of coordinate pairs can be 0 or larger)
            char ignore;

            std::istringstream stream (hint.c_str());
            if (stream >> ignore)
            {
                char ignore1; // : or ;
                char ignore2; // #
                int x, y;

                // FIXME: should find a way to add world formid here (and embedded in the hint)
                while (stream >> ignore1 >> ignore2 >> x >> y)
                    selection.add (CSMWorld::CellCoordinates (x, y));

                /// \todo adjust camera position
            }
        }
        else if (hint[0]=='r')
        {
            /// \todo implement 'r' type hints
        }

        setCellSelection (selection);
    }
}

void CSVRender::ForeignWorldspaceWidget::setCellSelection (const CSMWorld::CellSelection& selection)
{
    mSelection = selection;

    if (adjustCells())
        flagAsModified();

    emit cellSelectionChanged (mSelection);
}

std::pair< int, int > CSVRender::ForeignWorldspaceWidget::getCoordinatesFromId (const std::string& record) const
{
    std::istringstream stream (record.c_str());
    char ignore;
    int x, y;
    stream >> ignore >> x >> y;
    return std::make_pair(x, y);
}

bool CSVRender::ForeignWorldspaceWidget::handleDrop (
    const std::vector< CSMWorld::UniversalId >& dropData, DropType type)
{
    if (WorldspaceWidget::handleDrop (dropData, type))
        return true;

    if (type!=Type_CellsExterior)
        return false;

    bool selectionChanged = false;
    for (unsigned i = 0; i < dropData.size(); ++i)
    {
        std::pair<int, int> coordinates(getCoordinatesFromId(dropData[i].getId()));
        if (mSelection.add(CSMWorld::CellCoordinates(coordinates.first, coordinates.second)))
        {
            selectionChanged = true;
        }
    }
    if (selectionChanged)
    {
        if (adjustCells())
            flagAsModified();

        emit cellSelectionChanged(mSelection);
    }

    return true;
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::ForeignWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::DropType type) const
{
    dropRequirments requirements = WorldspaceWidget::getDropRequirements (type);

    if (requirements!=ignored)
        return requirements;

    switch (type)
    {
        case Type_CellsExterior:
            return canHandle;

        case Type_CellsInterior:
            return needUnpaged;

        default:
            return ignored;
    }
}

unsigned int CSVRender::ForeignWorldspaceWidget::getVisibilityMask() const
{
    return WorldspaceWidget::getVisibilityMask() | mControlElements->getSelection();
}

CSVWidget::SceneToolToggle *CSVRender::ForeignWorldspaceWidget::makeControlVisibilitySelector (
    CSVWidget::SceneToolbar *parent)
{
    mControlElements = new CSVWidget::SceneToolToggle (parent,
        "Controls & Guides Visibility", ":placeholder");

    mControlElements->addButton (":placeholder", Element_CellMarker, ":placeholder",
        "Cell marker");
    mControlElements->addButton (":placeholder", Element_CellArrow, ":placeholder", "Cell arrows");
    mControlElements->addButton (":placeholder", Element_CellBorder, ":placeholder", "Cell border");

    mControlElements->setSelection (0xffffffff);

    connect (mControlElements, SIGNAL (selectionChanged()),
        this, SLOT (elementSelectionChanged()));

    return mControlElements;
}

void CSVRender::ForeignWorldspaceWidget::cellDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    /// \todo check if no selected cell is affected and do not update, if that is the case
    if (adjustCells())
        flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::cellRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (adjustCells())
        flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::cellAdded (const QModelIndex& index, int start,
    int end)
{
    /// \todo check if no selected cell is affected and do not update, if that is the case
    if (adjustCells())
        flagAsModified();
}

void CSVRender::ForeignWorldspaceWidget::flagAsModSlot ()
{
    flagAsModified();
}
