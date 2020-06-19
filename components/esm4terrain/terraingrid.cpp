/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Modified by cc9cii 2020
 */
#include "terraingrid.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreAxisAlignedBox.h>
#include <OgreTextureManager.h>

#include <components/terrain/chunk.hpp>

#include "storage.hpp"

namespace ESM4Terrain
{
TerrainGrid::TerrainGrid(Ogre::SceneManager *sceneMgr,
    Terrain::Storage *storage, int visibilityFlags, bool shaders, Terrain::Alignment align, ESM4::FormId world)
    : Terrain::TerrainGrid(sceneMgr, storage, visibilityFlags, shaders, align), mWorld(world)
{
    mRootNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(); // FIXME: use the parent's one instead?
}

TerrainGrid::~TerrainGrid()
{
    while (!mGrid.empty())
    {
        unloadCell(mGrid.begin()->first.first, mGrid.begin()->first.second);
    }

    mSceneMgr->destroySceneNode(mRootNode);
}

// called by RenderingManager::enableTerrain(bool enable, ESM4::FormId worldId)
// and RenderingManager::frameStarted(float dt, bool paused)
void TerrainGrid::update(const Ogre::Vector3 &cameraPos)
{
}

// Chunk is Ogre::Renderable and Ogre::MovableObject
// mChunk is attached to mSceneNode then its material updated
//
// struct GridElement
// {
//     Ogre::SceneNode* mSceneNode;
//     Terrain::MaterialGenerator mMaterialGenerator;
//     Terrain::Chunk* mChunk;
// };
//
// typedef std::map<std::pair<int, int>, std::vector<Terrain::GridElement> > TES4Grid;
//
// According to Ogre 1.9 API, a Renderable class must follow this:
//
//   Classes implementing this interface must be based on a single material, a single world
//   matrix (or a collection of world matrices which are blended by weights), and must be
//   renderable via a single render operation.
//
// That seems to imply that there can be only one material for a Chunk, which leads to having a
// chunk per quadrant of a cell for TES4 where each quadrant can have several different layers
// of textures blended in with different opacity levels.



// Q how to apply textures to only certain verticies?


// Not sure if a cell should have 4 chunks or 1
// If 4 chunks, need to split the cell and load the vertex, normals and colours 4 times
// If 1 chunk, not sure how to load the textures, and whether any future processing will affect it
void TerrainGrid::loadCell(int x, int y)
{
    Tes4Grid::iterator it = mGrid.find(std::make_pair(x, y));
    if (it != mGrid.end())
        return; // already loaded

    std::vector<Terrain::GridElement> elements;
    elements.resize(4);

    // FIXME: how to get TES4 data without changing the interface?  mStorage is a pointer
    // to the base class, so may need to resort to down-casting
    ESM4Terrain::Storage* storage = static_cast<ESM4Terrain::Storage*>(World::mStorage);

    // there are 4 quadrants: 0 = bottom left. 1 = bottom right. 2 = top left. 3 = top right
    for (int i = 0; i < 4; ++i)
    {
        // (y+1)*4096 +---------------+
        //            |               |
        //        .   |               |
        //        .   |               |
        //        .   |     cell      |
        //        .   |    (x, y)     |
        //            |               |
        //            |               |
        //     y*4096 +---------------+
        //          x*4096  .....  (x+1)*4096

        // FIXME: ? why add 0.5f here?
        //
        // But noticed that Storage::getMinMaxHeights() and Storage::fillVertexBuffers()
        // subtract Vector2(size/2.f, size2/f) to get origin, where size is 1 when called from
        // TerrainGrid::loadCell()
        Ogre::Vector2 center(x+0.5f, y+0.5f);
        float minH, maxH;
        if (!storage->getMinMaxQuadHeights(center, minH, maxH, i)) // FIXME: use origin rather than center?
            return; // no terrain defined

        Ogre::Vector3 min, max;
        // below should be different for each quadrant
        switch (i)
        {
            case 0:
            {
                min = Ogre::Vector3(-0.5f*World::mStorage->getCellWorldSize(),
                                    -0.5f*World::mStorage->getCellWorldSize(),
                                    minH);
                max = Ogre::Vector3(0,
                                    0,
                                    maxH);
                break;
            }
            case 1:
            {
                min = Ogre::Vector3(0,
                                    -0.5f*World::mStorage->getCellWorldSize(),
                                    minH);
                max = Ogre::Vector3(0.5f*World::mStorage->getCellWorldSize(),
                                    0,
                                    maxH);
                break;
            }
            case 2:
            {
                min = Ogre::Vector3(-0.5f*World::mStorage->getCellWorldSize(),
                                    0,
                                    minH);
                max = Ogre::Vector3(0,
                                    0.5f*World::mStorage->getCellWorldSize(),
                                    maxH);
                break;
            }
            case 3:
            {
                min = Ogre::Vector3(0,
                                    0,
                                    minH);
                max = Ogre::Vector3(0.5f*World::mStorage->getCellWorldSize(),
                                    0.5f*World::mStorage->getCellWorldSize(),
                                    maxH);
                break;
            }
            default:
                break;
        }

        // FIXME: workaround for strange culling
//#if 0
        min = Ogre::Vector3(-0.5f*mStorage->getCellWorldSize(),
                           -0.5f*mStorage->getCellWorldSize(),
                           minH);
        max = Ogre::Vector3(0.5f*mStorage->getCellWorldSize(),
                           0.5f*mStorage->getCellWorldSize(),
                           maxH);
//#endif

        Ogre::AxisAlignedBox bounds(min, max);

        Terrain::GridElement element;

        Ogre::Vector2 worldCenter = center*World::mStorage->getCellWorldSize();

        // this needs to be per quadrant
        switch (i)
        {
            case 0:
            {
                element.mSceneNode
                    = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x-1024, worldCenter.y-1024, 0));

                break;
            }
            case 1: // FIXME: why reverse? (now done in fillQuadVertexBuffers)
            {
                element.mSceneNode
                    = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x+1024, worldCenter.y-1024, 0));

                break;
            }
            case 2: // FIXME: why reverse? (now done in fillQuadVertexBuffers)
            {
                element.mSceneNode
                    = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x-1024, worldCenter.y+1024, 0));

                break;
            }
            case 3:
            {
                element.mSceneNode
                    = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x+1024, worldCenter.y+1024, 0));

                break;
            }
            default:
                break;
        }
        //element.mSceneNode = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x, worldCenter.y, 0));

        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<Ogre::uint8> colours;
        storage->fillQuadVertexBuffers(center, mAlign, positions, normals, colours, i);
        //FIXME: for testing old behaviour
        //storage->fillVertexBuffers(0, 1, center, mAlign, positions, normals, colours);

        element.mChunk = new Terrain::Chunk(mCache.getUVBuffer(), bounds, positions, normals, colours);
        element.mChunk->setIndexBuffer(mCache.getIndexBuffer(0));
        element.mChunk->setVisibilityFlags(mVisibilityFlags);
        element.mChunk->setCastShadows(true);

        std::vector<Ogre::PixelBox> blendmaps;
        std::vector<Terrain::LayerInfo> layerList;
        storage->getQuadBlendmaps(center, mShaders, blendmaps, layerList, i); // FIXME: center or cell?

        element.mMaterialGenerator.setLayerList(layerList);

        // upload blendmaps to GPU
        std::vector<Ogre::TexturePtr> blendTextures;
        for (std::vector<Ogre::PixelBox>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
        {
            static int count=0;
            Ogre::TexturePtr map = Ogre::TextureManager::getSingleton().createManual("terrain/blend/"
                + Ogre::StringConverter::toString(count++), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, it->getWidth(), it->getHeight(), 0, it->format);

            Ogre::DataStreamPtr stream(OGRE_NEW Ogre::MemoryDataStream(it->data, it->getWidth()*it->getHeight()*Ogre::PixelUtil::getNumElemBytes(it->format), true));
            map->loadRawData(stream, it->getWidth(), it->getHeight(), it->format);
            blendTextures.push_back(map);
        }

        element.mMaterialGenerator.setBlendmapList(blendTextures);

        element.mSceneNode->attachObject(element.mChunk);
        updateMaterial(element);

        elements[i] = element;
        // FIXME: for testing old behaviour
        //mGrid[std::make_pair(x,y)] = element;
    }
    mGrid.insert(std::make_pair(std::make_pair(x,y), std::move(elements)));
}

void TerrainGrid::unloadCell(int x, int y)
{
    Tes4Grid::iterator it = mGrid.find(std::make_pair(x, y));
    if (it == mGrid.end())
        return;

    std::vector<Terrain::GridElement>& quads = it->second;

    for (std::size_t i = 0; i < quads.size(); ++i)
    {
        Terrain::GridElement& element = quads[i];
        delete element.mChunk;
        element.mChunk = NULL;

        const std::vector<Ogre::TexturePtr>& blendmaps = element.mMaterialGenerator.getBlendmapList();
        for (std::vector<Ogre::TexturePtr>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
            Ogre::TextureManager::getSingleton().remove((*it)->getName(),
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        mSceneMgr->destroySceneNode(element.mSceneNode);
        element.mSceneNode = NULL;
    }

    mGrid.erase(it);
}

void TerrainGrid::updateMaterial(Terrain::GridElement &element)
{
    element.mMaterialGenerator.enableShadows(getShadowsEnabled());
    element.mMaterialGenerator.enableSplitShadows(getSplitShadowsEnabled());
    element.mChunk->setMaterial(element.mMaterialGenerator.generate());
}

void TerrainGrid::applyMaterials(bool shadows, bool splitShadows)
{
    mShadows = shadows;
    mSplitShadows = splitShadows;
    for (Tes4Grid::iterator it = mGrid.begin(); it != mGrid.end(); ++it)
    {
        std::vector<Terrain::GridElement>& quads = it->second;
        for (std::size_t i = 0; i < quads.size(); ++i)
        {
            updateMaterial(quads[i]);
        }
    }
}

bool TerrainGrid::getVisible()
{
    return mVisible;
}

void TerrainGrid::setVisible(bool visible)
{
    mVisible = visible;
    mRootNode->setVisible(visible);
}

Ogre::AxisAlignedBox TerrainGrid::getWorldBoundingBox (const Ogre::Vector2& center)
{
    int cellX = static_cast<int>(std::floor(center.x));
    int cellY = static_cast<int>(std::floor(center.y));

    Tes4Grid::iterator it = mGrid.find(std::make_pair(cellX, cellY));
    if (it == mGrid.end())
        return Ogre::AxisAlignedBox::BOX_NULL;

    Ogre::AxisAlignedBox bigBox(Ogre::AxisAlignedBox::BOX_NULL);
    std::vector<Terrain::GridElement>& quads = it->second;
    for (std::size_t i = 0; i < quads.size(); ++i)
    {
        Terrain::Chunk* chunk = quads[i].mChunk;
        Ogre::SceneNode* node = quads[i].mSceneNode;
        Ogre::AxisAlignedBox box = chunk->getBoundingBox();

        // need to combine the 4 boxes
        bigBox.merge(Ogre::AxisAlignedBox(box.getMinimum() + node->getPosition(),
                                          box.getMaximum() + node->getPosition()));
        return bigBox;
    }
}

void TerrainGrid::syncLoad()
{

}
} // namesapce ESM4Terrain
