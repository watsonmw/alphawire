cmake_minimum_required(VERSION 3.30)
project(alphawire)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)

add_executable(alphawire WIN32
        src/imgui/imgui.cpp
        src/imgui/imgui_demo.cpp
        src/imgui/imgui_draw.cpp
        src/imgui/imgui_impl_opengl3.cpp
        src/imgui/imgui_impl_sdl3.cpp
        src/imgui/imgui_tables.cpp
        src/imgui/imgui_widgets.cpp
        src/mlib/mlib.c
        src/mlib/mlib-file-stdlib.c
        src/mlib/mlib-log-stdlib.c
        src/platform/windows/ptp-backend-wia.h
        src/platform/windows/ptp-backend-wia.c
        src/platform/windows/ptp-backend-libusbk.h
        src/platform/windows/ptp-backend-libusbk.c
        src/ui/ui-main.cpp
        src/ui/ui-ptp-debug.cpp
        src/ui/ui-live-view.cpp
        src/ptp/ptp-const.h
        src/ptp/ptp-control.h
        src/ptp/ptp-control.c
        src/ptp/ptp-device-list.h
        src/ptp/ptp-device-list.c
        src/ptp/utf.h
        src/ptp/utf.c
)

target_compile_definitions(alphawire PRIVATE -DM_USE_SDL -DM_USE_STDLIB)
target_include_directories(alphawire PRIVATE src src/imgui)
target_compile_options(alphawire PRIVATE -ggdb -Wall)
target_compile_features(alphawire PRIVATE cxx_std_17)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -DM_MEM_DEBUG -DM_ASSERT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DM_MEM_DEBUG -DM_ASSERT")

if(APPLE)
    find_library(SDL3_LIBRARY SDL3 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    target_link_libraries(alphawire ${COCOA_LIBRARY} ${SDL3_LIBRARY} ${OPENGL_LIBRARIES})
endif()

if(MINGW)
    # Include the command that downloads libraries
    include(FetchContent)

    # define a function for adding git dependencies
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

    find_package(SDL3 QUIET)
    if (NOT SDL3_FOUND)
        message(STATUS "Getting SDL3 from Github")
        include_dependency(SDL3 https://github.com/libsdl-org/SDL.git release-3.2.16)
    else()
        message(STATUS "Using local SDL3")
    endif()

    target_include_directories(alphawire PUBLIC ${SDL3_INCLUDE_DIRS})
    target_compile_definitions(alphawire PRIVATE SDL_MAIN_USE_CALLBACKS)

    target_include_directories(alphawire PRIVATE libs/libusbk)
    target_compile_options(alphawire PRIVATE -mconsole)
    target_link_libraries(alphawire mingw32 SDL3::SDL3 opengl32 -L${CMAKE_CURRENT_SOURCE_DIR}/libs/libusbk/amd64 -lusbK -static-libgcc -static-libstdc++)

    if (WIN32)
        add_custom_command(
                TARGET alphawire POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3>" "$<TARGET_FILE_DIR:alphawire>"
                VERBATIM
        )
    endif()
endif()
