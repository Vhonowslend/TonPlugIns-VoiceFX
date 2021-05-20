SET _CWD=%~dp0

PUSHD "%_CWD%\.." >NUL

:: VST3.x
cmake -H. -Bbuild/release/vst3 -G "Visual Studio 16 2019" -A"x64" -DCMAKE_INSTALL_PREFIX="build/release/vst3/install" -DENABLE_STEINBERG_VST3=ON
cmake --build build/release/vst3 --config RelWithDebInfo --target INSTALL
"build\release\vst3\installer-vst3.iss"

POPD
EXIT
