CXX ?= g++
DEPS ?= .deps/root/usr
CPPFLAGS += -I$(DEPS)/include/SDL2 -I$(DEPS)/include -I$(DEPS)/include/x86_64-linux-gnu
CXXFLAGS ?= -O3 -DNDEBUG
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic -pthread
LDLIBS = -l:libSDL2-2.0.so.0 -l:libSDL2_ttf-2.0.so.0 -l:libjpeg.so.62
LDFLAGS += -L$(DEPS)/lib/x86_64-linux-gnu
UI_HEADERS = src/menu_typography.hpp src/window_icon.hpp src/connection_recovery.hpp src/menu_motion.hpp src/ui.hpp src/menu_theme.hpp src/menu_ui.hpp src/frame_cadence.hpp
VIDEO_HEADERS = src/video.hpp src/video_common.hpp src/video_linux.hpp src/video_windows.hpp
all: build/xr1-l-player
build/xr1-l-player: src/main.cpp src/core.hpp src/frame_cadence.hpp $(VIDEO_HEADERS) src/audio.hpp $(UI_HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/main.cpp $(LDFLAGS) $(LDLIBS) -o $@
clean:
	rm -rf build
.PHONY: all clean
build/capture-test: tests/capture_test.cpp src/core.hpp src/frame_cadence.hpp $(VIDEO_HEADERS) src/audio.hpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) -O2 -g -std=c++17 -Wall -Wextra -pthread tests/capture_test.cpp $(LDFLAGS) $(LDLIBS) -Wl,--wrap=ioctl -Wl,--wrap=poll -o $@
test: build/capture-test
	LD_LIBRARY_PATH=$(DEPS)/lib/x86_64-linux-gnu ./build/capture-test
.PHONY: test
build/audio-test: tests/audio_test.cpp src/core.hpp src/frame_cadence.hpp src/audio.hpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) -O2 -std=c++17 -Wall -Wextra -pthread tests/audio_test.cpp $(LDFLAGS) $(LDLIBS) -o $@
build/render-test: tests/render_test.cpp src/core.hpp src/frame_cadence.hpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) -O2 -std=c++17 -Wall -Wextra -pthread tests/render_test.cpp $(LDFLAGS) $(LDLIBS) -o $@
build/controls-probe.so: tests/controls_probe.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) -O2 -std=c++17 -Wall -Wextra -shared -fPIC $< -ldl -o $@
controls-test: build/xr1-l-player build/controls-probe.so
	python3 tests/controls_test.py
.PHONY: controls-test
build/handoff-test: tests/handoff_test.cpp src/core.hpp src/frame_cadence.hpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) -O2 -g -std=c++17 -Wall -Wextra -pthread $< $(LDFLAGS) $(LDLIBS) -o $@
