# cx

A script runner for C and C++ with inline //DEPS — GitHub, git remotes, Conan, and vcpkg.

jbang-style single-file scripts. Every dep is explicit: `conan:`, `vcpkg:`, `gh:`, or `git:`.

```bash
chmod +x cx
export PATH="$PWD:$PATH"
# Needs Bash 4.3+ (nameref). On macOS: brew install bash
cx selfcheck
```

### Install (once per machine)

Host tools only — compilers, CMake, Conan CLI, curl/git. Package *trees* live in the project env (`.cx/`), not in system prefixes.

**Fedora**

```bash
sudo dnf install gcc gcc-c++ cmake curl git openssl ninja-build zip unzip tar
pipx install conan   # or: pip install --user conan
conan profile detect   # optional global ~/.conan2; project envs get their own
```

**macOS**

```bash
brew install cmake conan curl git openssl  # plus Xcode CLT or gcc
conan profile detect   # optional global profile
```

You do **not** need a global `~/vcpkg` when using `cx env init` (it clones vcpkg into `.cx/vcpkg`). You do **not** need different `//DEPS` per OS. Don’t copy `.cx/` between Linux and macOS — recreate with `cx env init` on each machine.

Still global by design: compilers, libc, OpenSSL, CMake, curl/git, and the `conan` CLI. Everything those tools *download* for a project should land under `.cx/`.

## Usage

```bash
cx examples/hello.c
cx examples/hello.cpp
cx examples/fmt.cpp
cx examples/fmt_vcpkg.cpp
cx examples/mixed_pm.cpp
cx examples/jmespath.cpp
```

Scripts use normal `.c` / `.cpp` extensions. C++ is built as C++17.

`CC` / `CXX` select compilers. Defaults: **Linux → gcc/g++ then clang**; **macOS → clang then gcc**. Conan builds still follow the Conan profile.

### Working directory

The binary runs with **cwd = the directory where you invoked `cx`** (not the cache build dir). `CX_CWD` is set to the same path. Relative opens like `fopen("data.txt")` and `getcwd()` behave as expected; real CLI args are unchanged (`argv[1]` is your first arg, not a injected path).

### Project env (venv-shaped)

```bash
cx env init                 # .cx/{cache,conan,vcpkg,activate} — clones+bootstraps vcpkg
# cx env init --no-vcpkg    # skip clone; use ambient VCPKG_ROOT instead
source .cx/activate         # or: eval "$(cx env activate)"
conan profile detect        # once per .cx (or let cx do it on first conan: build)
cx examples/fmt.cpp         # Conan packages → .cx/conan
cx examples/fmt_vcpkg.cpp   # uses .cx/vcpkg; installs → .cx/cache/.../vcpkg_installed
```

Layout:

```text
.cx/
  activate
  cache/                 # per-script builds + vcpkg_installed/
  conan/                 # CONAN_HOME
  vcpkg/                 # private bootstrapped vcpkg (unless --no-vcpkg)
  vcpkg-bincache/
  vcpkg-downloads/
```

If `.cx/` exists in the current directory (or `CX_ROOT` is set), `cx` picks it up automatically (including `VCPKG_ROOT=.cx/vcpkg` when present). Add `.cx/` to `.gitignore`.

```bash
cx clean examples/fmt.cpp   # drop one script’s build dir
cx clean --all              # wipe .cx/cache only (keeps conan/ + vcpkg/)
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for the pipeline.

## `//DEPS`

Prefix is **required**:

| Prefix | Ref | Backend |
|--------|-----|---------|
| `conan:` | `fmt/10.2.1` | Conan |
| `vcpkg:` | `fmt` or `fmt/10.2.1` | vcpkg (manifest mode) |
| `gh:` | `owner/repo/ref` (3 parts) | GitHub → CMake FetchContent |
| `gh:` | `owner/repo/ref/path/…` (4+) | GitHub raw file(s) |
| `git:` | `https://host/…/repo.git#ref` | Any git host → FetchContent |

`gh:` is GitHub shorthand. `git:` is the same FetchContent path with a **full repository URL** (GitLab, Codeberg, self-hosted, …). Raw single-file fetch remains `gh:` only (GitHub raw URLs).

`conan:` and `vcpkg:` may appear in the same script (install trees stay separate; CMake chainloads both toolchains). Do not request the **same package name** from both.

### Options / features `[…]`

Keys are **lowercase** for Conan and `gh:`. For `vcpkg:`, brackets list **features** (not `key=val`):

```cpp
//DEPS conan:boost/1.85.0[header_only=True] AS Boost::headers
//DEPS vcpkg:boost[asio,filesystem]
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False,jmespath_build_tests=False] AS jmespath::jmespath
```

| Backend | Brackets |
|---------|----------|
| `conan:` | package options as written (`header_only=True`) |
| `vcpkg:` | feature names (`asio,filesystem`) |
| `gh:` repo | CMake cache bools — keys uppercased (`build_testing` → `BUILD_TESTING`) |
| `git:` repo | same as `gh:` repo (FetchContent options) |

Values for Conan/`gh:` bools: `True`/`False` (or `ON`/`OFF`). No hidden defaults — pass flags when you need them.

Optional `AS pkg::target` when the default link name is wrong (`name::name` is assumed otherwise — e.g. Conan `libcurl` needs `AS CURL::libcurl`).

### Versions

Conan always uses `name/version`. For vcpkg, omit the version to take the registry baseline, or pin with `name/version` (written to manifest `overrides` plus `builtin-baseline` from your `VCPKG_ROOT` git HEAD):

```cpp
//DEPS conan:fmt/10.2.1
//DEPS vcpkg:fmt
//DEPS vcpkg:fmt/10.2.1
//DEPS vcpkg:boost/1.85.0[asio,filesystem]
//DEPS gh:rxi/log.c/master/src/log
//DEPS conan:nlohmann_json/3.11.3
//DEPS conan:libcurl/8.12.1 AS CURL::libcurl
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False,jmespath_build_tests=False] AS jmespath::jmespath
//DEPS git:https://gitlab.com/user/mylib.git#v2.0 AS mylib::mylib
```

## Cache

Resolution order:

1. `CX_CACHE` if set  
2. `$CX_ROOT/cache` or `./.cx/cache` when a project env exists  
3. `${XDG_CACHE_HOME:-~/.cache}/cx`

Each script gets `<cache>/<sha256(abspath)>/`. Stamp is `content|host|CC@ver|CXX@ver`. Unchanged stamp → warm exec; shared libs use Conan `VirtualRunEnv` and/or vcpkg lib dirs on `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH`.
