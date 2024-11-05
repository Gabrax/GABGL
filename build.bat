@echo off
setlocal enabledelayedexpansion

for /f %%a in ('echo prompt $E^| cmd') do set "ESC=%%a"

set "EXE_NAME=mygame"
set "EXE_EXTENSION=.exe"
set "EXE_PATH=%EXE_NAME%%EXE_EXTENSION%"

REM Set colors
set "GREEN=%ESC%[0;32m"
set "YELLOW=%ESC%[0;33m"
set "RED=%ESC%[0;31m"
set "RESET=%ESC%[0m"

REM Set build directory
set "BUILD_DIR=build"
set "ROOT_DIR=%cd%"

REM Check if build directory exists
if exist "%BUILD_DIR%" (
    echo !GREEN![*] Build directory exists %ESC%[0m
) else (
    echo !YELLOW![*] Build directory does not exist. Creating it now... %ESC%[0m
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%" || (
    echo !RED![*] Failed to navigate to build directory %ESC%[0m
    exit /b 1
)

REM Check if project is already configured
if exist "CMakeCache.txt" (
    echo !GREEN![*] Project is already configured %ESC%[0m
) else (
    echo !YELLOW![*] Configuring the project with CMake... %ESC%[0m
    cmake .. || (
        echo !RED![*] CMake configuration failed %ESC%[0m
        exit /b 1
    )
)

REM Build the project
echo !YELLOW![*] Building the project... %ESC%[0m
cmake --build . || (
    echo !RED![*] Build failed %ESC%[0m
    exit /b 1
)
echo !GREEN![*] Build completed successfully %ESC%[0m

REM Search for the executable in the build directory
echo !YELLOW![*] Searching for %EXE_PATH% in the build directory... %ESC%[0m
for /r %ROOT_DIR%\build\Debug %%f in (%EXE_PATH%) do (
    set "mygame_path=%%f"
    echo !GREEN![*] Executable %EXE_PATH% found at: !mygame_path! %ESC%[0m
    
    REM Check if the executable is already in the root directory
    if not "!mygame_path!"=="%ROOT_DIR%\%EXE_PATH%" (
        echo !YELLOW![*] Moving %EXE_PATH% to the root directory... %ESC%[0m
        move "!mygame_path!" "%ROOT_DIR%" || (
            echo !RED![*] Failed to move %EXE_PATH% %ESC%[0m
            exit /b 1
        )
    ) else (
        echo !GREEN![*] %EXE_PATH% is already in the root directory %ESC%[0m
    )

    REM Change to the root directory and run the executable
    echo !YELLOW![*] Running %EXE_PATH%... %ESC%[0m
    cd /d "%ROOT_DIR%" && "%EXE_PATH%" || (
        echo !RED![*] Failed to run %EXE_PATH%! %ESC%[0m
        exit /b 1
    )
    exit /b 0
)

echo !RED![*] Executable %EXE_PATH% not found! %ESC%[0m
exit /b 1
