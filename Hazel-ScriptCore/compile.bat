@echo off
setlocal

set "CSC_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\Roslyn\csc.exe"
set "OUTPUT_DIR=..\Hazelnut\Resources\Scripts"
set "OUTPUT_DLL=%OUTPUT_DIR%\Hazel-ScriptCore.dll"

:: Create output directory if it doesn't exist
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

:: Compile
"%CSC_PATH%" /target:library /out:"%OUTPUT_DLL%" /platform:x64 /optimize- /debug /langversion:latest ^
    Source\Hazel\Input.cs ^
    Source\Hazel\InternalCalls.cs ^
    Source\Hazel\KeyCode.cs ^
    Source\Hazel\Vector2.cs ^
    Source\Hazel\Vector3.cs ^
    Source\Hazel\Vector4.cs ^
    Source\Hazel\Scene\Components.cs ^
    Source\Hazel\Scene\Entity.cs

if %errorlevel% equ 0 (
    echo Build successful! Output: %OUTPUT_DLL%
) else (
    echo Build failed with error code %errorlevel%
    exit /b %errorlevel%
)
