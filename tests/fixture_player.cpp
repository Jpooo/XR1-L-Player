// Regression executable only: never installed or included in release packages.
#include "capture_fixture.hpp"
#define main fixture_application_main
#include "../src/main.cpp"
#undef main
#undef Capture
#undef videoDevices
#undef SDL_GetNumAudioDevices
#undef SDL_GetAudioDeviceName
int main(int argc,char **argv){fixture::enabled=true;return fixture_application_main(argc,argv);}
