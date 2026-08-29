project('alphawire', ['c', 'cpp'],
  version : '0.1',
  default_options : [
    'c_std=gnu17', # meson will fallback to c17 if gcc/clang is not the compiler being used
    'cpp_std=c++20',
  ]
)

cc = meson.get_compiler('c')
cpp = meson.get_compiler('cpp')

debug_args = []
if get_option('buildtype').startswith('debug')
  debug_args += ['-DDEBUG', '-DM_MEM_DEBUG', '-DM_ASSERT', '-DM_STACKTRACE']
endif

# Internal args used when building alphawire lib
alphawire_c_args = debug_args
# External args exported by alphawire lib when including it into a project
alphawire_lib_args = ['-DM_THREADING', '-DM_USE_STDLIB', '-DAW_LOG_LEVEL=3']
alphawire_lib_deps = []

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


host_system = host_machine.system()
message('Host system:', host_system)

if host_system == 'darwin'
  alphawire_sources += files(
    'src/aw/platform/osx/aw-backend-iokit.c'
  )
  alphawire_sources += ptp_ip_sources
  alphawire_c_args += ['-DAW_ENABLE_IOKIT', '-DAW_ENABLE_IP']
  alphawire_lib_args += ['-DM_PTHREADS']
  alphawire_lib_deps += dependency('appleframeworks', modules : ['Cocoa', 'IOKit'])

elif host_system == 'windows'
  alphawire_sources += files(
    'src/aw/platform/windows/aw-backend-libusbk.c',
    'src/aw/platform/windows/aw-backend-wia.c',
    'src/aw/platform/windows/win-utils.c'
  )
  alphawire_sources += ptp_ip_sources
  alphawire_c_args += ['-DAW_ENABLE_LIBUSBK', '-DAW_ENABLE_WIA', '-DAW_ENABLE_IP', '-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0600']
  
  inc += include_directories('libs/libusbk')
  
  alphawire_lib_deps += cc.find_library('ws2_32')
  alphawire_lib_deps += cc.find_library('Iphlpapi')
  alphawire_lib_deps += cc.find_library('wiaguid')
  alphawire_lib_deps += cc.find_library('dbghelp')
  
  # libusbK library path depends on architecture. 
  # For now, let's assume amd64 as per CMakeLists.txt
  libusbk_lib = cc.find_library('libusbK', dirs : meson.current_source_dir() / 'libs/libusbk/amd64')
  alphawire_lib_deps += libusbk_lib

  if cc.get_id() == 'gcc' # MinGW
    inc += include_directories('libs/mingw')
    alphawire_lib_deps += cc.find_library('uuid', dirs : meson.current_source_dir() / 'libs/mingw')
  endif

elif host_system == 'linux'
  alphawire_sources += files(
    'src/aw/platform/libusb/aw-backend-libusb.c'
  )
  alphawire_sources += ptp_ip_sources
  alphawire_lib_deps += dependency('libusb-1.0')
  alphawire_c_args += ['-DAW_ENABLE_LIBUSB', '-DAW_ENABLE_IP']
  alphawire_lib_args += ['-DM_PTHREADS']
endif

alphawire_lib = both_libraries('alphawire',
  alphawire_sources,
  include_directories : inc,
  dependencies : alphawire_lib_deps,
  c_args : alphawire_c_args + alphawire_lib_args,
  c_shared_args : ['-DALPHAWIRE_BUILDING_SHARED_LIB'],
)

alphawire_lib_dep = declare_dependency(
  include_directories : inc,
  link_with : alphawire_lib,
  dependencies : alphawire_lib_deps,
  compile_args : alphawire_lib_args
)

# UI Application
ui_sources = files(
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

sdl3_dep = dependency('sdl3', required : true, static: true, fallback: ['sdl3', 'sdl3_dep'])

ui_inc = include_directories('src', 'src/imgui')
ui_deps = [sdl3_dep]
ui_args = ['-DSDL_MAIN_USE_CALLBACKS']
ui_args += debug_args

if host_system == 'darwin'
  ui_deps += dependency('appleframeworks', modules : ['Cocoa', 'IOKit', 'OpenGL'])
elif host_system == 'windows'
  ui_deps += cc.find_library('opengl32')
  ui_args += ['-DWINVER=0x0A00', '-D_WIN32_WINNT=0x0600', '-DAW_ENABLE_LIBUSBK', '-DAW_ENABLE_WIA', '-DAW_ENABLE_IP']
elif host_system == 'linux'
  ui_deps += dependency('gl')
endif

executable('alphawireui',
  ui_sources,
  include_directories : ui_inc,
  dependencies : ui_deps + [alphawire_lib_dep],
  link_with : alphawire_lib.get_static_lib(),
  c_args : ui_args,
  cpp_args : ui_args,
  install : true,
  win_subsystem : 'console'
)
