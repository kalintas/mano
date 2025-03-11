Mano emulator in C++

# Installation<br>
First install the Emscripten.
https://emscripten.org/docs/getting_started/downloads.html
Im using vcpkg for package managing, install required packages:
Be sure to install it in emscripten triplet.
```
vcpkg install sdl2:wasm32-emscripten
vcpkg install imgui:wasm32-emscripten

vcpkg integrate install
```
After that you should run the build script and the cmake.
```
bash build.sh or ./build.sh
cmake --build build
```
# Running locally 
To run on the local you must use emrun:
```
emrun build.emscripten/index.html
```
