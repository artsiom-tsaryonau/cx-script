# cx

jbang-style C/C++ scripts: one `//DEPS` line, shape picks the backend.

```bash
chmod +x cx
export PATH="$PWD:$PATH"
brew install cmake conan   # for Conan / CMake GitHub repos
conan profile detect       # once
```

## Usage

```bash
cx examples/hello.cx
cx examples/hello.cppx
cx examples/fmt.cppx
cx examples/jmespath.cppx
```

## `//DEPS` (unified)

| Ref shape | Backend |
|-----------|---------|
| `fmt/10.2.1` | Conan (`name/version`) |
| `pkg/ver@user/channel` | Conan |
| `owner/repo/ref` | GitHub repo → CMake `FetchContent` |
| `owner/repo/ref/path/stem` | GitHub raw `.h` + `.c` |
| `owner/repo/ref/path/file.hpp` | GitHub raw single file |

Optional: `AS pkg::target` (or `AS pkg`) when the CMake name ≠ the default.

```cpp
//DEPS fmt/10.2.1
//DEPS rxi/log.c/master/src/log
//DEPS p-ranav/indicators/master/single_include/indicators/indicators.hpp
//DEPS boost/1.85.0 AS Boost::headers
//DEPS nlohmann_json/3.11.3
//DEPS robertmrk/jmespath.cpp/0.2.1 AS jmespath::jmespath
```

### jmespath

The docs say `jmespath.cpp/x.y.z@robertmrk/stable`, but that remote was **Bintray** (HTTP 410 Gone). The working form is GitHub FetchContent for the library + Conan Center for Boost / nlohmann_json (see `examples/jmespath.cppx`).

## Architecture

```text
//DEPS  →  classify by shape
            ├─ Conan        → conan install + CMakeDeps
            ├─ owner/repo/ref → FetchContent
            └─ longer path  → curl raw file(s)
         →  generated CMakeLists → build → exec
```

No `//GITHUB` / `H` / `HPP` keywords — the ref string is enough.

Cache: `~/.cache/cx/<hash>/`.
