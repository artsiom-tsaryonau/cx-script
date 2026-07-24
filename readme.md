# cx

jbang-style C/C++ scripts. Every dep is explicit: `conan:` or `gh:`.

```bash
chmod +x cx
export PATH="$PWD:$PATH"
brew install cmake conan
conan profile detect   # once
```

## Usage

```bash
cx examples/hello.cx
cx examples/fmt.cppx
cx examples/jmespath.cppx
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for the full pipeline.

## `//DEPS`

Prefix is **required**:

| Prefix | Ref | Backend |
|--------|-----|---------|
| `conan:` | `fmt/10.2.1` | Conan |
| `gh:` | `owner/repo/ref` (3 parts) | GitHub → CMake FetchContent |
| `gh:` | `owner/repo/ref/path/…` (4+) | GitHub raw file(s) |

### Options `[key=val,…]`

Same bracket syntax for both backends (like Cargo features / Conan options):

```cpp
//DEPS conan:boost/1.85.0[header_only=True] AS Boost::headers
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[BUILD_TESTING=False,JMESPATH_BUILD_TESTS=False] AS jmespath::jmespath
```

| Backend | Brackets |
|---------|----------|
| `conan:` | package options (`header_only=True`, …) |
| `gh:` repo | CMake cache bools (`BUILD_TESTING=False`, …) |

No hidden defaults — pass flags explicitly when you need them.

Optional `AS pkg::target` when the default link name is wrong.

```cpp
//DEPS conan:fmt/10.2.1
//DEPS gh:rxi/log.c/master/src/log
//DEPS conan:nlohmann_json/3.11.3
//DEPS gh:robertmrk/jmespath.cpp/0.2.1[BUILD_TESTING=False,JMESPATH_BUILD_TESTS=False] AS jmespath::jmespath
```

## Cache

`~/.cache/cx/<path-hash>/` with a content stamp: unchanged scripts warm-`exec`; Conan / `cmake -S` run only when generated inputs change.
