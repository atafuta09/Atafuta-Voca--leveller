@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
set "PATH=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"

echo === Running CMake Configure with Ninja ===
cmake -B build_ninja -G Ninja -DCMAKE_BUILD_TYPE=Release -DCOPY_PLUGIN_AFTER_BUILD=FALSE

echo === Building AutoLeveler_VST3 ===
cmake --build build_ninja --config Release --target AutoLeveler_VST3
