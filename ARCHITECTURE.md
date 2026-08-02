# Architecture

How `cx` turns a script into a running binary.

## Bash baseline

Requires **Bash 4.3+** (`nameref`, solid arrays). `cx selfcheck` verifies bash version, curl/openssl, a no-deps compile/run, and the `//DEPS` regex.

Notable practices: `set -euo pipefail`, `type -P` for compilers, `pwd -P`, mkdir locks (not `flock`), stamp re-check after lock, `nullglob` for vcpkg lib globs, no `eval` for path mutation.

```text
script.c / script.cpp
    │
    ├─ resolve CC/CXX (env, else Darwin: clang→gcc / Linux: gcc→clang)
    │
    ├─ cache base: CX_CACHE | $CX_ROOT/cache | ./.cx/cache | XDG ~/.cache/cx
    │
    ├─ warm?  (.cx_stamp == content|host|compilers && binary exists)
    │         → prepare_run_env → cd $CALLER_PWD → exec
    │
    ├─ parse //DEPS lines
    │     conan:…  → Conan requires (+ [options]) + VirtualRunEnv
    │     vcpkg:…  → vcpkg.json dependencies (+ features / version overrides)
    │     gh:…/…/ref           → FetchContent (CMake)
    │     gh:…/…/ref/path/…    → curl raw file(s)
    │
    ├─ strip shebang → copy as script.c / script.cpp in cache
    │
    ├─ if any //DEPS: generate CMakeLists.txt (+ RPATH hints)
    │     if conan: → conanfile.txt + conan install (only if conanfile changed)
    │     if vcpkg: → vcpkg.json (only if manifest changed); needs VCPKG_ROOT
    │     cmake -S / cmake --build
    │
    └─ else: $CC / $CXX -o script
         then prepare_run_env → cd $CALLER_PWD → exec
```

C++ scripts use `-std=c++17` (plain build) / `CMAKE_CXX_STANDARD 17` (CMake).

### CWD contract

Build artifacts live under the cache dir; **execution** restores the caller’s working directory and sets `CX_CWD` to that path. Scripts do not receive a synthetic `argv[1]` path — use `getcwd()` / `CX_CWD` / relative paths.

### Run-time libraries

- Conan: `VirtualRunEnv` → `build/conanrun.sh` sourced before exec  
- vcpkg: `vcpkg_installed/*/lib` (and `bin`) prepended to `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH`  
- CMake: `CMAKE_BUILD_WITH_INSTALL_RPATH` + `CMAKE_BUILD_RPATH_USE_ORIGIN`

## Project env (`.cx/`)

```text
.cx/
  activate              # source this, or eval "$(cx env activate)"
  cache/                # per-script build workspaces (+ vcpkg_installed/)
  conan/                # CONAN_HOME
  vcpkg/                # private clone + bootstrap (cx env init)
  vcpkg-bincache/
  vcpkg-downloads/
```

`cx env init` creates this and, by default, shallow-clones + bootstraps vcpkg into `.cx/vcpkg`. Use `--no-vcpkg` to skip and rely on an ambient `VCPKG_ROOT`.

Presence of `./.cx` (or `CX_ROOT`) redirects cache, Conan home, binary caches, and (when present) `VCPKG_ROOT` without requiring activate.

`.cx/conan` needs its own `profiles/default` — `conan profile detect` after activate, or auto on first `conan install`.

**Still host-global:** compiler, libc, OpenSSL, CMake, curl/git, Conan CLI. **Env-local:** Conan store, vcpkg tool/registry, vcpkg downloads/binary cache, per-script install/build trees.

This is **not** a VM or a full sysroot. Recreate `.cx` on each machine with `cx env init`; don’t copy it across OS/arch.

## `//DEPS` contract

Every dependency **must** start with a prefix:

| Prefix | Role |
|--------|------|
| `conan:` | Resolve via Conan (`CMakeDeps` + `CMakeToolchain` + `VirtualRunEnv`) |
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

### `AS`

Overrides CMake `find_package` / link target when the default `name::name` guess is wrong.

## Why Conan / vcpkg + CMake

- **Conan** and **vcpkg** each own a dependency *graph*. Install trees are independent and **host-specific** (Linux vs macOS ABI).
- **CMake** owns *build/link* of the script and of `gh:` repos.
- Portable unit is `//DEPS` (+ optional project `.cx` layout), not the downloaded package blobs.

## Invalidation

| Change | Effect |
|--------|--------|
| Script bytes change | New `.cx_stamp` → rebuild; raw `gh:` files re-fetched |
| OS/arch or compiler path/version | New stamp → rebuild |
| `conanfile.txt` content change | `conan install` again |
| `vcpkg.json` content change | reconfigure (manifest install during cmake) |
| Generated `CMakeLists.txt` change | `cmake -S` again |
| Otherwise | `cmake --build` only, or warm `exec` |

## CLI

```bash
cx <script> [args...]
cx clean <script>|--all
cx env init [dir]
cx env activate [dir]    # eval "$(cx env activate)"
```

## What we deliberately skip (for now)

- Shared global `pkgs/<dep-hash>/` dedup across scripts (Conan/vcpkg caches already help)
- Bundled compilers
- Replacing `//DEPS` with a second lock dialect
- Content-hashing build *workspaces* (would cold-start PMs on every edit)
