# Compilation
pushd ..

rd /S /Q build-win-release
cmake -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -B build-win-release
cmake --build build-win-release --config Release

popd
