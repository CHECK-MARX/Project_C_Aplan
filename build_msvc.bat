@echo off
setlocal ENABLEEXTENSIONS

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if not exist "%VSWHERE%" (
    echo vswhere.exe not found. Install Visual Studio 2019 Build Tools or Community edition.
    exit /b 1
)

set "VSINSTALL="

for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE%" -latest -products * -version [16.0^,17.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSINSTALL=%%I"
)

if not defined VSINSTALL (
    for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE%" -latest -products * -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VSINSTALL=%%I"
    )
)

if not defined VSINSTALL (
    for /f "usebackq tokens=* delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VSINSTALL=%%I"
    )
)

if not defined VSINSTALL (
    echo Unable to locate a Visual Studio installation with required C++ tools.
    exit /b 1
)

set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARS%" (
    call "%VCVARS%"
) else (
    set "VCVARS=%VSINSTALL%\VC\Auxiliary\Build\vcvars32.bat"
    if exist "%VCVARS%" (
        call "%VCVARS%"
    ) else (
        echo Could not locate vcvars32/64.bat under "%VSINSTALL%".
        exit /b 1
    )
)

cl /nologo /utf-8 /TC /W4 /WX- /permissive- /Zc:wchar_t /EHsc- ^
   /DUNICODE /D_UNICODE ^
   src\main.c src\sim.c src\ui.c src\input.c ^
   /link user32.lib gdi32.lib

if errorlevel 1 (
    exit /b %errorlevel%
)

exit /b 0
