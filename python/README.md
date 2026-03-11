# occ_gordon Python Bindings

This folder builds the `occ_gordon` Python extension module against `pythonocc-core`.

## Prerequisites

- Python environment with:
  - `pythonocc-core`
  - `opencascade`
  - `cmake`
  - `scikit-build-core`
- A C++ toolchain supported by CMake (MSVC on Windows).
- `swig` executable.

## Important: SWIG runtime ABI must match pythonocc wrappers

`pythonocc-core` is generated with a specific SWIG runtime ABI (`swig_runtime_dataN`).
`occ_gordon` must be built with a SWIG executable that uses the **same runtime ABI**,
otherwise runtime type conversion fails (for example when passing `Geom_Curve` objects).

This project checks ABI compatibility at CMake configure time and fails on mismatch.
Exact SWIG version equality is optional.

To see the required version in your environment:

```bash
python -c "import OCC.Core.Geom as G, pathlib, re; t=pathlib.Path(G.__file__).read_text(encoding='utf-8', errors='ignore'); print(re.search(r'^# Version ([0-9]+\\.[0-9]+\\.[0-9]+)', t, re.M).group(1))"
```

## Source of `pythonocc-core` SWIG files

Conda packages may not include SWIG interface sources (`src/SWIG_files/...`).
`occ_gordon` can fetch matching `pythonocc-core` sources automatically (source-only, no subproject build).

Default behavior:
- If `PythonOCC_SOURCE_DIR` is not set, CMake detects installed pythonocc version and fetches the matching tag from:
  - `https://github.com/tpaviot/pythonocc-core.git`

## CMake options (python/CMakeLists.txt)

- `PythonOCC_SOURCE_DIR`  
  Local source tree containing `src/SWIG_files/wrapper/Standard.i`.
- `OCC_GORDON_FETCH_PYTHONOCC_SOURCE` (`ON` by default)  
  Auto-fetch pythonocc source if `PythonOCC_SOURCE_DIR` is missing.
- `OCC_GORDON_PYTHONOCC_REPOSITORY`  
  Override pythonocc git URL.
- `OCC_GORDON_PYTHONOCC_GIT_TAG`  
  Override fetched git tag (default: installed pythonocc version).
- `OCC_GORDON_SKIP_SWIG_VERSION_CHECK` (`OFF` by default)  
  Bypass SWIG compatibility check (not recommended).
- `OCC_GORDON_ENFORCE_EXACT_SWIG_VERSION` (`OFF` by default)  
  Also require exact SWIG version equality (stricter than ABI check).

## Build example (CMake)

From repository root:

```bash
cmake -S python -B python/build/dev ^
  -DPython3_EXECUTABLE=<path-to-python> ^
  -DOpenCASCADE_DIR=<path-to-opencascade-config-dir> ^
  -DSWIG_EXECUTABLE=<path-to-swig>

cmake --build python/build/dev --config Release
```

The extension module is created as:
- `python/build/dev/Release/_occ_gordon_native.pyd` (Windows)

## Build example (pip / scikit-build-core)

From `python/`:

```bash
python -m pip install -v --no-build-isolation .
```

If needed, pass CMake settings via `CMAKE_ARGS`, for example:

```bash
set CMAKE_ARGS=-DSWIG_EXECUTABLE=<path-to-swig>
python -m pip install -v --no-build-isolation .
```

## Smoke test

```bash
python examples/create_wing.py
```

Expected behavior: interpolation runs, then a viewer window opens.
