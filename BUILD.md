
Buuilding using both CMake and Meson are supported.

The library is not single header, but should be easy to add to your own build system as well, just see the meson build
file as an example.

SDL3 is needed for the debug UI.  Meson will automatically build it or download binaries when building the UI.


Packages:

    libalphawire - static or shared object library (for use in your program)
    alphawireui  - IMGUI + SDL3 UI for controlling single camera (uses libalphawire)
    pyawire      - Python bindings for alphawire (directly builds alphawire sources into the python bindings


libalphawire and alphawireui are built using meson / cmake.

pyawire is built using the Python build module, see [README.md](pyawire/README.md).


Ubuntu
===

Packages needed:

    sudo apt install build-essential pkg-config
    sudo apt install libusb-1.0-0-dev libudev-dev libdbus-1-dev 


Only if building with CMake:

    sudo apt install cmake


Meson
===

Meson is a build system written in Python and installs as a Python package, so you may want to setup a separate
environment for it e.g. using conda or virtualenv.

Install meson and ninja:

    pip install meson ninja


Debug build:

    meson setup builddir
    cd builddir
    ninja


Release build:

    meson setup builddir --buildtype=release
    cd builddir
    ninja


CMake
===

Install CMake.

Debug build:

    cmake -S . -B cmake-build
    cmake --build cmake-build --config Debug

Release build:

    cmake -S . -B cmake-build
    cmake --build cmake-build --config Release


Alphawire Development
===

For building the main library and example UI using meson and for building the Python binding, setup a conda environment.

First download Miniconda and install it.

Then create a conda environment:

    conda create -n alphawire python=3.10
    conda activate alphawire
    pip install meson ninja

Whenever you work on alphawire you can then activate the alphawire environment.

For building the Python bindings see: [README.md](pyawire/README.md).
