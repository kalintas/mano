rm -rf release 
mkdir release 
cd release 
cmake\
  "-DCMAKE_TOOLCHAIN_FILE=/home/unuzdaq/vcpkg/scripts/buildsystems/vcpkg.cmake"\
  "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=~/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"\
  "-DCMAKE_BUILD_TYPE=Release"\
  "-DVCPKG_TARGET_TRIPLET=wasm32-emscripten" ..
