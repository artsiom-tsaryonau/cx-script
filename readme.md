# cx

C/C++ script runner with jbang-style GitHub deps.

## Install

```bash
chmod +x cx
# put this directory on PATH, or:
export PATH="$PWD:$PATH"
```

Needs: `bash`, `curl`, `openssl`, `gcc` / `g++`.

## Usage

```bash
cx examples/hello.cx
cx examples/hello.cppx
```

Extensions: `.c` / `.cx` → `gcc`, `.cpp` / `.cppx` → `g++`.

## Dependencies

```c
//GITHUB H rxi/log.c/master/src/log
//GITHUB HPP p-ranav/indicators/master/single_include/indicators/indicators
```

Format: `//GITHUB <kind> <owner>/<repo>/<ref>/<path/to/stem>`

| Kind | Downloads | Notes |
|------|-----------|--------|
| `H` | `stem.h` + `stem.c` | `.c` is compiled and linked |
| `HPP` | `stem.hpp` | header-only |

Resolved as:

`https://raw.githubusercontent.com/<owner>/<repo>/<ref>/<path/to/stem>.<ext>`

Local include name is the basename: `#include "log.h"`, `#include "indicators.hpp"`.

Cache: `~/.cache/cx/<hash>/` (hash of absolute script path). Rebuilds when the script is newer than the binary. Wipe the cache dir to force re-fetch (no checksums yet).
