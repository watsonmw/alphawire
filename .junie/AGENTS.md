# Junie Instructions for AlphaWire

## Python Environment

During development use the Miniconda environment named `alphawire`.   This has meson, ninja and dependencies needed for
building the Python binding.

Activate this environment before running any Python-related commands or scripts for `pyawire`.

To activate the environment:
```
conda activate alphawire
```

## Running the `awire` Wrapper

When working with the `pyawire` component:

1.  **Navigate to the `pyawire` directory**:
    ```
    cd pyawire
    ```

2.  **Development Setup**:
    If you need to build or install the wrapper in development mode:
    ```
    # Build extension (if needed)
    python build_extension.py --only-if-changed
    
    # Install in editable mode (typically already done)
    pip install -e .
    ```

3.  **Running Examples**:
    To run the capture example:
    ```
    python examples\capture.py
    ```


## Notes
- The package is named `pyawire` for installation but the module is `awire`
- On Windows conda is installed in "C:\Users\<username>\miniconda3\condabin\conda.bat"
- On OSX conda is installed in "$HOME\miniconda3\condabin\conda"
- On Linux conda is installed in "$HOME\miniconda3\condabin\conda"