rm -rf build
mkdir build
cd build
cmake\
  "-DCMAKE_TOOLCHAIN_FILE=/home/unuzdaq/vcpkg/scripts/buildsystems/vcpkg.cmake"\
  "-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=~/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"\
  "-DVCPKG_TARGET_TRIPLET=wasm32-emscripten" ..
