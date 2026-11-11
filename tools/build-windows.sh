#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
COMPILER="${CROSS_CXX:-.deps/toolchain/usr/bin/x86_64-w64-mingw32-g++-posix}"
SDK=.deps/win-sdk
mkdir -p build/windows
SOURCE="${1:-src/main.cpp}"
OUTPUT="${2:-build/windows/XR1-L-Player.exe}"
set -- "$SOURCE"
SUBSYSTEM=-mconsole
if [ "$SOURCE" = "src/main.cpp" ] || [ "$SOURCE" = "tests/windows_icons_test.cpp" ]; then
  if [ "$SOURCE" = "src/main.cpp" ]; then SUBSYSTEM=-mwindows; fi
  WINDRES="${CROSS_WINDRES:-.deps/toolchain/usr/bin/x86_64-w64-mingw32-windres}"
  "$WINDRES" --preprocessor="$COMPILER" --preprocessor-arg=-E --preprocessor-arg=-xc --preprocessor-arg=-DRC_INVOKED \
    -Iassets/branding -Isrc -O coff src/windows_app.rc build/windows/windows-app-icon.o
  set -- "$@" build/windows/windows-app-icon.o
fi
"$COMPILER" -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread -DSDL_MAIN_HANDLED -DNOMINMAX -D_WIN32_WINNT=0x0602 \
  -I"$SDK/SDL2-2.32.4/x86_64-w64-mingw32/include/SDL2" \
  -I"$SDK/SDL2_ttf-2.24.0/x86_64-w64-mingw32/include/SDL2" -I"$SDK/mingw64/include" \
  "$@" -o "$OUTPUT" "$SUBSYSTEM" \
  -L"$SDK/SDL2-2.32.4/x86_64-w64-mingw32/lib" -L"$SDK/SDL2_ttf-2.24.0/x86_64-w64-mingw32/lib" -L"$SDK/mingw64/lib" \
  -static-libgcc -static-libstdc++ -lSDL2 -lSDL2_ttf -ljpeg -lmfreadwrite -lmf -lmfplat -lmfuuid -lole32 -lpropsys -luuid -lavrt -lgdi32
