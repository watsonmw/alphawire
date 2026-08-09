
Install dependencies:

    pip install --upgrade build twine

Build:

    python -m build

Install:

    pip install dist/pyawire-0.1.0-*.whl --force-reinstall


Compatibility:

The package is built using the Python Stable ABI (Limited API), meaning a single wheel
should work on all CPython versions from 3.7 onwards for a given platform.
For Apple Silicon (arm64), the wheel is tagged with `macosx_11_0_arm64` for maximum compatibility
across macOS 11 and newer.

Upload:

    twine upload dist/*
    
Development & Debugging:

    # Remove current awire installation
    pip uninstall pyawire
    
    # Install in development mode, you can edit the source code and see changes immediately
    pip install -e .

    # Build the c extension shared library as needed
    python build_extension.py --debug

    # To support IDE development you can build only if the shared library dependant source files change (i.e. cffi)
    # python build_extension.py --debug --only-if-changed

    # Run tester
    python examples\tester.py
