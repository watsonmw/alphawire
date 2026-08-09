# Junie Instructions for AlphaWire

This document contains instructions for Junie to interact with the AlphaWire project, specifically the Python `awire` wrapper.

## Python Environment

The project uses a Conda environment named `alphawire`. 
Always ensure this environment is activated before running any Python-related commands or scripts for `pyawire`.

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
    
    # Install in editable mode
    pip install -e .
    ```

3.  **Running Examples**:
    To run the capture example:
    ```
    python examples\capture.py
    ```

4.  **Running the `awire` module**:
    To run the main entry point (if applicable):
    ```
    python -m awire
    ```

## Notes
- The package is named `pyawire` for installation but the module is `awire`.
- On Windows conda is installed in "C:\Users\<username>\miniconda3\condabin\conda.bat".