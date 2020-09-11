SET _CWD=%~dp0

SET "NVAFX=C:\Tools\SDKs\Nvidia\Audio-Effects-0.5.3"
SET "VSTSDK=C:\Tools\SDKs\Steinberg\VST_SDK"

PUSHD "%_CWD%\.." >NUL
cmake -H. -Bbuild/release/32 -G "Visual Studio 16 2019" -A"Win32" -DCMAKE_INSTALL_PREFIX="build/release/install" -DVSTSDK_DIR="%VSTSDK%" -DNVAFX_DIR="%NVAFX%"
cmake -H. -Bbuild/release/64 -G "Visual Studio 16 2019" -A"x64"   -DCMAKE_INSTALL_PREFIX="build/release/install" -DVSTSDK_DIR="%VSTSDK%" -DNVAFX_DIR="%NVAFX%"
cmake --build build/release/32 --config RelWithDebInfo --target INSTALL
cmake --build build/release/64 --config RelWithDebInfo --target INSTALL
cmake --build build/release/64 --config RelWithDebInfo --target PACKAGE_7Z
cmake --build build/release/64 --config RelWithDebInfo --target PACKAGE_ZIP
"build\release\64\installer.iss"
POPD
