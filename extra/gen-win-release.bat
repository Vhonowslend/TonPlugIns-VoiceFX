SET _CWD=%~dp0

PUSHD "%_CWD%\.." >NUL
cmake -H. -Bbuild/release/64 -G "Visual Studio 16 2019" -A"x64" -DCMAKE_INSTALL_PREFIX="build/release/install" -DINSTALL_VST2=ON
cmake --build build/release/64 --config RelWithDebInfo --target INSTALL
cmake --build build/release/64 --config RelWithDebInfo --target PACKAGE_7Z
cmake --build build/release/64 --config RelWithDebInfo --target PACKAGE_ZIP
"build\release\64\installer.iss"
POPD
