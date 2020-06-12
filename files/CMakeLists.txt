project(resources)

set(WATER_FILES
    water_nm.png
    circle.png
)

set(MATERIAL_FILES
    atmosphere.shader
    atmosphere.shaderset
    clouds.shader
    clouds.shaderset
    core.h
    moon.shader
    moon.shaderset
    objects.mat
    objects.shader
    objects.shaderset
    tes5objects.mat
    tes5objects.shader
    tes5objects.shaderset
    tes5msnobjects.mat
    tes5msnobjects.shader
    tes5msnobjects.shaderset
    openmw.configuration
    pathgrid_pt.nif
    quad.mat
    quad.shader
    quad.shaderset
    shadowcaster.mat
    shadowcaster.shader
    shadowcaster.shaderset
    shadows.h
    sky.mat
    stars.shader
    stars.shaderset
    sun.shader
    sun.shaderset
    terrain.shader
    terrain.shaderset
    tes4terrain.shader
    tes4terrain.shaderset
    underwater.h
    water.mat
    water.shader
    water.shaderset
    selection.mat
    selection.shader
    selection.shaderset
    mygui.mat
    mygui.shader
    mygui.shaderset
    ripples.particle
    clouds.jpg
    clouds.material
)

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/water "${OpenMW_BINARY_DIR}/resources/water/" "${WATER_FILES}")

copy_all_files(${CMAKE_CURRENT_SOURCE_DIR}/materials "${OpenMW_BINARY_DIR}/resources/materials/" "${MATERIAL_FILES}")
