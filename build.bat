@echo off
setlocal enabledelayedexpansion

REM OS Detection
for /f "delims=" %%i in ('ver') do set "OS_VER=%%i"
echo %OS_VER% | find /i "Windows" >nul
if errorlevel 1 (
    echo [*] Unsupported OS: %OS_VER%
    exit /b 1
) else (
    set "EXE_EXTENSION=.exe"
    echo [*] Running on Windows
)

set "EXE_NAME=mygame"
set "EXE_PATH=%EXE_NAME%%EXE_EXTENSION%"

REM Colors
for /f %%a in ('echo prompt $E^| cmd') do set "ESC=%%a"
set "RED=%ESC%[0;31m"
set "GREEN=%ESC%[0;32m"
set "YELLOW=%ESC%[0;33m"
set "RESET=%ESC%[0m"

set "BUILD_DIR=build"
set "ROOT_DIR=%cd%"
set "SEARCH_PATH=%ROOT_DIR%\%BUILD_DIR%"

REM Compiler (passed as an argument)
set "COMPILER=%1"
if "%COMPILER%"=="" (
    echo !YELLOW![*] Using the default system compiler...!RESET!
) else (
    echo !YELLOW![*] Compiler specified: %COMPILER%!RESET!
    if /i "%COMPILER%"=="msvc" (
        set "SEARCH_PATH=%ROOT_DIR%\%BUILD_DIR%\Debug"
    )
)

REM Check if build directory exists
if exist "%BUILD_DIR%" (
    echo !GREEN![*] Build directory exists.!RESET!
) else (
    echo !YELLOW![*] Build directory does not exist. Creating it now...!RESET!
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%" || (
    echo !RED![*] Failed to navigate to build directory.!RESET!
    exit /b 1
)

REM Check if project is already configured
if exist "CMakeCache.txt" (
    echo !GREEN![*] Project is already configured.!RESET!
) else (
    echo !YELLOW![*] Configuring the project with CMake...!RESET!
    if "%COMPILER%"=="" (
        cmake .. || (
            echo !RED![*] CMake configuration failed.!RESET!
            exit /b 1
        )
    ) else if /i "%COMPILER%"=="msvc" (
        cmake -G "Visual Studio 17 2022" .. || (
            echo !RED![*] CMake configuration failed.!RESET!
            exit /b 1
        )
    ) else if /i "%COMPILER%"=="clang" (
        cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. || (
            echo !RED![*] CMake configuration failed.!RESET!
            exit /b 1
        )
    ) else if /i "%COMPILER%"=="gcc" (
        cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .. || (
            echo !RED![*] CMake configuration failed.!RESET!
            exit /b 1
        )
    ) else (
        echo !RED![*] Unsupported compiler: %COMPILER%!RESET!
        exit /b 1
    )
)

REM Build the project
echo !YELLOW![*] Building the project...!RESET!
cmake --build . || (
    echo !RED![*] Build failed.!RESET!
    exit /b 1
)
echo !GREEN![*] Build completed successfully.!RESET!

REM Search for the executable
echo !YELLOW![*] Searching for %EXE_PATH% in the directory: %SEARCH_PATH%...!RESET!
for /r "%SEARCH_PATH%" %%f in ("%EXE_PATH%") do (
    set "mygame_path=%%f"
    echo !GREEN![*] Executable %EXE_PATH% found at: !mygame_path!!RESET!

    if not "!mygame_path!"=="%ROOT_DIR%\%EXE_PATH%" (
        echo !YELLOW![*] Moving %EXE_PATH% to the root directory...!RESET!
        move "!mygame_path!" "%ROOT_DIR%" || (
            echo !RED![*] Failed to move %EXE_PATH%.!RESET!
            exit /b 1
        )
    ) else (
        echo !GREEN![*] %EXE_PATH% is already in the root directory.!RESET!
    )

    echo !YELLOW![*] Running %EXE_PATH%...!RESET!
    cd /d "%ROOT_DIR%" && "%EXE_PATH%" || (
        echo !RED![*] Failed to run %EXE_PATH%!RESET!
        exit /b 1
    )
    exit /b 0
)

echo !RED![*] Executable %EXE_PATH% not found in %SEARCH_PATH%!RESET!
exit /b 1

