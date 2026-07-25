# cx

jbang-style C/C++ scripts. Every dep is explicit: `conan:`, `vcpkg:`, or `gh:`.

```bash
chmod +x cx
export PATH="$PWD:$PATH"
brew install cmake conan curl openssl  # plus a C/C++ toolchain (gcc/g++ or clang)
conan profile detect   # once

# for vcpkg: deps — bootstrapped checkout required (once):
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg   # add to ~/.zshrc
```

## Usage

```bash
cx examples/hello.c
cx examples/hello.cpp
cx examples/fmt.cpp
cx examples/fmt_vcpkg.cpp
cx examples/mixed_pm.cpp
cx examples/jmespath.cpp
```

Scripts use normal `.c` / `.cpp` extensions (`//DEPS` comments are enough). C++ is built as C++17.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the full pipeline.

## `//DEPS`

Prefix is **required**:

| Prefix | Ref | Backend |
|--------|-----|---------|
| `conan:` | `fmt/10.2.1` | Conan |
| `vcpkg:` | `fmt` or `fmt/10.2.1` | vcpkg (manifest mode) |
| `gh:` | `owner/repo/ref` (3 parts) | GitHub → CMake FetchContent |
| `gh:` | `owner/repo/ref/path/…` (4+) | GitHub raw file(s) |

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
```

## Cache

Each script gets a directory under `${XDG_CACHE_HOME:-~/.cache}/cx/<sha256(abspath)>/`.

- Unchanged script content → warm `exec` of the cached binary
- Content change → rebuild; Conan / vcpkg manifest / `cmake -S` re-run only when generated inputs change

There is no `cx clean`. Wipe cache manually:

```bash
# one script (abspath of the .c/.cpp file)
rm -rf ~/.cache/cx/"$(printf '%s' /absolute/path/to/script.cpp | openssl dgst -sha256 | awk '{print $NF}')"

# all cx caches
rm -rf ~/.cache/cx
```
