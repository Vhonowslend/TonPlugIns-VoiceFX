SET _CWD=%~dp0

PUSHD "%_CWD%\.." >NUL

:: VST2.x
cmake -H. -Bbuild/release/vst2 -G "Visual Studio 16 2019" -A"x64" -DCMAKE_INSTALL_PREFIX="build/release/vst2/install" -DENABLE_STEINBERG_VST3=OFF -DENABLE_STEINBERG_VST2=ON
cmake --build build/release/vst2 --config RelWithDebInfo --target INSTALL
"build\release\vst2\installer-vst2.iss"

POPD
EXIT
