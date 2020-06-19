/*
  Copyright (C) 2017-2020 cc9cii

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

  The factory pattern is based on the answer provided by Roger Pate in
  https://stackoverflow.com/questions/1832003/instantiating-classes-by-name-with-factory-pattern

*/
#ifndef NIBTOGRE_NIOBJECT_H
#define NIBTOGRE_NIOBJECT_H

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

#include <boost/current_function.hpp>

#include "factory.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace NiBtOgre
{
    class NiStream;
    class Header;
    class NiModel;
    struct BuildData;

    class NiObject
    {
    public:
        virtual ~NiObject() {}

        //typedef std::vector<std::unique_ptr<NiObject> > RecordBlocks; // FIXME: no longer used

        // For some objects build() does not make sense - the default implementation does nothing.
        virtual void build(BuildData *data, NiObject *parent = nullptr) {}

        typedef NiBtOgre::Factory<NiObject> Factory;
        static Factory::Type create(Factory::Key const& name,
            uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
        {
            return mFactory.create(name, index, stream, model, data);
        }

        template<class Derived>
        static void define(Factory::Key const& name)
        {
            if (!mFactory.define(name, &Factory::template create_func<NiObject, Derived>))
                throw std::logic_error(std::string(BOOST_CURRENT_FUNCTION) + ": name already registered");
        }

        NiObject(std::uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
            : mModel(model), mSelfRef(index) {}

        inline std::uint32_t selfRef() const { return mSelfRef; }

    protected:
        NiObject() = default;  // disallow the default constructor in derived classes
        const NiModel& mModel; // a little akward, but need a way to access NiObject Ptrs/Refs and TES5 strings

        const std::uint32_t mSelfRef; // NIF block index of this object

    private:
        static Factory mFactory;
    };

    // TexCoord is used by:
    //
    // BSEffectShaderProperty
    // BSLightingShaderProperty
    // BSSkyShaderProperty
    // BSWaterShaderProperty
    // NiGeometryData
    // TexDesc
    struct TexCoord
    {
        float u;
        float v;
    };

    // index to strings in Header::mStrings
    typedef std::uint32_t StringIndex;

    // index to objects in NiModel::mObjects
    typedef std::int32_t AbstractAdditionalGeometryDataRef;
    typedef std::int32_t BSAnimNotesRef;
    typedef std::int32_t BSMultiBoundDataRef;
    typedef std::int32_t BSMultiBoundRef;
    typedef std::int32_t BSShaderTextureSetRef;
    typedef std::int32_t NiAVObjectRef;
    typedef std::int32_t NiBSplineBasisDataRef;
    typedef std::int32_t NiBSplineDataRef;
    typedef std::int32_t NiBoolDataRef;
    typedef std::int32_t NiCollisionObjectRef;
    typedef std::int32_t NiColorDataRef;
    typedef std::int32_t NiControllerSequenceRef;
    typedef std::int32_t NiDefaultAvObjectPaletteRef;
    typedef std::int32_t NiDynamicEffectRef;
    typedef std::int32_t NiExtraDataRef;
    typedef std::int32_t NiFloatDataRef;
    typedef std::int32_t NiFloatInterpolatorRef;
    typedef std::int32_t NiGeometryDataRef;
    typedef std::int32_t NiInterpolatorRef;
    typedef std::int32_t NiKeyframeDataRef;
    typedef std::int32_t NiMorphDataRef;
    typedef std::int32_t NiNodeRef;
    typedef std::int32_t NiObjectNETRef;
    typedef std::int32_t NiObjectRef;
    typedef std::int32_t NiPSysColliderRef;
    typedef std::int32_t NiPSysModifierRef;
    typedef std::int32_t NiPSysSpawnModifierRef;
    typedef std::int32_t NiPaletteRef;
    typedef std::int32_t NiParticleModifierRef;
    typedef std::int32_t NiParticleSystemRef;
    typedef std::int32_t NiPixelDataRef;
    typedef std::int32_t NiPoint3InterpolatorRef;
    typedef std::int32_t NiPosDataRef;
    typedef std::int32_t NiPropertyRef;
    typedef std::int32_t NiSkinDataRef;
    typedef std::int32_t NiSkinInstanceRef;
    typedef std::int32_t NiSkinPartitionRef;
    typedef std::int32_t NiSourceTextureRef;
    typedef std::int32_t NiStringPaletteRef;
    typedef std::int32_t NiTextKeyExtraDataRef;
    typedef std::int32_t NiTimeControllerRef;
    typedef std::int32_t NiTransformDataRef;
    typedef std::int32_t NiTriBasedGeomRef;
    typedef std::int32_t NiTriStripsDataRef;
    typedef std::int32_t NiUVDataRef;
    typedef std::int32_t NiVisDataRef;
    typedef std::int32_t bhkCompressedMeshShapeDataRef;
    typedef std::int32_t bhkSerializableRef;
    typedef std::int32_t bhkConvexShapeRef;
    typedef std::int32_t bhkShapeRef;
    typedef std::int32_t hkPackedNiTriStripsDataRef;
}

#endif // NIBTOGRE_NIOBJECT_H
