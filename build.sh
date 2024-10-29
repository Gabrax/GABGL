#!/bin/bash

BUILD_DIR="build"

if [ -d "$BUILD_DIR" ]; then
    echo "Build directory exists."
else
    echo "Build directory does not exist. Creating it now..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR" || { echo "Failed to navigate to build directory"; exit 1; }

if [ -f "CMakeCache.txt" ]; then
    echo "Project is already configured."
else
    echo "Configuring the project with CMake..."
    cmake .. || { echo "CMake configuration failed"; exit 1; }
    # cmake -G "Ninja" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. || { echo "CMake configuration failed"; exit 1; }
fi

echo "Building the project..."
cmake --build . || { echo "Build failed"; exit 1; }
echo "Build completed successfully."

echo "Searching for mygame.exe in the build directory..."
if mygame_path=$(find . -type f -name "mygame.exe" -print -quit); then
    echo "Executable mygame.exe found at: $mygame_path"
    
    # Run the executable with the working directory set to the location of mygame.exe
    echo "Running mygame.exe..."
    exe_dir=$(dirname "$mygame_path")
    (cd "$exe_dir" && "./$(basename "$mygame_path")") || { echo "Failed to run mygame.exe"; exit 1; }
else
    echo "Executable mygame.exe not found."
fi
