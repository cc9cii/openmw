#include "niffile.hpp"
#include "effect.hpp"

#include <map>

#include <OgreResourceGroupManager.h>

namespace Nif
{

/// Open a NIF stream. The name is used for error messages.
NIFFile::NIFFile(const std::string &name)
    : mVer(0), mUserVer(0), mUserVer2(0)
    , filename(name)
{
#if 0
    parse();
#else
    NIFStream nif (this, Ogre::ResourceGroupManager::getSingleton().openResource(filename));

    std::vector<std::string> blockTypes;
    std::vector<unsigned short> blockTypeIndex;
    std::vector<unsigned int> blockSize;
    std::vector<std::string> strings;

    parseHeader(nif, blockTypes, blockTypeIndex, blockSize, strings);
    nif.close();
#endif
}

NIFFile::~NIFFile()
{
    for (std::vector<Record*>::iterator it = records.begin() ; it != records.end(); ++it)
    {
        delete *it;
    }
}

template <typename NodeType> static Record* construct() { return new NodeType; }

struct RecordFactoryEntry {

    typedef Record* (*create_t) ();

    create_t        mCreate;
    RecordType      mType;

};

///Helper function for adding records to the factory map
static std::pair<std::string,RecordFactoryEntry> makeEntry(std::string recName, Record* (*create_t) (), RecordType type)
{
    RecordFactoryEntry anEntry = {create_t,type};
    return std::make_pair(recName, anEntry);
}

///These are all the record types we know how to read.
static std::map<std::string,RecordFactoryEntry> makeFactory()
{
    std::map<std::string,RecordFactoryEntry> newFactory;
    newFactory.insert(makeEntry("NiNode",                     &construct <NiNode>                      , RC_NiNode                        ));
    newFactory.insert(makeEntry("AvoidNode",                  &construct <NiNode>                      , RC_AvoidNode                     ));
    newFactory.insert(makeEntry("NiBSParticleNode",           &construct <NiNode>                      , RC_NiBSParticleNode              ));
    newFactory.insert(makeEntry("NiBSAnimationNode",          &construct <NiNode>                      , RC_NiBSAnimationNode             ));
    newFactory.insert(makeEntry("NiBillboardNode",            &construct <NiNode>                      , RC_NiBillboardNode               ));
    newFactory.insert(makeEntry("NiTriShape",                 &construct <NiTriShape>                  , RC_NiTriShape                    ));
    newFactory.insert(makeEntry("NiRotatingParticles",        &construct <NiRotatingParticles>         , RC_NiRotatingParticles           ));
    newFactory.insert(makeEntry("NiAutoNormalParticles",      &construct <NiAutoNormalParticles>       , RC_NiAutoNormalParticles         ));
    newFactory.insert(makeEntry("NiCamera",                   &construct <NiCamera>                    , RC_NiCamera                      ));
    newFactory.insert(makeEntry("RootCollisionNode",          &construct <NiNode>                      , RC_RootCollisionNode             ));
    newFactory.insert(makeEntry("NiTexturingProperty",        &construct <NiTexturingProperty>         , RC_NiTexturingProperty           ));
    newFactory.insert(makeEntry("NiFogProperty",              &construct <NiFogProperty>               , RC_NiFogProperty                 ));
    newFactory.insert(makeEntry("NiMaterialProperty",         &construct <NiMaterialProperty>          , RC_NiMaterialProperty            ));
    newFactory.insert(makeEntry("NiZBufferProperty",          &construct <NiZBufferProperty>           , RC_NiZBufferProperty             ));
    newFactory.insert(makeEntry("NiAlphaProperty",            &construct <NiAlphaProperty>             , RC_NiAlphaProperty               ));
    newFactory.insert(makeEntry("NiVertexColorProperty",      &construct <NiVertexColorProperty>       , RC_NiVertexColorProperty         ));
    newFactory.insert(makeEntry("NiShadeProperty",            &construct <NiShadeProperty>             , RC_NiShadeProperty               ));
    newFactory.insert(makeEntry("NiDitherProperty",           &construct <NiDitherProperty>            , RC_NiDitherProperty              ));
    newFactory.insert(makeEntry("NiWireframeProperty",        &construct <NiWireframeProperty>         , RC_NiWireframeProperty           ));
    newFactory.insert(makeEntry("NiSpecularProperty",         &construct <NiSpecularProperty>          , RC_NiSpecularProperty            ));
    newFactory.insert(makeEntry("NiStencilProperty",          &construct <NiStencilProperty>           , RC_NiStencilProperty             ));
    newFactory.insert(makeEntry("NiVisController",            &construct <NiVisController>             , RC_NiVisController               ));
    newFactory.insert(makeEntry("NiGeomMorpherController",    &construct <NiGeomMorpherController>     , RC_NiGeomMorpherController       ));
    newFactory.insert(makeEntry("NiKeyframeController",       &construct <NiKeyframeController>        , RC_NiKeyframeController          ));
    newFactory.insert(makeEntry("NiAlphaController",          &construct <NiAlphaController>           , RC_NiAlphaController             ));
    newFactory.insert(makeEntry("NiUVController",             &construct <NiUVController>              , RC_NiUVController                ));
    newFactory.insert(makeEntry("NiPathController",           &construct <NiPathController>            , RC_NiPathController              ));
    newFactory.insert(makeEntry("NiMaterialColorController",  &construct <NiMaterialColorController>   , RC_NiMaterialColorController     ));
    newFactory.insert(makeEntry("NiBSPArrayController",       &construct <NiBSPArrayController>        , RC_NiBSPArrayController          ));
    newFactory.insert(makeEntry("NiParticleSystemController", &construct <NiParticleSystemController>  , RC_NiParticleSystemController    ));
    newFactory.insert(makeEntry("NiFlipController",           &construct <NiFlipController>            , RC_NiFlipController              ));
    newFactory.insert(makeEntry("NiAmbientLight",             &construct <NiLight>                     , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiDirectionalLight",         &construct <NiLight>                     , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiTextureEffect",            &construct <NiTextureEffect>             , RC_NiTextureEffect               ));
    newFactory.insert(makeEntry("NiVertWeightsExtraData",     &construct <NiVertWeightsExtraData>      , RC_NiVertWeightsExtraData        ));
    newFactory.insert(makeEntry("NiTextKeyExtraData",         &construct <NiTextKeyExtraData>          , RC_NiTextKeyExtraData            ));
    newFactory.insert(makeEntry("NiStringExtraData",          &construct <NiStringExtraData>           , RC_NiStringExtraData             ));
    newFactory.insert(makeEntry("NiGravity",                  &construct <NiGravity>                   , RC_NiGravity                     ));
    newFactory.insert(makeEntry("NiPlanarCollider",           &construct <NiPlanarCollider>            , RC_NiPlanarCollider              ));
    newFactory.insert(makeEntry("NiParticleGrowFade",         &construct <NiParticleGrowFade>          , RC_NiParticleGrowFade            ));
    newFactory.insert(makeEntry("NiParticleColorModifier",    &construct <NiParticleColorModifier>     , RC_NiParticleColorModifier       ));
    newFactory.insert(makeEntry("NiParticleRotation",         &construct <NiParticleRotation>          , RC_NiParticleRotation            ));
    newFactory.insert(makeEntry("NiFloatData",                &construct <NiFloatData>                 , RC_NiFloatData                   ));
    newFactory.insert(makeEntry("NiTriShapeData",             &construct <NiTriShapeData>              , RC_NiTriShapeData                ));
    newFactory.insert(makeEntry("NiVisData",                  &construct <NiVisData>                   , RC_NiVisData                     ));
    newFactory.insert(makeEntry("NiColorData",                &construct <NiColorData>                 , RC_NiColorData                   ));
    newFactory.insert(makeEntry("NiPixelData",                &construct <NiPixelData>                 , RC_NiPixelData                   ));
    newFactory.insert(makeEntry("NiMorphData",                &construct <NiMorphData>                 , RC_NiMorphData                   ));
    newFactory.insert(makeEntry("NiKeyframeData",             &construct <NiKeyframeData>              , RC_NiKeyframeData                ));
    newFactory.insert(makeEntry("NiSkinData",                 &construct <NiSkinData>                  , RC_NiSkinData                    ));
    newFactory.insert(makeEntry("NiUVData",                   &construct <NiUVData>                    , RC_NiUVData                      ));
    newFactory.insert(makeEntry("NiPosData",                  &construct <NiPosData>                   , RC_NiPosData                     ));
    newFactory.insert(makeEntry("NiRotatingParticlesData",    &construct <NiRotatingParticlesData>     , RC_NiRotatingParticlesData       ));
    newFactory.insert(makeEntry("NiAutoNormalParticlesData",  &construct <NiAutoNormalParticlesData>   , RC_NiAutoNormalParticlesData     ));
    newFactory.insert(makeEntry("NiSequenceStreamHelper",     &construct <NiSequenceStreamHelper>      , RC_NiSequenceStreamHelper        ));
    newFactory.insert(makeEntry("NiSourceTexture",            &construct <NiSourceTexture>             , RC_NiSourceTexture               ));
    newFactory.insert(makeEntry("NiSkinInstance",             &construct <NiSkinInstance>              , RC_NiSkinInstance                ));
    return newFactory;
}


///Make the factory map used for parsing the file
static const std::map<std::string,RecordFactoryEntry> factories = makeFactory();

///Good nif file headers
static std::vector<std::string> makeGoodHeaders()
{
    std::vector<std::string> header;
    header.push_back("NetImmerse File Format");
    header.push_back("Gamebryo File Format");
    return header;
}
static const std::vector<std::string> goodHeaders = makeGoodHeaders();

///Good nif file versions
static std::set<unsigned int> makeGoodVersions()
{
    std::set<unsigned int> versions;
    versions.insert(0x04000002); // Morrowind NIF version
	versions.insert(0x0a000100); // TES4 old
    versions.insert(0x0a020000); // TES4 old
    versions.insert(0x0a01006a); // TES4 GOG
    versions.insert(0x14000004);
    versions.insert(0x14000005); // Oblivion
    versions.insert(0x14020007); // Skyrim
    return versions;
}
static const std::set<unsigned int> GoodVersions = makeGoodVersions();

/// Get the file's version in a human readable form
std::string NIFFile::printVersion(unsigned int version)
{
    union ver_quad
    {
        uint32_t full;
        uint8_t quad[4];
    } version_out;

    version_out.full = version;

    return Ogre::StringConverter::toString(version_out.quad[3])
    +"." + Ogre::StringConverter::toString(version_out.quad[2])
    +"." + Ogre::StringConverter::toString(version_out.quad[1])
    +"." + Ogre::StringConverter::toString(version_out.quad[0]);
}

size_t NIFFile::parseHeader(NIFStream nif,
        std::vector<std::string>& blockTypes, std::vector<unsigned short>& blockTypeIndex,
        std::vector<unsigned int>& blockSize, std::vector<std::string>& strings)
{
    //The number of records/blocks to get from the file
    size_t numBlocks = 0;

    // Check the header string
    std::string head = nif.getVersionString();
    bool isGood = false;
    for (std::vector<std::string>::const_iterator it = goodHeaders.begin() ; it != goodHeaders.end(); it++)
    {
        if (head.find(*it) != std::string::npos)
        isGood = true;
    }
    if (!isGood)
        fail("Invalid NIF header:  " + head);

    // Get BCD version
    mVer = nif.getUInt();
    if (GoodVersions.find(mVer) == GoodVersions.end())
        fail("Unsupported NIF version: " + printVersion(mVer));

    if (mVer >= 0x14000004) // 20.0.0.4
    {
        bool isLittleEndian = !!nif.getChar();
        if (!isLittleEndian)
            fail("Is not Little Endian");
    }

    if (mVer >= 0x0a010000) // 10.1.0.0
        mUserVer = nif.getUInt();

    numBlocks = nif.getInt();

    if (mVer >= 0x0a000100) // 5.0.0.1
    {
        if ((mUserVer >= 10) || ((mUserVer == 1) && (mVer != 0x0a020000)))
        {
            mUserVer2 = nif.getUInt();

            char size = nif.getChar();
            nif.getString(size); //string creator
            size = nif.getChar();
            nif.getString(size); //string exportInfo1
            size = nif.getChar();
            nif.getString(size); //string exportInfo1
        }

        unsigned short numBlockTypes = nif.getUShort();
        for (unsigned int i = 0; i < numBlockTypes; ++i)
            blockTypes.push_back(nif.getString());

        blockTypeIndex.resize(numBlocks);
        for (unsigned int i = 0; i < numBlocks; ++i)
            blockTypeIndex[i] = nif.getUShort();
    }

    if (mVer >= 0x14020007) // 20.2.0.7
    {
        blockSize.resize(numBlocks);
        for (unsigned int i = 0; i < numBlocks; ++i)
            blockSize[i] = nif.getUInt();
    }

    if (mVer >= 0x14010003) // 20.1.0.3
    {
        unsigned int numStrings = nif.getUInt();
        unsigned int maxStringLength = nif.getUInt();
        unsigned int size = 0;
        strings.resize(numStrings);
        for (unsigned int i = 0; i < numStrings; ++i)
        {
            size = nif.getUInt();
            strings[i] = nif.getString(size);
        }
    }

    if (mVer >= 0x0a000100) // 5.0.0.6
    {
        nif.getUInt(); //unsigned int unknown
    }

	if (mVer == 0x0a000100)
    {
		nif.getUInt(); // hack
    }

    return numBlocks;
}

void NIFFile::parse()
{
    NIFStream nif (this, Ogre::ResourceGroupManager::getSingleton().openResource(filename/*, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME*/));

    std::vector<std::string> blockTypes;
    std::vector<unsigned short> blockTypeIndex;
    std::vector<unsigned int> blockSize;
    std::vector<std::string> strings;

    // Parse the header, and get the Number of records
    size_t recNum = parseHeader(nif, blockTypes, blockTypeIndex, blockSize, strings);

    records.resize(recNum);
    for(size_t i = 0; i < recNum; ++i)
    {
        Record *r = NULL;

        std::string rec;
        if (mVer >= 0x0a000100) // 10.0.1.0
            rec = blockTypes[blockTypeIndex[i]];
        else                   // 4.0.0.2 (for example)
            rec = nif.getString();

        if (rec.empty())
            fail("Record number " + Ogre::StringConverter::toString(i)
                     + " out of " + Ogre::StringConverter::toString(recNum) + " is blank.");

        std::map<std::string,RecordFactoryEntry>::const_iterator entry = factories.find(rec);

        if (entry != factories.end())
        {
            r = entry->second.mCreate ();
            r->recType = entry->second.mType;
        }
        else
            fail("Unknown record type " + rec);

        assert(r != NULL);
        assert(r->recType != RC_MISSING);
        r->recName = rec;
        r->recIndex = i;

        r->nifVer = mVer;
        r->userVer = mUserVer;
        r->userVer2 = mUserVer2;
        r->strings = &strings;

        records[i] = r;
        r->read(&nif);
    }

    size_t rootNum = nif.getUInt();
    roots.resize(rootNum);

    //Determine which records are roots
    for(size_t i = 0;i < rootNum;i++)
    {
        int idx = nif.getInt();
        if (idx >= 0)
        {
            roots[i] = records.at(idx);
        }
        else
        {
            roots[i] = NULL;
            warn("Null Root found");
        }
    }

    // Once parsing is done, do post-processing.
    for(size_t i=0; i<recNum; i++)
        records[i]->post(this);
}

}
