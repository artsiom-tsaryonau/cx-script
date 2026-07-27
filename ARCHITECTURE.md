# Architecture

How `cx` turns a script into a running binary.

## Pipeline

```text
script.c / script.cpp
    │
    ├─ resolve CC/CXX (env, else clang→gcc / clang++→g++)
    │
    ├─ warm?  (.cx_stamp == content|compilers && binary exists)
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
    ├─ if any //DEPS: generate CMakeLists.txt
    │     if conan: → conanfile.txt + conan install (only if conanfile changed)
    │     if vcpkg: → vcpkg.json (only if manifest changed); needs VCPKG_ROOT
    │     cmake -S (one arg list):
    │       toolchain = vcpkg and/or conan as needed
    │       compilers  = detected CC/CXX unless Conan owns the profile
    │     cmake --build
    │     exec build/script
    │
    └─ else: $CC / $CXX -o script && exec
```

C++ scripts use `-std=c++17` (plain build) / `CMAKE_CXX_STANDARD 17` (CMake).

`CC` and `CXX` are the usual Unix env vars for the C and C++ compiler commands (e.g. `CC=clang CXX=clang++ cx script.cpp`).

Cache root: `${XDG_CACHE_HOME:-~/.cache}/cx/<sha256(abspath)>/`.

Build directories are path-keyed so editing a script keeps the same workspace (Conan/vcpkg trees stay warm). The stamp is `content_hash|CC_BIN|CXX_BIN` — changing the compiler path forces a rebuild.

Any `//DEPS` line uses CMake (including header-only `gh:` fetches). No-deps scripts use a direct compiler invocation.

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
| `CC` / `CXX` path change | New stamp → rebuild |
| `conanfile.txt` content change | `conan install` again |
| `vcpkg.json` content change | reconfigure (manifest install during cmake) |
| Generated `CMakeLists.txt` change | `cmake -S` again |
| Otherwise | `cmake --build` only, or warm `exec` |

## Future: C++ environments (“venv”)

Python’s venv is path isolation around one interpreter. C++ has no import-time module path — isolation means **headers, libs, transitive graphs, and the toolchain that built them**, exposed at compile time via a prefix / toolchain file.

`cx` already does the per-script half of this (cache dir + Conan/vcpkg install trees). A named environment layer should build on that, not replace Conan/vcpkg.

### Recommended tiers

| Tier | Goal | Sketch |
|------|------|--------|
| **1. Shared package cache** | Avoid reinstalling the same dep set under every path-keyed script dir | Content- or dep-set–addressed trees under `~/.cache/cx/pkgs/<hash>/`; script dirs link or `CMAKE_PREFIX_PATH` into them. Keep path-keyed **workspaces** so script edits stay incremental. |
| **2. Named env** | Reuse one dep set across scripts / shells | `cx env create <name>` from `//DEPS` or a lockfile → `~/.cache/cx/envs/<name>/` with manifests + install prefix + recorded compiler fingerprint. |
| **3. Activate** | Feel like `source venv/bin/activate` | `eval "$(cx env activate <name>)"` exports `CMAKE_PREFIX_PATH`, `PKG_CONFIG_PATH`, `CPATH` / `CPLUS_INCLUDE_PATH`, `LIBRARY_PATH`, and optionally `CMAKE_TOOLCHAIN_FILE` / `CX_ENV`. |
| **4. True PM isolation** | Envs don’t share global Conan/vcpkg state | Per-env `CONAN_HOME` (and vcpkg binary-cache / installed tree). Host still needs the `vcpkg` / `conan` *tools*; only package stores are private. |
| **5. Bundled toolchain** | Fully hermetic (conda-style) | Ship or download a pinned clang/gcc into the env. Defer — ABI + size dominate; tiers 1–4 deliver most of the value. |

### Suggested layout

```text
~/.cache/cx/
  pkgs/<dep-set-hash>/          # tier 1 shared installs
  envs/<name>/
    lock / conanfile / vcpkg.json
    prefix/                     # or PM-native trees
    toolchain.cmake
    activate.sh
    meta (compiler fingerprint, CX version)
  <script-abspath-hash>/        # today’s per-script build workspace
```

### CLI / DSL (proposed)

```bash
cx env create ml --from examples/jmespath.cpp   # or --from lock file
cx env activate ml                              # print export lines
cx -e ml script.cpp                             # build/run inside env
cx env run ml -- cmake -S …                     # arbitrary command with env vars
```

Optional script hint:

```cpp
//ENV ml
//DEPS conan:fmt/10.2.1   # merge into env or require match with lock
```

### Design rules (when implementing)

1. **Do not reimplement package resolution** — keep generating Conan/vcpkg/CMake inputs; own layout, locks, activation, and cache keys only.
2. **Record the compiler** in env metadata (same fingerprint idea as `.cx_stamp`). Refuse or warn when activating an env built with a different ABI/toolchain.
3. **Prefer static / rpath-friendly profiles** on macOS so activation does not depend on fragile `DYLD_LIBRARY_PATH`.
4. **Locks are not sandboxes** — separate caches ≠ security isolation.
5. **Reproducibility next** — pin vcpkg `builtin-baseline` in the env lock (not only live `VCPKG_ROOT` HEAD); prefer commit tags over branch names for `gh:`.

### What not to do early

- Replace `//DEPS` with a separate `requirements.txt` dialect (keep one source of truth).
- Bundle compilers before named env + activate work.
- Switch script workspaces to pure content-hash keys (that cold-starts Conan/vcpkg on every edit); share **packages**, not **build dirs**.
