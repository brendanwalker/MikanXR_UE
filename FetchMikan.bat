@echo off
call %~dp0SetMikanVars_x64.bat

echo "Clear Output Binaries"
del "%~dp0Binaries\Win64\*.dll"
del "%~dp0Binaries\Win64\*.pdb"

echo "Clear Old Headers"
del "%~dp0ThirdParty\MikanXR\include\*.h"

echo "Copy Mikan DLLs"
copy "%MIKAN_DIST_PATH%\MikanAPI.dll" "%~dp0ThirdParty\MikanXR\bin\win64"
copy "%MIKAN_DIST_PATH%\MikanCore.dll" "%~dp0ThirdParty\MikanXR\bin\win64"
copy "%MIKAN_DIST_PATH%\SpoutLibrary.dll" "%~dp0ThirdParty\MikanXR\bin\win64"
IF %ERRORLEVEL% NEQ 0 (
  echo "Error copying DLLs"
  goto failure
)

echo "Copy Mikan header files"
copy "%MIKAN_DIST_PATH%\include\*.h" "%~dp0ThirdParty\MikanXR\include"
IF %ERRORLEVEL% NEQ 0 (
  echo "Error copying header files"
  goto failure
)

echo "Copy Mikan Libs"
copy "%MIKAN_DIST_PATH%\lib\MikanAPI.lib" "%~dp0ThirdParty\MikanXR\lib\win64\MikanAPI.lib"
copy "%MIKAN_DIST_PATH%\lib\MikanCore.lib" "%~dp0ThirdParty\MikanXR\lib\win64\MikanCore.lib"
IF %ERRORLEVEL% NEQ 0 (
  echo "Error copying libs"
  goto failure
)


popd
EXIT /B 0

:failure
pause
EXIT /B 1