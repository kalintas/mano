Mano simulator in C++

# Installation<br>
First install the Emscripten.
```
https://emscripten.org/docs/getting_started/downloads.html
```
Install required packages using vcpkg (make sure to use the emscripten triplet):
```
vcpkg install sdl2:wasm32-emscripten
vcpkg install imgui:wasm32-emscripten

vcpkg integrate install
```
After that run the build script to configure the build.
Edit `build.sh` to update vcpkg and emsdk paths to match your system
```
bash build.sh or ./build.sh
```
And finally build the project.
```
cmake --build build
```
# Running locally 
You must use emrun to run the simulator locally.
```
emrun docs/index.html
```
