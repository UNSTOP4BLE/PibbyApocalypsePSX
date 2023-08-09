cmake -S . -B ./build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=/c/PSn00bSDK/lib/libpsn00b/cmake/sdk.cmake
cmake --build ./build
/c/Users/Bruno/Documents/duckstation-windows-x64-release/duckstation-qt-x64-ReleaseLTCG.exe ./build/Pibby_Apocalypse_PSX.cue
