# cx

jbang-style C/C++ scripts. Every dep is explicit: `conan:` or `gh:`.

```bash
chmod +x cx
export PATH="$PWD:$PATH"
brew install cmake conan curl openssl  # plus a C/C++ toolchain (gcc/g++ or clang)
conan profile detect   # once
```

## Usage

```bash
cx examples/hello.c
cx examples/hello.cpp
cx examples/fmt.cpp
cx examples/jmespath.cpp
```

Scripts use normal `.c` / `.cpp` extensions (`//DEPS` comments are enough). C++ is built as C++17.

See [ARCHITECTURE.md](ARCHITECTURE.md) for the full pipeline.

## `//DEPS`

Prefix is **required**:

| Prefix | Ref | Backend |
|--------|-----|---------|
| `conan:` | `fmt/10.2.1` | Conan |
| `gh:` | `owner/repo/ref` (3 parts) | GitHub → CMake FetchContent |
| `gh:` | `owner/repo/ref/path/…` (4+) | GitHub raw file(s) |

### Options `[key=val,…]`

Keys are **lowercase** for both backends (Cargo/Conan style):

```cpp
//DEPS conan:boost/1.85.0[header_only=True] AS Boost::headers
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False,jmespath_build_tests=False] AS jmespath::jmespath
```

| Backend | Brackets |
|---------|----------|
| `conan:` | package options as written (`header_only=True`) |
| `gh:` repo | CMake cache bools — keys uppercased (`build_testing` → `BUILD_TESTING`) |

Values: `True`/`False` (or `ON`/`OFF`). No hidden defaults — pass flags when you need them.

Optional `AS pkg::target` when the default link name is wrong.

```cpp
//DEPS conan:fmt/10.2.1
//DEPS gh:rxi/log.c/master/src/log
//DEPS conan:nlohmann_json/3.11.3
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[build_testing=False,jmespath_build_tests=False] AS jmespath::jmespath
```

## Cache

`${XDG_CACHE_HOME:-~/.cache}/cx/<path-hash>/` with a content stamp: unchanged scripts warm-`exec`; Conan / `cmake -S` run only when generated inputs change.
