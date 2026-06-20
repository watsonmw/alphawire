
To build this package, install dependencies:

    pip install --upgrade build twine

Build:

    python -m build
    
Audit:
    
    auditwheel repair dist/awire-*.whl

Compatibility:

The package is built using the Python Stable ABI (Limited API), meaning a single wheel
should work on all CPython versions from 3.7 onwards for a given platform.
For Apple Silicon (arm64), the wheel is tagged with `macosx_11_0_arm64` for maximum compatibility
across macOS 11 and newer.

Install:

    pip install dist/pyawire-0.1.0-*.whl --force-reinstall

Upload:

    twine upload dist/*