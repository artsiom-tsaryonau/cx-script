# Architecture

How `cx` turns a script into a running binary.

## Pipeline

```text
script.cx / .cppx
    │
    ├─ warm?  (.cx_stamp == sha256(script) && binary exists)
    │         → exec cached binary
    │
    ├─ parse //DEPS lines
    │     conan:…  → Conan requires (+ [options])
    │     gh:…/…/ref           → FetchContent (CMake)
    │     gh:…/…/ref/path/…    → curl raw file(s)
    │
    ├─ strip shebang → script.c / script.cpp in cache
    │
    ├─ if any deps: generate CMakeLists (+ conanfile.txt)
    │     conan install (only if conanfile changed)
    │     cmake -S     (only if generated files changed)
    │     cmake --build
    │     exec build/script
    │
    └─ else: gcc/g++ -o script && exec
```

Cache root: `~/.cache/cx/<sha256(abspath)>/`.

## `//DEPS` contract

Every dependency **must** start with a prefix:

| Prefix | Role |
|--------|------|
| `conan:` | Resolve via Conan (`CMakeDeps` + `CMakeToolchain`) |
| `gh:` | GitHub — either whole-repo CMake or raw files |

### `gh:` shape

- **3** path segments (`owner/repo/ref`) → `FetchContent_Declare` + `MakeAvailable`, then link `AS` target (default derived from repo name).
- **4+** segments → download from `raw.githubusercontent.com`; bare stem fetches `.h`+`.c`, a filename with extension fetches that file only.

### Brackets `[key=val,…]` (lowercase keys for both backends)

```cpp
//DEPS conan:boost/1.85.0[header_only=True] AS Boost::headers
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False] AS jmespath::jmespath
```

| Backend | Brackets mean |
|---------|----------------|
| `conan:` | Conan `[options]` → `pkg/*:key=val` (keys stay lowercase) |
| `gh:` (FetchContent) | CMake `set(KEY VAL CACHE BOOL …)` — key is uppercased (`build_testing` → `BUILD_TESTING`) |

`True`/`False`/`ON`/`OFF` are accepted for bools. Nothing is implied — pass `build_testing=False` (and project flags like `jmespath_build_tests=False`) when needed.

### `AS`

Overrides CMake `find_package` / link target when the default `name::name` guess is wrong (`AS Boost::headers`, `AS jmespath::jmespath`).

## Why Conan + CMake

- **Conan** owns the dependency *graph* (versions, options, transitive libs) — the Maven Central analogue for C/C++.
- **CMake** owns *build/link* of the script and of `gh:` repos (FetchContent), using Conan’s toolchain when Conan deps are present.

Raw `gh:` files skip Conan and only need curl + a generated `add_executable`.

## Invalidation

| Change | Effect |
|--------|--------|
| Script bytes change | New `.cx_stamp` → rebuild; raw `gh:` files re-fetched |
| `conanfile.txt` content change | `conan install` again |
| Generated `CMakeLists.txt` change | `cmake -S` again |
| Otherwise | `cmake --build` only, or warm `exec` |
