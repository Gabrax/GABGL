#!/bin/bash


RED="\e[31m"
GREEN="\e[32m"
YELLOW="\e[33m"
RESET="\e[0m"

BUILD_DIR="build"
ROOT_DIR=$(pwd)

if [ -d "$BUILD_DIR" ]; then
    echo -e "${GREEN}Build directory exists.${RESET}"
else
    echo -e "${YELLOW}Build directory does not exist. Creating it now...${RESET}"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR" || { echo -e "${RED}Failed to navigate to build directory${RESET}"; exit 1; }

if [ -f "CMakeCache.txt" ]; then
    echo -e "${GREEN}Project is already configured.${RESET}"
else
    echo -e "${YELLOW}Configuring the project with CMake...${RESET}"
    cmake .. || { echo -e "${RED}CMake configuration failed${RESET}"; exit 1; }
    # cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. || { echo -e "${RED}CMake configuration failed${RESET}"; exit 1; }
fi

echo -e "${YELLOW}Building the project...${RESET}"
cmake --build . || { echo -e "${RED}Build failed${RESET}"; exit 1; }
echo -e "${GREEN}Build completed successfully.${RESET}"

echo -e "${YELLOW}Searching for mygame.exe in the build directory...${RESET}"
if mygame_path=$(find . -type f -name "mygame.exe" -print -quit); then
    echo -e "${GREEN}Executable mygame.exe found at: $mygame_path${RESET}"
    
    # Move the executable to the root directory
    echo -e "${YELLOW}Moving mygame.exe to the root directory...${RESET}"
    mv "$mygame_path" "$ROOT_DIR" || { echo -e "${RED}Failed to move mygame.exe${RESET}"; exit 1; }

    # Run the executable from the root directory
    echo -e "${YELLOW}Running mygame.exe from the root directory...${RESET}"
    (cd "$ROOT_DIR" && "./mygame.exe") || { echo -e "${RED}Failed to run mygame.exe${RESET}"; exit 1; }
else
    echo -e "${RED}Executable mygame.exe not found.${RESET}"
fi
