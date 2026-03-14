cmake_minimum_required(VERSION 3.6)

project(MaelstromAndroid)

# Project root is three levels up from android/app/jni/
set(MAELSTROM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

# SDL3
add_subdirectory("${MAELSTROM_ROOT}/external/SDL" SDL_build EXCLUDE_FROM_ALL)

# SDL_net (static)
block()
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory("${MAELSTROM_ROOT}/external/SDL_net" SDL_net_build EXCLUDE_FROM_ALL)
endblock()

# maclib (Mac resource library — platform-independent despite the name)
add_subdirectory("${MAELSTROM_ROOT}/maclib" maclib_build)

# Game sources — same list as the root CMakeLists.txt minus Maelstrom.rc (Windows resource file)
set(MAELSTROM_SOURCES
    ${MAELSTROM_ROOT}/game/about.cpp
    ${MAELSTROM_ROOT}/game/continue.cpp
    ${MAELSTROM_ROOT}/game/controls.cpp
    ${MAELSTROM_ROOT}/game/fastrand.cpp
    ${MAELSTROM_ROOT}/game/game.cpp
    ${MAELSTROM_ROOT}/game/gameinfo.cpp
    ${MAELSTROM_ROOT}/game/gameover.cpp
    ${MAELSTROM_ROOT}/game/init.cpp
    ${MAELSTROM_ROOT}/game/load.cpp
    ${MAELSTROM_ROOT}/game/lobby.cpp
    ${MAELSTROM_ROOT}/game/MacDialog.cpp
    ${MAELSTROM_ROOT}/game/MaelstromUI.cpp
    ${MAELSTROM_ROOT}/game/main.cpp
    ${MAELSTROM_ROOT}/game/make.cpp
    ${MAELSTROM_ROOT}/game/myerror.cpp
    ${MAELSTROM_ROOT}/game/netplay.cpp
    ${MAELSTROM_ROOT}/game/object.cpp
    ${MAELSTROM_ROOT}/game/objects.cpp
    ${MAELSTROM_ROOT}/game/particles.cpp
    ${MAELSTROM_ROOT}/game/player.cpp
    ${MAELSTROM_ROOT}/game/rect.cpp
    ${MAELSTROM_ROOT}/game/replay.cpp
    ${MAELSTROM_ROOT}/game/scores.cpp
    ${MAELSTROM_ROOT}/game/starfield.cpp
    ${MAELSTROM_ROOT}/game/steam.cpp

    ${MAELSTROM_ROOT}/screenlib/SDL_FrameBuf.cpp
    ${MAELSTROM_ROOT}/screenlib/UIArea.cpp
    ${MAELSTROM_ROOT}/screenlib/UIBaseElement.cpp
    ${MAELSTROM_ROOT}/screenlib/UIContainer.cpp
    ${MAELSTROM_ROOT}/screenlib/UIDialogButton.cpp
    ${MAELSTROM_ROOT}/screenlib/UIDialog.cpp
    ${MAELSTROM_ROOT}/screenlib/UIDrawEngine.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElementButton.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElementCheckbox.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElement.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElementDropdown.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElementEditbox.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElementRadio.cpp
    ${MAELSTROM_ROOT}/screenlib/UIElementThumbstick.cpp
    ${MAELSTROM_ROOT}/screenlib/UIManager.cpp
    ${MAELSTROM_ROOT}/screenlib/UIPanel.cpp
    ${MAELSTROM_ROOT}/screenlib/UITemplates.cpp
    ${MAELSTROM_ROOT}/screenlib/UITexture.cpp

    ${MAELSTROM_ROOT}/utils/files.c
    ${MAELSTROM_ROOT}/utils/hashtable.c
    ${MAELSTROM_ROOT}/utils/loadxml.cpp
    ${MAELSTROM_ROOT}/utils/prefs.cpp

    ${MAELSTROM_ROOT}/miniz/miniz.c
)

# Build as SHARED library (required for Android JNI loading)
add_library(main SHARED ${MAELSTROM_SOURCES})

target_compile_options(main PRIVATE -Wall -Wextra
    -Wno-cast-function-type -Wno-sign-compare
    -Wno-unused-function -Wno-unused-parameter)

# Android has getifaddrs since API 24
if(ANDROID_NATIVE_API_LEVEL GREATER_EQUAL 24)
    target_compile_definitions(main PRIVATE HAVE_GETIFADDRS)
endif()

target_link_libraries(main PRIVATE SDLmac)
target_link_libraries(main PRIVATE SDL3::SDL3)
target_link_libraries(main PRIVATE SDL3_net::SDL3_net-static)
