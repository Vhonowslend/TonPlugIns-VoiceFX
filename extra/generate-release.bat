SET _CWD=%~dp0

PUSHD "%_CWD%" >NUL
START /D "%_CWD%" generate-release-vst3.bat
START /D "%_CWD%" generate-release-vst2.bat
POPD >NUL
