# Building - Linux
Congratulations! Linux is far easier to build on than Windows.

### Setup
- You will need to have `cmake`, `make`, and `gcc` installed.
- You will need development packages as outlined in the libraries section.

### Libraries

You will need development packages for GLM, GLEW, SDL3, assimp, and ZLIB.

Arch Packages:
[glm](https://archlinux.org/packages/extra/x86_64/glm/),
[glew](https://archlinux.org/packages/extra/x86_64/glew/),
[sdl3](https://archlinux.org/packages/extra/x86_64/sdl3/),
[zlib](https://archlinux.org/packages/core/x86_64/zlib/),
[assimp](https://archlinux.org/packages/extra/x86_64/assimp/),

Note: There is no SDL3 package available for Ubuntu versions before 25.04 "Plucky Puffin."
If you are using an older version, you will have to manually build and install SDL3.

### Building
There are 3 build types available:
- `Debug` - No optimizations, debug symbols and features enabled.
- `Release` - Full optimizations, no debug symbols or features.
- `RelWithDebInfo` - Full optimizations, debug symbols and features enabled.

Open the terminal in the project directory and run the following commands to build the project:
```sh
mkdir build
cd build
cmake -B . -DCMAKE_BUILD_TYPE=[Build type] ..
cmake --build . --target all -- -j
```
The compiled executables will be in the `build/bin` directory.
