# Building

Install Qt 6, FreeRDP 3 development libraries, pkg-config, CMake, and a C++20
compiler using the distribution package manager. On Arch/Omarchy the relevant
packages are `base-devel cmake qt6-base freerdp`; on Ubuntu the package names
are typically `build-essential cmake qt6-base-dev libfreerdp3-dev` (verify the
FreeRDP 3 package name for the particular Ubuntu release).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Sanitizer build:

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DOPENRDP_ENABLE_SANITIZERS=ON
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

CMake uses pkg-config and links the public distribution packages
`freerdp3`, `freerdp-client3`, and `winpr3`; no architecture-specific library
directory is hardcoded.
