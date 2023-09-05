project (Components)

if(APPLE)
    set(BUNDLE_RESOURCES_DIR "${APP_BUNDLE_DIR}/Contents/Resources")
    set(OPENMW_RESOURCES_ROOT ${BUNDLE_RESOURCES_DIR})
endif(APPLE)

# Version file
set (VERSION_IN_FILE "${OpenMW_SOURCE_DIR}/files/version.in")
set (VERSION_FILE_PATH_BASE "${OpenMW_BINARY_DIR}")
set (VERSION_FILE_PATH_RELATIVE resources/version)
if (GIT_CHECKOUT)
    get_generator_is_multi_config(multi_config)
    add_custom_target (git-version
       COMMAND ${CMAKE_COMMAND}
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
            -DVERSION_IN_FILE=${VERSION_IN_FILE}
            -DVERSION_FILE_PATH_BASE=${VERSION_FILE_PATH_BASE}
            -DVERSION_FILE_PATH_RELATIVE=${VERSION_FILE_PATH_RELATIVE}
            -DOPENMW_VERSION_MAJOR=${OPENMW_VERSION_MAJOR}
            -DOPENMW_VERSION_MINOR=${OPENMW_VERSION_MINOR}
            -DOPENMW_VERSION_RELEASE=${OPENMW_VERSION_RELEASE}
            -DOPENMW_VERSION=${OPENMW_VERSION}
            -DMACROSFILE=${CMAKE_SOURCE_DIR}/cmake/OpenMWMacros.cmake
            "-DCMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}"
            -Dgenerator_is_multi_config_var=${multi_config}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/GitVersion.cmake
            VERBATIM)
else (GIT_CHECKOUT)
    configure_resource_file(${VERSION_IN_FILE} ${VERSION_FILE_PATH_BASE} ${VERSION_FILE_PATH_RELATIVE})
endif (GIT_CHECKOUT)

# source files

add_component_dir (lua
    luastate scriptscontainer asyncpackage utilpackage serialization configuration l10n storage
    )

add_component_dir (l10n
    messagebundles manager
    )

#vsgopenmw-fixme(rewrite-me)
add_component_dir (settings
    settings parser
    )

#<vsgopenmw-code-style>
# Views terrain.
# Depends on pipeline.
add_component_dir (terrain
    bounds storage indexbuffer builder
    lod view viewdata paging preload
    )
# Adapts nif to vsg::Node and kf to Anim::ControllerMap.
# Depends on nif, pipeline, animation.
add_component_dir (vsgadapters/nif
    nif anim channel particle kf
    )
add_component_dir (vsgadapters
    # Encapsulates vfs component.
    vfs
    # Temporarily converts types.
    osgcompat
    # Avoids linking full OSG library for yet-to-be-ported modules.
    osg/Matrixf osg/Matrixd osg/Quat osg/MatrixDecomposition
    )
add_component_dir (vsgadapters/sdl
    window surface
    )
add_component_dir (vsgadapters/mygui
    render texture manualtexture
    )
# Assembles vulkan pipelines.
add_component_dir (pipeline
    builder options override graphics particle particledata terrain descriptorvalue layout sets bindings computebindings viewbindings viewdata material descriptorimagecache descriptorbuffercache lightgrid fogofwar worldoverlay
    )
# Implements algorithms.
add_component_dir (render
    engine download screenshot headless limitframerate rendertexture cascade computeimage
    screen depth attachmentformat renderpass multisample
    )
# Renders pipelines.
# Depends on pipeline, render.
add_component_dir (view
    scene descriptors collectlights defaultstate lightgrid shadow
    )
# Provides dynamic scene graph objects.
add_component_dir (animation
    # Channels map time to value.
    channel interpolate extrapolate path range reset
    # Controllers modify objects.
    controller tcontroller tmutable updatedata rotate roll skin morph transformcontroller texmat switch stateswitch color cullparticles
    # Nodes may be placed in the scene graph.
    transform billboard
    # Tools support nodes.
    softwareskin clone attachbone
    # Logic plays animations.
    animation visitor context update autoplay transformupdate tags bones controllermap contents meta
    )
# Creates animation scene graph.
# Depends on esm, animation.
add_component_dir (mwanimation
    context play groups clone decorate attach
    color position light lighteffect effect effects
    create object objectanim actor wielding face
    )
# Conveniently extends vsg.
# vsgutil must not depend on other components.
# vsgutil may be implicitely depended on.
#add_base_dir(vsgutil
add_component_dir (vsgutil
    # Object extensions.
    attachable name id operation image
    # Nodes.
    setviewportstate projection viewrelative computebin nullbin suspendrenderpass
    # Algorithms.
    bounds transform computetransform intersectnormal toggle group compilecontext compileop updatethreads arraytexture
    # Visitors.
    traverse traversestate nodepath sharebuffer cullnode searchbyname searchbytype cameradistanceintersector arraystate
    # I/O.
    cache virtualfile imageio readimage decompress readnode readshader share shareimage deletionqueue
    # Temporary workarounds.
    setbin sharedview
    )
# Supports MyGUI.
# Depends on vfs, vsgadapters/mygui.
add_component_dir (myguiplatform #(mygui
    # Backends.
    vfs platform loglistener
    # Layers.
    scalinglayer #additivelayer
    ## Widgets.
    #
    ## Loaders.
    #fontloader
    )
#</vsgopenmw-code-style>

add_component_dir (bsa
    bsa_file compressedbsafile ba2gnrlfile ba2dx10file ba2file memorystream
    )

add_component_dir (vfs
    manager archive bsaarchive filesystemarchive registerarchives
    )

#<vsgopenmw-code-style="pending">
# Assembles I/O components for use.
# Depends on vsgadapters/vfs, vsgadapters/nif, nifbullet.
add_component_dir (resource
    # Interfaces.
    resourcesystem
    # ReaderWriters.
    shadersettings fallback decompress bulletshape bulletshapereader
    niffilemanager resourcemanager #vsgopenmw-delete-me
    objectcache multiobjectcache # bulletshapemanager #vsgopenmw-delete-me
    # foreachbulletobject stats # vsgopenmw-fixme(find-my-place)
    )
#</vsgopenmw-code-style>

#vsgopenmw-delete-me
#[[
add_component_dir (shader
    shadermanager shadervisitor removedalphafunc
    )

add_component_dir (sceneutil
    clone attach visitor util statesetupdater controller skeleton riggeometry morphgeometry lightcontroller
    lightmanager lightutil positionattitudetransform workqueue pathgridutil waterutil writescene serialize optimizer
    actorutil detourdebugdraw navmesh agentpath shadow mwshadowtechnique recastmesh shadowsbin osgacontroller rtt
    screencapture depth color riggeometryosgaextension extradata unrefqueue lightcommon
    )
]]

add_component_dir (nif
    controlled effect niftypes record controller extra node record_ptr data niffile property nifkey base nifstream physics
    )

add_component_dir (nifbullet
    bulletnifloader
    )

add_component_dir (to_utf8
    to_utf8
    )

add_component_dir(esm attr common defs esmcommon records util luascripts format refid esmbridge
    formidrefid
    stringrefid
    generatedrefid
    indexrefid
    serializerefid
    )

#vsgopenmw-delete-me
#[[
add_component_dir(fx pass technique lexer widgets stateupdater)

add_component_dir(std140 ubo)
]]

add_component_dir (esm3
    esmreader esmwriter loadacti loadalch loadappa loadarmo loadbody loadbook loadbsgn loadcell
    loadclas loadclot loadcont loadcrea loaddial loaddoor loadench loadfact loadglob loadgmst
    loadinfo loadingr loadland loadlevlist loadligh loadlock loadprob loadrepa loadltex loadmgef loadmisc
    loadnpc loadpgrd loadrace loadregn loadscpt loadskil loadsndg loadsoun loadspel loadsscr loadstat
    loadweap aipackage effectlist spelllist variant variantimp loadtes3 cellref filter
    savedgame journalentry queststate locals globalscript player objectstate cellid cellstate globalmap
    inventorystate containerstate npcstate creaturestate dialoguestate statstate npcstats creaturestats
    weatherstate quickkeys fogstate spellstate activespells creaturelevliststate doorstate projectilestate debugprofile
    aisequence magiceffects custommarkerstate stolenitems transport animationstate controlsstate mappings readerscache
    infoorder
    )

add_component_dir (esm3terrain
    storage
    )

#vsgopenmw-fixme(find-my-place)
#[[
add_component_dir (esm4
    acti
    actor
    common
    dialogue
    effect
    formid
    inventory
    lighting
    loadachr
    loadacre
    loadacti
    loadalch
    loadaloc
    loadammo
    loadanio
    loadappa
    loadarma
    loadarmo
    loadaspc
    loadbook
    loadbptd
    loadcell
    loadclas
    loadclfm
    loadclot
    loadcont
    loadcrea
    loaddial
    loaddobj
    loaddoor
    loadeyes
    loadflor
    loadflst
    loadfurn
    loadglob
    loadgras
    loadgrup
    loadhair
    loadhdpt
    loadidle
    loadidlm
    loadimod
    loadinfo
    loadingr
    loadkeym
    loadland
    loadlgtm
    loadligh
    loadltex
    loadlvlc
    loadlvli
    loadlvln
    loadmato
    loadmisc
    loadmset
    loadmstt
    loadmusc
    loadnavi
    loadnavm
    loadnote
    loadnpc
    loadotft
    loadpack
    loadpgrd
    loadpgre
    loadpwat
    loadqust
    loadrace
    loadrefr
    loadregn
    loadroad
    loadsbsp
    loadscol
    loadscpt
    loadscrl
    loadsgst
    loadslgm
    loadsndr
    loadsoun
    loadstat
    loadtact
    loadterm
    loadtes4
    loadtree
    loadtxst
    loadweap
    loadwrld
    reader
    reference
    script
    readerutils
    typetraits
)
#]]

#add_base_dir(stl
add_component_dir (misc
    # Conveniently extends STL.
    # misc must not depend on other components.
    # misc may be implicitely depended on.
    utf8stream rng messageformatparser weakcache thread compression tuplemeta tuplehelpers
    # vsgopenmw-fixme(find-my-place)
    constants resourcehelpers osguservalues color
    )

#vsgopenmw-delete-me
#[[
add_component_dir (stereo
    frustum multiview stereomanager types
    )
]]

#vsgopenmw-delete-me
add_component_dir (debug
    #vsgopenmw-fixme(find-my-place)
    #debugdraw
    #vsgopenmw-deprecated
    #vsgopenmw-fixme(out-syntax)
    #vsgopenmw-fixme(stdio-compat)
    debuglog
    #vsgopenmw-fixme(global-state)
    #vsgopenmw-fixme(escape-pipe)
    #vsgopenmw-fixme(stdio-newlines)
    #debugging
    #vsgopenmw-delete-me
    #gldebug
    )

IF(NOT WIN32 AND NOT APPLE)
    add_definitions(-DGLOBAL_DATA_PATH="${GLOBAL_DATA_PATH}")
    add_definitions(-DGLOBAL_CONFIG_PATH="${GLOBAL_CONFIG_PATH}")
ENDIF()
add_component_dir (files
    #vsgopenmw-move-me(xdgpath)
    linuxpath androidpath windowspath macospath fixedpath multidircollection collections configurationmanager
    constrainedfilestream memorystream hash configfileparser openfile constrainedfilestreambuf conversion
    )

add_component_dir (compiler
    context controlparser errorhandler exception exprparser extensions fileparser generator
    lineparser literals locals output parser scanner scriptparser skipparser streamerrorhandler
    stringparser tokenloc nullerrorhandler opcodes extensions0 declarationparser
    quickfileparser discardparser junkparser
    )

add_component_dir (interpreter
    context controlopcodes genericopcodes installopcodes interpreter localopcodes mathopcodes
    miscopcodes opcodes runtime types defines
    )

add_component_dir (translation
    translation
    )

#vsgopenmw-delete-me
add_component_dir (loadinglistener
    loadinglistener
    )

#vsgopenmw-move-me(components/mygui)
add_component_dir (widgets
    box fontwrapper imagebutton tags list numericeditbox sharedstatebutton windowcaption widgets
    )

#vsgopenmw-move-me(components/mygui)
add_component_dir (fontloader
    fontloader
    )

#<vgopenmw-code-style="pending">
#vsgopenmw-move-me(components/sdl)
# Wraps SDL.
add_component_dir (sdlutil
    init inputwrapper videowrapper events cursormanager sdlmappings
    #vsgopenmw-delete-me
    #imagetosurface sdlvideowrapper
    )
#</vsgopenmw-code-style>

add_component_dir (version
    version
    )

add_component_dir (fallback
    fallback validate
    )

add_component_dir (lua_ui
    registerscriptsettings scriptsettings
    properties widget element util layers content alignment resources
    adapter text textedit window image container flex
    )
copy_resource_file("lua_ui/content.lua" "${OPENMW_RESOURCES_ROOT}" "resources/lua_libs/content.lua")


#vsgopenmw-delete-me
if(WIN32)
    add_component_dir (crashcatcher
        windows_crashcatcher
        windows_crashmonitor
        windows_crashshm
    )
elseif(NOT ANDROID)
    add_component_dir (crashcatcher
        crashcatcher
    )
endif()

add_component_dir(detournavigator
    debug
    makenavmesh
    findsmoothpath
    recastmeshbuilder
    navmeshmanager
    navigatorimpl
    asyncnavmeshupdater
    recastmesh
    tilecachedrecastmeshmanager
    recastmeshobject
    navmeshtilescache
    settings
    navigator
    findrandompointaroundcircle
    raycast
    navmeshtileview
    offmeshconnectionsmanager
    preparednavmeshdata
    navmeshcacheitem
    navigatorutils
    generatenavmeshtile
    navmeshdb
    serialization
    navmeshdbutils
    recast
    gettilespositions
    collisionshapetype
    stats
    commulativeaabb
    recastcontext
    )

#vsgopenmw-fixme(find-my-place)
add_component_dir(loadinglistener
    reporter
    )

add_component_dir(sqlite3
    db
    statement
    transaction
)

#[[
add_component_dir(esmloader
    load
    esmdata
)
#]]

#vsgopenmw-delete-me
add_component_dir(navmeshtool
    protocol
    )

add_component_dir(platform
    #vsgopenmw-delete-me
    #platform
    file
    )

if (WIN32)
#vsgopenmw-cleanup-platform-support
    add_component_dir(platform
        file.win32
        )
elseif (UNIX)
#vsgopenmw-cleanup-platform-support
    add_component_dir(platform
        file.posix
        )
else ()
    add_component_dir(platform
        file.stdio
        )
endif()

set (ESM_UI ${CMAKE_SOURCE_DIR}/files/ui/contentselector.ui
    )

if (USE_QT)
    add_component_qt_dir (contentselector
        model/modelitem model/esmfile
        model/naturalsort model/contentmodel
        model/loadordererror
        view/combobox view/contentselector
        )
    add_component_qt_dir (config
        gamesettings
        launchersettings
        )

    add_component_qt_dir (process
        processinvoker
        )

    add_component_qt_dir (misc
        helpviewer utf8qtextstream
        )

    add_component_qt_dir (files
        qtconversion
        qtconfigpath
        )

    QT_WRAP_UI(ESM_UI_HDR ${ESM_UI})
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64" AND NOT APPLE)
        add_definitions(-fPIC)
    endif()
endif ()

include_directories(${BULLET_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

find_package(SQLite3 REQUIRED)

add_library(components STATIC ${COMPONENT_FILES})

target_link_libraries(components
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_IOSTREAMS_LIBRARY}
    vsg::vsg
    ${SDL2_LIBRARIES}
    ${MyGUI_LIBRARIES}
    ${LUA_LIBRARIES}
    LZ4::LZ4
    RecastNavigation::DebugUtils
    RecastNavigation::Detour
    RecastNavigation::Recast
    SQLite::SQLite3
    #smhasher
    ${ICU_LIBRARIES}
    yaml-cpp
    )

if(Boost_VERSION_STRING VERSION_GREATER_EQUAL 1.77.0)
    target_link_libraries(components ${Boost_ATOMIC_LIBRARY})
endif()

target_link_libraries(components ${BULLET_LIBRARIES})

if (WIN32)
    target_link_libraries(components
    ${Boost_LOCALE_LIBRARY}
    ${Boost_ZLIB_LIBRARY})
endif()

if (USE_QT)
    add_library(components_qt STATIC ${COMPONENT_QT_FILES} ${ESM_UI_HDR})
    target_link_libraries(components_qt components Qt::Widgets Qt::Core)
    target_compile_definitions(components_qt PRIVATE OPENMW_DOC_BASEURL="${OPENMW_DOC_BASEURL}")

    if (BUILD_WITH_CODE_COVERAGE)
        target_compile_options(components_qt PRIVATE --coverage)
        target_link_libraries(components_qt gcov)
    endif()
endif()

if (GIT_CHECKOUT)
    add_dependencies (components git-version)
endif (GIT_CHECKOUT)

if (OSG_STATIC AND CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(X11 REQUIRED COMPONENTS Xinerama Xrandr)
    target_link_libraries(components ${CMAKE_DL_LIBS} X11::X11 X11::Xinerama X11::Xrandr)
    find_package(Fontconfig MODULE)
    if(Fontconfig_FOUND)
        target_link_libraries(components Fontconfig::Fontconfig)
    endif()
endif()

if (WIN32)
    target_link_libraries(components shlwapi)
endif()

# Fix for not visible pthreads functions for linker with glibc 2.15
if (UNIX AND NOT APPLE)
    target_link_libraries(components ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_WITH_CODE_COVERAGE)
    target_compile_options(components PRIVATE --coverage)
    target_link_libraries(components gcov)
endif()


# Make the variable accessible for other subdirectories
set(COMPONENT_FILES ${COMPONENT_FILES} PARENT_SCOPE)

target_compile_definitions(components PUBLIC BT_USE_DOUBLE_PRECISION)

if(OSG_STATIC)
    unset(_osg_plugins_static_files)
    add_library(components_osg_plugins INTERFACE)
    foreach(_plugin ${USED_OSG_PLUGINS})
        string(TOUPPER ${_plugin} _plugin_uc)
        if(OPENMW_USE_SYSTEM_OSG)
            list(APPEND _osg_plugins_static_files ${${_plugin_uc}_LIBRARY})
        else()
            list(APPEND _osg_plugins_static_files $<TARGET_FILE:${${_plugin_uc}_LIBRARY}>)
            target_link_libraries(components_osg_plugins INTERFACE $<TARGET_PROPERTY:${${_plugin_uc}_LIBRARY},LINK_LIBRARIES>)
            add_dependencies(components_osg_plugins ${${_plugin_uc}_LIBRARY})
        endif()
    endforeach()
    # We use --whole-archive because OSG plugins use registration.
    get_whole_archive_options(_opts ${_osg_plugins_static_files})
    target_link_options(components_osg_plugins INTERFACE ${_opts})
    target_link_libraries(components components_osg_plugins)

    if(OPENMW_USE_SYSTEM_OSG)
        # OSG plugin pkgconfig files are missing these dependencies.
        # https://github.com/openscenegraph/OpenSceneGraph/issues/1052
        find_package(Freetype REQUIRED)
        find_package(JPEG REQUIRED)
        find_package(PNG REQUIRED)
        target_link_libraries(components Freetype::Freetype JPEG::JPEG PNG::PNG)
    endif()
endif(OSG_STATIC)

if(USE_QT)
    set_property(TARGET components_qt PROPERTY AUTOMOC ON)
endif(USE_QT)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.16 AND MSVC)
    target_precompile_headers(components PUBLIC
        <sol/sol.hpp>

        <osg/State>
        <osg/StateSet>
        <osg/Node>
        <osg/Drawable>
        <osg/Camera>

        <MyGUI_Widget.h>

        <boost/filesystem.hpp>

        <algorithm>
        <filesystem>
        <fstream>
        <functional>
        <memory>
        <ostream>
        <string>
        <vector>
    )

    target_precompile_headers(components PRIVATE
        <osgViewer/Viewer>

        <boost/program_options/options_description.hpp>
    )
endif()
