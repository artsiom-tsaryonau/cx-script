# Architecture

How `cx` turns a script into a running binary.

## Pipeline

```text
script.c / script.cpp
    │
    ├─ warm?  (.cx_stamp == sha256(script) && binary exists)
    │         → exec cached binary
    │
    ├─ parse //DEPS lines
    │     conan:…  → Conan requires (+ [options])
    │     vcpkg:…  → vcpkg.json dependencies (+ features / version overrides)
    │     gh:…/…/ref           → FetchContent (CMake)
    │     gh:…/…/ref/path/…    → curl raw file(s)
    │
    ├─ strip shebang → copy as script.c / script.cpp in cache
    │
    ├─ if any deps: generate CMakeLists.txt
    │     if conan: → conanfile.txt + conan install (only if conanfile changed)
    │     if vcpkg: → vcpkg.json (only if manifest changed); needs VCPKG_ROOT
    │     cmake -S:
    │       conan only  → CMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    │       vcpkg only  → CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/.../vcpkg.cmake
    │       both        → vcpkg.cmake + VCPKG_CHAINLOAD_TOOLCHAIN_FILE=conan_toolchain
    │     cmake --build
    │     exec build/script
    │
    └─ else: gcc/g++ -o script && exec
```

C++ scripts use `-std=c++17` (plain build) / `CMAKE_CXX_STANDARD 17` (CMake).

Cache root: `${XDG_CACHE_HOME:-~/.cache}/cx/<sha256(abspath)>/`.

Conan packages land under `build/` (`conan install -of build`). vcpkg packages land under `vcpkg_installed/` (manifest mode). The **script** is still one CMake project — both PMs are visible via toolchain chainloading when mixed.

## `//DEPS` contract

Every dependency **must** start with a prefix:

| Prefix | Role |
|--------|------|
| `conan:` | Resolve via Conan (`CMakeDeps` + `CMakeToolchain`) |
| `vcpkg:` | Resolve via vcpkg manifest (`vcpkg.json` + `vcpkg.cmake`) |
| `gh:` | GitHub — either whole-repo CMake or raw files |

The same package **name** must not appear under both `conan:` and `vcpkg:` in one script.

### `vcpkg:` shape

- `vcpkg:fmt` — dependency only (registry / baseline from the user’s vcpkg).
- `vcpkg:fmt/10.2.1` — also emit a manifest `overrides` entry plus `builtin-baseline` from `VCPKG_ROOT`’s git HEAD.
- `vcpkg:boost[asio,filesystem]` — enable those features.

### `gh:` shape

- **3** path segments (`owner/repo/ref`) → `FetchContent_Declare` + `MakeAvailable`, then link `AS` target (default derived from repo name).
- **4+** segments → download from `raw.githubusercontent.com`; bare stem fetches `.h`+`.c`, a filename with extension fetches that file only.

### Brackets `[…]`

```cpp
//DEPS conan:boost/1.85.0[header_only=True] AS Boost::headers
//DEPS vcpkg:boost[asio,filesystem]
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False] AS jmespath::jmespath
```

| Backend | Brackets mean |
|---------|----------------|
| `conan:` | Conan `[options]` → `pkg/*:key=val` (keys stay lowercase) |
| `vcpkg:` | feature names (comma-separated) |
| `gh:` (FetchContent) | CMake `set(KEY VAL CACHE BOOL …)` — key is uppercased (`build_testing` → `BUILD_TESTING`) |

`True`/`False`/`ON`/`OFF` are accepted for Conan/`gh:` bools. Nothing is implied — pass `build_testing=False` (and project flags like `jmespath_build_tests=False`) when needed.

### `AS`

Overrides CMake `find_package` / link target when the default `name::name` guess is wrong (`AS Boost::headers`, `AS jmespath::jmespath`).

## Why Conan / vcpkg + CMake

- **Conan** and **vcpkg** each own a dependency *graph* (versions, options/features, transitive libs). Install trees are independent.
- **CMake** owns *build/link* of the script and of `gh:` repos (FetchContent). When both PMs are used, vcpkg’s toolchain is primary and Conan’s is chainloaded so `find_package` sees both.

Raw `gh:` files skip package managers and only need curl + a generated `add_executable`.

## Invalidation

| Change | Effect |
|--------|--------|
| Script bytes change | New `.cx_stamp` → rebuild; raw `gh:` files re-fetched |
| `conanfile.txt` content change | `conan install` again |
| `vcpkg.json` content change | reconfigure (manifest install during cmake) |
| Generated `CMakeLists.txt` change | `cmake -S` again |
| Otherwise | `cmake --build` only, or warm `exec` |
