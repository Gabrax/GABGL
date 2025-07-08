#!/bin/bash

OS=$(uname)
EXE_NAME="gl_engine"
EXE_EXTENSION=""
EXE_PATH="$EXE_NAME$EXE_EXTENSION"

RED="\e[31m"
GREEN="\e[32m"
YELLOW="\e[33m"
RESET="\e[0m"

COMPILER=""
BUILD_TYPE="Release"

for arg in "$@"; do
    case "$arg" in
        -CMSVC|-Cmsvc|-cmsvc) COMPILER="msvc" ;;
        -CGCC|-Cgcc|-cgcc) COMPILER="gcc" ;;
        -CCLANG|-Cclang|-cclang) COMPILER="clang" ;;
        -Ccl|-CCL|-ccl) COMPILER="cl" ;;
        -DRELEASE|-Drelease|-drelease) BUILD_TYPE="Release" ;;
        -DDEBUG|-Ddebug|-ddebug) BUILD_TYPE="RelWithDebInfo" ;;
        --help|-h)
            echo -e "${YELLOW}Usage:${RESET} ./build.sh [options]"
            echo -e "\n${YELLOW}Options:${RESET}"
            echo -e "  -C{gcc|clang|msvc|cl}  Choose compiler"
            echo -e "  -D{debug|release}      Choose build mode"
            echo -e "  --help, -h             Show this help"
            exit 0
            ;;
        *)
            echo -e "${RED}[!] Unknown argument: $arg${RESET}"
            ;;
    esac
done

# OS detection
if [[ "$OS" == "Linux" || "$OS" == "Darwin" ]]; then
    EXE_EXTENSION=""
    echo -e "${YELLOW}[*] Running on Linux or macOS${RESET}"
elif [[ "$OS" == MINGW* || "$OS" == CYGWIN* || "$OS" == MSYS* ]]; then
    EXE_EXTENSION=".exe"
    echo -e "${YELLOW}[*] Running on Windows${RESET}"
else
    echo -e "${RED}Unsupported OS: $OS${RESET}"
    exit 1
fi

EXE_PATH="$EXE_NAME$EXE_EXTENSION"
BUILD_DIR="build"
ROOT_DIR=$(pwd)

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || { echo -e "${RED}[*] Failed to enter build directory${RESET}"; exit 1; }

# If previous config used a different build type, force reconfigure
RECONFIGURE=false
if [[ -f "CMakeCache.txt" ]]; then
    PREV_TYPE=$(grep CMAKE_BUILD_TYPE CMakeCache.txt | cut -d= -f2)
    if [[ "$PREV_TYPE" != "$BUILD_TYPE" ]]; then
        echo -e "${YELLOW}[*] Switching build type from $PREV_TYPE to $BUILD_TYPE...${RESET}"
        RECONFIGURE=true
    fi
fi

# Configure the project
if [[ "$RECONFIGURE" == true || ! -f "CMakeCache.txt" ]]; then
    echo -e "${YELLOW}[*] Configuring project with CMake ($BUILD_TYPE)...${RESET}"
    rm -f CMakeCache.txt

    CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

    case "$COMPILER" in
        "")
            cmake .. $CMAKE_ARGS || { echo -e "${RED}[*] CMake configuration failed${RESET}"; exit 1; }
            ;;
        cl)
            cmake -G "Ninja" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. || exit 1
            ;;
        msvc)
            cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=$BUILD_TYPE .. || exit 1
            ;;
        clang)
            cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ $CMAKE_ARGS -Wno-dev .. || exit 1
            ;;
        gcc)
            cmake -G "Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ $CMAKE_ARGS -Wno-dev .. || exit 1
            ;;
        *)
            echo -e "${RED}[*] Unsupported compiler: $COMPILER${RESET}"
            exit 1
            ;;
    esac
else
    echo -e "${GREEN}[*] Project already configured for $BUILD_TYPE${RESET}"
fi

# Build
echo -e "${YELLOW}[*] Building ($BUILD_TYPE)...${RESET}"
cmake --build . --config "$BUILD_TYPE" || { echo -e "${RED}[*] Build failed${RESET}"; exit 1; }

# Find executable
echo -e "${YELLOW}[*] Looking for $EXE_PATH...${RESET}"
FOUND_EXE=$(find . -type f -name "$EXE_PATH" | head -n 1)

if [[ -z "$FOUND_EXE" ]]; then
    echo -e "${RED}[*] Executable $EXE_PATH not found${RESET}"
    exit 1
fi

# Move to root if not already
if [[ "$ROOT_DIR/$EXE_PATH" != "$FOUND_EXE" ]]; then
    echo -e "${YELLOW}[*] Moving executable to root...${RESET}"
    mv "$FOUND_EXE" "$ROOT_DIR/$EXE_PATH" || { echo -e "${RED}[*] Failed to move $EXE_PATH${RESET}"; exit 1; }
fi

if [[ "$BUILD_TYPE" == "RelWithDebInfo" ]]; then
    PDB_FILE_NAME="${EXE_NAME}.pdb"
    echo -e "${YELLOW}[*] Looking for debug symbols: $PDB_FILE_NAME...${RESET}"
    FOUND_PDB=$(find . -type f -name "$PDB_FILE_NAME" | head -n 1)

    if [[ -n "$FOUND_PDB" ]]; then
        if [[ "$ROOT_DIR/$PDB_FILE_NAME" != "$FOUND_PDB" ]]; then
            echo -e "${YELLOW}[*] Moving $PDB_FILE_NAME to root...${RESET}"
            mv "$FOUND_PDB" "$ROOT_DIR/$PDB_FILE_NAME" || { echo -e "${RED}[*] Failed to move $PDB_FILE_NAME${RESET}"; exit 1; }
        else
            echo -e "${GREEN}[*] $PDB_FILE_NAME is already in the root directory${RESET}"
        fi
    else
        echo -e "${YELLOW}[*] No PDB file found (not generated or already moved)${RESET}"
    fi
fi

cd "$ROOT_DIR"
echo -e "${GREEN}[*] Build completed. Running ./$EXE_PATH${RESET}"
"./$EXE_PATH"
