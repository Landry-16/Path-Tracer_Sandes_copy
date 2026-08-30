##
## EPITECH PROJECT, 2026
## Raytracer
## File description:
## CMakeLists
##

cmake_minimum_required(VERSION 3.20)

project(raytracer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# Output binaries and shared libraries to the repo root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/plugins)


find_package(SFML 2 REQUIRED COMPONENTS graphics window system)
find_library(LIBCONFIG_LIB config++ REQUIRED)

# core binary
file(GLOB_RECURSE SRC_RAYTRACER CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM SRC_RAYTRACER ${CMAKE_SOURCE_DIR}/src/denoise_tool.cpp)

add_executable(${PROJECT_NAME} ${SRC_RAYTRACER})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${LIBCONFIG_LIB}
    dl
    pthread
    sfml-graphics
    sfml-window
    sfml-system
)
target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)

# denoise tool binary
add_executable(denoise_tool
    src/denoise_tool.cpp
    src/image/PPMWriter.cpp
    src/math/Vec3.cpp
)
target_include_directories(denoise_tool PRIVATE include)
target_link_libraries(denoise_tool PRIVATE dl)

#PLUGINS
macro(add_plugin PLUGIN_NAME)
    cmake_parse_arguments(P "" "" "SRCS;INCS;LIBS" ${ARGN})
    add_library(raytracer_${PLUGIN_NAME} SHARED ${P_SRCS})
    set_target_properties(raytracer_${PLUGIN_NAME} PROPERTIES PREFIX "")
    target_include_directories(raytracer_${PLUGIN_NAME} PRIVATE
        include
        plugins/src/include
        ${P_INCS}
    )
    if(P_LIBS)
        target_link_libraries(raytracer_${PLUGIN_NAME} PRIVATE ${P_LIBS})
    endif()
endmacro()


foreach(CATEGORY primitives materials textures)
    file(GLOB CATEGORY_SRCS CONFIGURE_DEPENDS plugins/src/${CATEGORY}/*.cpp)
    foreach(SRC ${CATEGORY_SRCS})
        get_filename_component(PLUGIN_NAME ${SRC} NAME_WE)
        add_plugin(${PLUGIN_NAME} SRCS ${SRC})
    endforeach()
endforeach()


file(GLOB OBJ_LOADER_SRCS CONFIGURE_DEPENDS plugins/src/ObjLoader/*.cpp)
add_plugin(obj_loader
    SRCS ${OBJ_LOADER_SRCS}
    INCS plugins/include
)

file(GLOB STL_LOADER_SRCS CONFIGURE_DEPENDS plugins/src/StlLoader/*.cpp)
add_plugin(stl_loader
    SRCS ${STL_LOADER_SRCS}
    INCS plugins/include
)

# Denoisers
set(OIDN_ROOT ${CMAKE_SOURCE_DIR}/plugins/denoiser/oidn)

add_plugin(simple_denoiser
    SRCS plugins/src/denoisers/simple_denoiser.cpp
         src/math/Vec3.cpp
)

add_plugin(oidn_denoiser
    SRCS plugins/src/denoisers/oidn_denoiser.cpp
         src/math/Vec3.cpp
    INCS ${OIDN_ROOT}/include
    LIBS ${OIDN_ROOT}/lib/libOpenImageDenoise.so
)
target_link_options(raytracer_oidn_denoiser PRIVATE
    -Wl,-rpath,$ORIGIN/denoiser/oidn/lib
)

# IMGUI
# imgui has no CMakeLists, FetchContent just downloads the sources.
# imgui-sfml v2.6 has a CMakeLists that compiles imgui + the SFML backend
# into a single static ImGui-SFML library (BUILD_SHARED_LIBS=OFF).
# That static lib is then linked into our SHARED plugin .so, so no extra

include(FetchContent)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

# Disable imgui-sfml's internal find_package(SFML) cause SFML is already found
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)

FetchContent_Declare(imgui_sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG v2.6.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui_sfml)

# Wire SFML manually so imgui-SFML.cpp can find SFML headers and our
# plugin gets SFML through ImGui-SFML::ImGui-SFML.
target_link_libraries(ImGui-SFML PUBLIC sfml-graphics sfml-window sfml-system)
# ImGui-SFML is a static lib that will be linked into a SHARED plugin, so need PIC
set_target_properties(ImGui-SFML PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(raytracer_imgui_overlay SHARED
    plugins/src/ui/ImGuiOverlayConfig.cpp
    plugins/src/ui/ImGuiOverlay.cpp
    plugins/src/ui/ImGuiOverlayScene.cpp
    plugins/src/ui/ImGuiOverlayInspector.cpp
    plugins/src/ui/ImGuiOverlayLights.cpp
    plugins/src/ui/ImGuiOverlayCamera.cpp
    plugins/src/ui/ImGuiOverlayModals.cpp
)
set_target_properties(raytracer_imgui_overlay PROPERTIES PREFIX "")
target_include_directories(raytracer_imgui_overlay PRIVATE include plugins/src/ui)
target_link_libraries(raytracer_imgui_overlay PRIVATE
    ImGui-SFML::ImGui-SFML
    sfml-graphics
    sfml-window
    sfml-system
    ${LIBCONFIG_LIB}
)