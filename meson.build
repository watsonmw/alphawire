project('alphawire', ['c', 'cpp'],
  version : '0.1',
  default_options : ['c_std=c17', 'cpp_std=c++20']
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')
cmake = import('cmake')

# Common definitions
add_project_arguments('-DM_USE_SDL', '-DM_USE_STDLIB', '-DAW_LOG_LEVEL=3', language : ['c', 'cpp'])

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', '-DM_MEM_DEBUG', '-DM_ASSERT', '-DM_STACKTRACE', '-DM_THREADING', language : ['c', 'cpp'])
endif

inc = [include_directories('src')]

alphawire_sources = files(
  'src/mlib/mlib-file-stdlib.c',
  'src/mlib/mlib-log-stdlib.c',
  'src/mlib/mlib.c',
  'src/mlib/utf8.c',
  'src/aw/aw-backend.c',
  'src/aw/aw-control.c',
  'src/aw/aw-device-list.c',
  'src/aw/aw-log.c',
  'src/aw/aw-util.c',
  'src/aw/platform/usb-const.c'
)

ptp_ip_sources = files(
  'src/mlib/msock.c',
  'src/mlib/mxml.c',
  'src/aw/platform/ip/http-client.c',
  'src/aw/platform/ip/aw-backend-ip.c'
)

alphawire_deps = []
alphawire_c_args = ['-DALPHAWIRE_BUILDING_SHARED_LIB']

host_system = host_machine.system()
message('Host system:', host_system)

if host_system == 'darwin'
  alphawire_sources += files(
    'src/aw/platform/osx/aw-backend-iokit.c'
  )
  alphawire_sources += ptp_ip_sources
  alphawire_c_args += ['-DAW_ENABLE_IOKIT', '-DAW_ENABLE_IP']
  add_project_arguments('-DM_PTHREADS', language : ['c', 'cpp'])
  
  alphawire_deps += dependency('appleframeworks', modules : ['Cocoa', 'IOKit'])

elif host_system == 'windows'
  alphawire_sources += files(
    'src/aw/platform/windows/aw-backend-libusbk.c',
    'src/aw/platform/windows/aw-backend-wia.c',
    'src/aw/platform/windows/win-utils.c'
  )
  alphawire_sources += ptp_ip_sources
  alphawire_c_args += ['-DAW_ENABLE_LIBUSBK', '-DAW_ENABLE_WIA', '-DAW_ENABLE_IP', '-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0600']
  
  inc += include_directories('libs/libusbk')
  
  alphawire_deps += cc.find_library('ws2_32')
  alphawire_deps += cc.find_library('Iphlpapi')
  alphawire_deps += cc.find_library('wiaguid')
  alphawire_deps += cc.find_library('dbghelp')
  
  # libusbK library path depends on architecture. 
  # For now, let's assume amd64 as per CMakeLists.txt
  libusbk_lib = cc.find_library('libusbK', dirs : meson.current_source_dir() / 'libs/libusbk/amd64')
  alphawire_deps += libusbk_lib

  if cc.get_id() == 'gcc' # MinGW
    inc += include_directories('libs/mingw')
    alphawire_deps += cc.find_library('uuid', dirs : meson.current_source_dir() / 'libs/mingw')
  endif

elif host_system == 'linux'
  alphawire_sources += files(
    'src/aw/platform/libusb/aw-backend-libusb.c'
  )
  alphawire_sources += ptp_ip_sources
  alphawire_deps += dependency('libusb-1.0')
  alphawire_c_args += ['-DAW_ENABLE_LIBUSB', '-DAW_ENABLE_IP']
  add_project_arguments('-DM_PTHREADS', language : ['c', 'cpp'])
endif

alphawire_lib = shared_library('alphawire',
  alphawire_sources,
  include_directories : inc,
  dependencies : alphawire_deps,
  c_args : alphawire_c_args,
  install : true
)

# UI Application
alphawireui_sources = files(
  'src/imgui/imgui.cpp',
  'src/imgui/imgui_demo.cpp',
  'src/imgui/imgui_draw.cpp',
  'src/imgui/imgui_impl_opengl3.cpp',
  'src/imgui/imgui_impl_sdl3.cpp',
  'src/imgui/imgui_tables.cpp',
  'src/imgui/imgui_widgets.cpp',
  'src/mlib/marena.c',
  'src/mlib/mlib-file-stdlib.c',
  'src/mlib/mlib-log-stdlib.c',
  'src/mlib/mlib.c',
  'src/mlib/utf8.c',
  'src/ui/ui-live-view.cpp',
  'src/ui/ui-main.cpp',
  'src/ui/ui-ptp-debug.cpp'
)

ui_inc = include_directories('src', 'src/imgui')

sdl3_dep = dependency('sdl3', required : false)
if not sdl3_dep.found()
  # Use the same version as CMakeLists.txt
  sdl3_proj = cmake.subproject('sdl3', cmake_options: ['-DSDL_STATIC=OFF', '-DSDL_SHARED=ON'])
  sdl3_dep = sdl3_proj.dependency('SDL3-shared')
endif
ui_deps = [sdl3_dep]

ui_args = ['-DSDL_MAIN_USE_CALLBACKS']

if host_system == 'darwin'
  ui_deps += dependency('appleframeworks', modules : ['Cocoa', 'IOKit', 'OpenGL'])
elif host_system == 'windows'
  ui_deps += cc.find_library('opengl32')
  ui_args += ['-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0600', '-DAW_ENABLE_LIBUSBK', '-DAW_ENABLE_WIA', '-DAW_ENABLE_IP']
elif host_system == 'linux'
  ui_deps += dependency('gl')
endif

executable('alphawireui',
  alphawireui_sources,
  include_directories : ui_inc,
  dependencies : ui_deps,
  c_args : ui_args,
  cpp_args : ui_args,
  link_with : alphawire_lib,
  install : true,
  win_subsystem : 'console'
)
