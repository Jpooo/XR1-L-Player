#include "../src/audio.hpp"
#include <iostream>
int main() {
    if(SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER)<0) return 1;
    {
        Audio audio(20);audio.volume=1;
        audio.start(nullptr,nullptr,false);
        SDL_Delay(500);
        audio.stop();
        std::cout<<audio.mode<<"\nAudio bridge overflow samples: "<<audio.ring.dropped<<" / underflow samples: "<<audio.ring.starved<<"\n";
    }
    SDL_Quit();
}
