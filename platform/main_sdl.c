#include "rb_platform.h"
#include "rb_keycodes.h"
#include "rb_types.h"

#include <SDL.h>

// --- forward declarations ----------------------------------------------------

extern void jupiter_main_loop(void);

// --- globals -----------------------------------------------------------------

static SDL_Texture* s_texture = NULL;
static SDL_Window* s_window = NULL;
static SDL_Renderer* s_renderer = NULL;
static UINT32* s_pixels = NULL;

#define WIDTH       256
#define HEIGHT      192
#define HEARTBEAT   16

// --- pixel support -----------------------------------------------------------

static void create_pixel_buffer(SDL_Renderer* r) {
    s_pixels = malloc(WIDTH * HEIGHT * sizeof(UINT32));
    s_texture = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
}

void platform_set_pixel(int x, int y, UINT32 color) {
    if (x < 0 || y < 0 || x > WIDTH-1 || y > HEIGHT-1) return;
    s_pixels[y * WIDTH + x] = color;    
}

// --- platform support --------------------------------------------------------

void platform_render_frame(void) {
    SDL_UpdateTexture(s_texture, NULL, s_pixels, WIDTH * sizeof(Uint32));
    SDL_RenderCopy(s_renderer, s_texture, NULL, NULL);
    SDL_RenderPresent(s_renderer);

    platform_log("Frame rendered");
}

static int process_event() {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_TEXTINPUT:
                return e.text.text[0];
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:       return KEY_ESC;
                    case SDLK_RETURN:       return KEY_ENTER;
                    
                    case SDLK_F1:           return KEY_F1;
                    case SDLK_F2:           return KEY_F2;
                    case SDLK_F3:           return KEY_F3;
                    case SDLK_F4:           return KEY_F4;
                    case SDLK_F5:           return KEY_F5;
                    case SDLK_F6:           return KEY_F6;
                    case SDLK_F7:           return KEY_F7;
                    case SDLK_F8:           return KEY_F8;
                    case SDLK_F9:           return KEY_F9;
                    case SDLK_F10:          return KEY_F10;

                    case SDLK_BACKSPACE:    return KEY_BACKSPACE;
                    case SDLK_DELETE:       return KEY_DELETE;
                    case SDLK_TAB:          return KEY_TAB;
                    case SDLK_HOME:         return KEY_HOME;
                    case SDLK_END:          return KEY_END;
                    case SDLK_PAGEUP:       return KEY_PAGE_UP;
                    case SDLK_PAGEDOWN:     return KEY_PAGE_DOWN;
 
                    case SDLK_LEFT:  return KEY_LEFT;
                    case SDLK_RIGHT: return KEY_RIGHT;
                    case SDLK_UP:    return KEY_UP;
                    case SDLK_DOWN:  return KEY_DOWN;

                }
                break;
            }
    }

    return 0;
}

int platform_get_key(void) {
    SDL_StartTextInput();

    int key = process_event();
    
    SDL_StopTextInput();

    return key;
}

// --- main function -----------------------------------------------------------

int main(int const argc, const char* const argv[], char* envv[]) {
    platform_log("Path: %s\n", argv[0]);

    int sdl_init_flags = SDL_INIT_VIDEO|SDL_INIT_TIMER;

    if (SDL_Init(sdl_init_flags) != 0) {
        platform_err("SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    s_window = SDL_CreateWindow("Jupiter", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!s_window) {
        platform_err("SDL window error: %s\n", SDL_GetError());
        SDL_Quit();

        return 1;
    }

    s_renderer = SDL_CreateRenderer(s_window, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!s_renderer) {
        s_renderer = SDL_CreateRenderer(s_window, -1, 0);
    }

    create_pixel_buffer(s_renderer);

    platform_log("Starting main loop\n");

    // jupiter main loop
    jupiter_main_loop();

    SDL_DestroyTexture(s_texture);

    SDL_ShowCursor(SDL_ENABLE);

    SDL_DestroyRenderer(s_renderer);
    SDL_DestroyWindow(s_window);

    SDL_Quit(); 
    
    return 0;
}
