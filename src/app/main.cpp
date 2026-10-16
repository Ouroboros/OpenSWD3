#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "platform/display.h"
#include "platform/surface.h"

int main(int, char**)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        return 1;
    }

    {
        swd3::Display display;

        if (!display.Open("SWD3"))
        {
            SDL_Quit();
            return 1;
        }

        swd3::Surface screen(swd3::Display::kWidth, swd3::Display::kHeight);
        bool running = true;

        while (running)
        {
            SDL_Event event;

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                }
                else
                {
                    display.HandleEvent(event);
                }
            }

            display.Present(screen);
            SDL_Delay(16);
        }
    }

    SDL_Quit();
    return 0;
}
