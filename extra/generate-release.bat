@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION
SET "_CWD=%~dp0"

SET "CMAKE_GENERATOR=Visual Studio 17 2022"
SET "CMAKE_ARCHITECTURE=x64"

CALL :COMPILE %*
GOTO :EOF

:COMPILE
    PUSHD "%_CWD%" >NUL
    IF "%~1"=="VST2" (
        CALL :COMPILE_VST2
    ) ELSE IF "%~1"=="VST3" (
        CALL :COMPILE_VST3
    ) ELSE IF "%~1"=="VST2_DEMO" (
        CALL :COMPILE_VST2_DEMO
    ) ELSE IF "%~1"=="VST3_DEMO" (
        CALL :COMPILE_VST3_DEMO
    ) ELSE (
        START /D "%_CWD%" cmd /C generate-release.bat VST2
        START /D "%_CWD%" cmd /C generate-release.bat VST2_DEMO
        START /D "%_CWD%" cmd /C generate-release.bat VST3
        START /D "%_CWD%" cmd /C generate-release.bat VST3_DEMO
    )
    POPD >NUL
EXIT /B 0

:COMPILE_VST2
    CALL :CMAKE "vst2" -DENABLE_FULL_VERSION=ON -DENABLE_STEINBERG_VST3=OFF -DENABLE_STEINBERG_VST2=ON -DPACKAGE_PREFIX="../build/release/"
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC" ../build/release/vst2/installer-vst2.iss"    
EXIT /B 0

:COMPILE_VST2_DEMO
    CALL :CMAKE "vst2d" -DENABLE_FULL_VERSION=OFF -DENABLE_STEINBERG_VST3=OFF -DENABLE_STEINBERG_VST2=ON -DPACKAGE_PREFIX="../build/release/"
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC" "../build/release/vst2d/installer-vst2.iss"    
EXIT /B 0

:COMPILE_VST3
    CALL :CMAKE "vst3" -DENABLE_FULL_VERSION=ON -DENABLE_STEINBERG_VST3=ON -DENABLE_STEINBERG_VST2=OFF -DPACKAGE_PREFIX="../build/release/"
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC" "../build/release/vst3/installer-vst3.iss"
EXIT /B 0

:COMPILE_VST3_DEMO
    CALL :CMAKE "vst3d" -DENABLE_FULL_VERSION=OFF -DENABLE_STEINBERG_VST3=ON -DENABLE_STEINBERG_VST2=OFF -DPACKAGE_PREFIX="../build/release/"
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC" "../build/release/vst3d/installer-vst3.iss"
EXIT /B 0

:CMAKE
    SET "NAME=%~1"
    :: Shift first argument away
    set "_args=%*"
    set "_args=!_args:*%1 =!"

    cmake ^
        -H".." -B"../build/release/!NAME!" ^
        -G "%CMAKE_GENERATOR%" -A "%CMAKE_ARCHITECTURE%" ^
        -DCMAKE_INSTALL_PREFIX="../build/distrib/!NAME!" ^
        !_args!
    cmake --build "../build/release/!NAME!" --config Release --target INSTALL    
EXIT /B 0
