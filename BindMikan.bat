@echo off
setlocal

::Select the path to the root Boost folder
if DEFINED MIKAN_DIST_PATH (goto setup_copy_script)
set "psCommand="(new-object -COM 'Shell.Application')^
.BrowseForFolder(0,'Please select the root folder for Mikan github build (ex: C:\MikanXR\dist\Win64\Debug).',0,0).self.path""
for /f "usebackq delims=" %%I in (`powershell %psCommand%`) do set "MIKAN_DIST_PATH=%%I"
if NOT DEFINED MIKAN_DIST_PATH (goto failure)

:setup_copy_script

:: Write out the paths to a batch file
del SetMikanVars_x64.bat
echo @echo off >> SetMikanVars_x64.bat
echo set "MIKAN_DIST_PATH=%MIKAN_DIST_PATH%" >> SetMikanVars_x64.bat

:: Copy latest Mikan libs
call FetchMikan.bat || goto failure
EXIT /B 0

:failure
pause
EXIT /B 1