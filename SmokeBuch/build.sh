#!/bin/bash

defines="-DENGINE"
warnings="-Wno-writable-strings -Wno-format-security -Wno-c++11-extensions -Wno-deprecated-declarations"
includes="-Ivendor"
timestamp=$(date +%s)

if [[ "$(uname)" == "Linux" ]]; then
    echo "Running on Linux"
    libs="-lX11 -lGL -lfreetype"
    outputFile=app
    queryProcesses=$(pgrep $outputFile)

    # fPIC position independent code
    rm -f game_* # Remove old game_* files
    clang++ -g "src/game.cpp" -shared -fPIC -o game_$timestamp.so $warnings $defines
    mv game_$timestamp.so game.so

elif [[ "$(uname)" == "Darwin" ]]; then
    echo "Running on Mac"
    libs="-framework Cocoa"
    sdkpath=$(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
    includes="-Ivendor -isysroot ${sdkpath} -I${sdkpath}/System/Library/Frameworks/Cocoa.framework/Headers"
    objc_dep="src/mac_platform.m"
    outputFile=app
    # clean up old object files
    rm -f src/*.o
else
    echo "Running on Windows"
    libs="-luser32 -lgdi32 -lopengl32 -lole32 -Lvendor/lib -lfreetype.lib"
    outputFile=app.exe
    queryProcesses=$(tasklist | grep $outputFile)

    rm -f game_* # Remove old game_* files
    clang++ -g "src/game.cpp" -shared -o game_$timestamp.dll $warnings $defines
    mv game_$timestamp.dll game.dll
fi

processRunning=$queryProcesses

if [ -z "$processRunning" ]; then
    echo "Engine not running, building main..."
    clang++ $includes -g "src/main.cpp" $objc_dep -o $outputFile $libs $warnings $defines
    # Check if the compilation was successful
    if [ $? -eq 0 ]; then
        echo "Compilation successful."
        echo " "
        
        ./$outputFile
    else
        echo "Compilation failed."
    fi
else
    echo "Hot Reload!"
fi

