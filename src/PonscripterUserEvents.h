#define ONS_SOUND_EVENT (SDL_USEREVENT + 1)
#define ONS_MIDI_EVENT (SDL_USEREVENT + 2)
#define ONS_WAVE_EVENT (SDL_USEREVENT + 3)
#define ONS_MUSIC_EVENT (SDL_USEREVENT + 4)

#define INTERNAL_REDRAW_EVENT (SDL_USEREVENT + 5)

// This sets up the fadeout event flag for use in mp3 fadeout.  Recommend for integration.  [Seung Park, 20060621]
#define ONS_FADE_EVENT (SDL_USEREVENT + 6)
