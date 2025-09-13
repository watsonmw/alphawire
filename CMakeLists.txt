cmake_minimum_required(VERSION 3.30)
project(alphawire)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)

# When building in debug add our extra debug flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DM_MEM_DEBUG -DM_ASSERT -DM_STACKTRACE")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DM_MEM_DEBUG -DM_ASSERT -DM_STACKTRACE")

##########################################
# Alphawire shared library
##########################################

add_library(alphawire SHARED
    src/mlib/mlib-file-stdlib.c
    src/mlib/mlib-log-stdlib.c
    src/mlib/mlib.c
    src/mlib/utf.c
    src/mlib/utf.h
    src/ptp/ptp-backend.c
    src/ptp/ptp-backend.h
    src/ptp/ptp-const.h
    src/ptp/ptp-control.c
    src/ptp/ptp-control.h
    src/ptp/ptp-device-list.c
    src/ptp/ptp-device-list.h
    src/ptp/ptp-log.c
    src/ptp/ptp-log.h
    src/ptp/ptp-util.c
    src/ptp/ptp-util.h
    src/platform/usb-const.c
    src/platform/usb-const.h
)

target_compile_definitions(alphawire PRIVATE M_USE_SDL M_USE_STDLIB PTP_LOG_LEVEL=3 ALPHAWIRE_BUILDING_SHARED_LIB)
target_include_directories(alphawire PUBLIC src)
target_compile_features(alphawire PUBLIC cxx_std_17)

if(APPLE)
    target_sources(alphawire PRIVATE
        src/platform/osx/ptp-backend-iokit.c
        src/platform/osx/ptp-backend-iokit.h
    )

    target_compile_definitions(alphawire PRIVATE PTP_ENABLE_IOKIT)
    target_compile_options(alphawire PRIVATE -fvisibility=hidden)

    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    target_link_libraries(alphawire PUBLIC ${COCOA_LIBRARY} ${IOKIT_LIBRARY})
endif()

# MinGW and Visual Studio compiles on Windows
if(WIN32)
    # Add Windows-only sources
    target_sources(alphawire PRIVATE
        src/platform/windows/ptp-backend-libusbk.c
        src/platform/windows/ptp-backend-libusbk.h
        src/platform/windows/ptp-backend-wia.c
        src/platform/windows/ptp-backend-wia.h
        src/platform/windows/win-utils.c
        src/platform/windows/win-utils.h
    )

    # Configure backends
    target_compile_definitions(alphawire PRIVATE PTP_ENABLE_LIBUSBK PTP_ENABLE_WIA)

    # Set Windows version to compile to (needed to pick up WIA).
    target_compile_definitions(alphawire PRIVATE WINVER=0x0A00 _WIN32_WINNT=0x0600)

    # Configure libUSBK
    target_include_directories(alphawire PRIVATE libs/libusbk)
    target_link_libraries(alphawire PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/libusbk/amd64/libusbK.lib)

    # MinGW specific
    if (MINGW)
        # MinGW 13 headers are missing IWiaItemExtras, use our own copies of the MinGW 12 versions:
        target_include_directories(alphawire PRIVATE libs/mingw)
        target_link_libraries(alphawire mingw32 ${CMAKE_CURRENT_SOURCE_DIR}/libs/mingw/libuuid.a)

        target_compile_options(alphawire PRIVATE -ggdb -Wall)
        target_compile_options(alphawire PRIVATE -fvisibility=hidden)
        target_link_libraries(alphawire mingw32 -static-libgcc -static-libstdc++)
    else()
        # Configure WIA
        target_link_libraries(alphawire PUBLIC wiaguid.lib)
    endif()
endif()

##########################################
# Alphawire UI
##########################################

add_executable(alphawireui
    src/imgui/imgui.cpp
    src/imgui/imgui_demo.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_impl_opengl3.cpp
    src/imgui/imgui_impl_sdl3.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/mlib/mlib-file-stdlib.c
    src/mlib/mlib-log-stdlib.c
    src/mlib/mlib.c
    src/mlib/utf.c
    src/mlib/utf.h
    src/ui/ui-live-view.cpp
    src/ui/ui-main.cpp
    src/ui/ui-ptp-debug.cpp
)

target_compile_definitions(alphawireui PRIVATE M_USE_SDL M_USE_STDLIB)
target_include_directories(alphawireui PRIVATE src src/imgui)
target_compile_features(alphawireui PRIVATE cxx_std_17)

# Function for fetching external git project
include(FetchContent)
function(include_dependency libName gitURL gitTag)
    # setup the declare
    FetchContent_Declare(${libName}
        GIT_REPOSITORY ${gitURL}
        GIT_TAG        ${gitTag}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    FetchContent_MakeAvailable(${libName})
endfunction()

# SDL3 - Use local if available otherwise download and compile
find_package(SDL3 QUIET)
if (NOT SDL3_FOUND)
    message(STATUS "Getting SDL3 from Github")
    include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.2.16)
else()
    message(STATUS "Using local SDL3")
endif()

# Configure SDL
target_include_directories(alphawireui PUBLIC ${SDL3_INCLUDE_DIRS})
target_compile_definitions(alphawireui PRIVATE SDL_MAIN_USE_CALLBACKS)
target_link_libraries(alphawireui PRIVATE SDL3::SDL3)

# Link alphawire lib to ui
target_link_libraries(alphawireui PRIVATE alphawire)

if(APPLE)
    find_package(OpenGL REQUIRED)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    target_link_libraries(alphawireui PRIVATE ${COCOA_LIBRARY} ${OPENGL_LIBRARIES} ${IOKIT_LIBRARY})
endif()

# MinGW and Visual Studio compiles on Windows
if(WIN32)
    # Set Windows version to compile to (this just matches alphawire library - but probably doesnt need to)
    target_compile_definitions(alphawireui PRIVATE WINVER=0x0A00 _WIN32_WINNT=0x0600)

    # MinGW specific
    if (MINGW)
        target_compile_options(alphawireui PRIVATE -ggdb -Wall -mconsole)
        target_link_libraries(alphawireui mingw32 -static-libgcc -static-libstdc++)
    else()
        target_link_options(alphawireui PRIVATE /SUBSYSTEM:CONSOLE)
    endif()

    # ImGUI
    target_link_libraries(alphawireui opengl32)

    # Copy SDL binary to runtime directory
    add_custom_command(
        TARGET alphawireui POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:alphawireui>"
        VERBATIM
    )
endif()

# Copy to build directory, so alphawire-cffi-compile.py can pick up
add_custom_command(
        TARGET alphawire POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:alphawire>" "${CMAKE_CURRENT_SOURCE_DIR}/build"
        VERBATIM
)
