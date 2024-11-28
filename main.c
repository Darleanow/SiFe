#include <SDL2/SDL.h>

#undef main

#include <SDL_opengl.h>
#include <stdio.h>
#include "Renderer.h"
#include "microui.h"
#include "Constants.h"
#include "Logger.h"
#include "UiState.h"
#include "Menu.h"

SDL_Window *window = NULL;
static UIState ui_state;
static float scale_factor = 1.0f;

// Input handling constants and mappings
#define KEY_MAP_MASK 0xff

static const char button_map[256] = {
        [SDL_BUTTON_LEFT & KEY_MAP_MASK] = MU_MOUSE_LEFT,
        [SDL_BUTTON_RIGHT & KEY_MAP_MASK] = MU_MOUSE_RIGHT,
        [SDL_BUTTON_MIDDLE & KEY_MAP_MASK] = MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
        [SDLK_LSHIFT & KEY_MAP_MASK] = MU_KEY_SHIFT,
        [SDLK_RSHIFT & KEY_MAP_MASK] = MU_KEY_SHIFT,
        [SDLK_LCTRL & KEY_MAP_MASK] = MU_KEY_CTRL,
        [SDLK_RCTRL & KEY_MAP_MASK] = MU_KEY_CTRL,
        [SDLK_LALT & KEY_MAP_MASK] = MU_KEY_ALT,
        [SDLK_RALT & KEY_MAP_MASK] = MU_KEY_ALT,
        [SDLK_RETURN & KEY_MAP_MASK] = MU_KEY_RETURN,
        [SDLK_BACKSPACE & KEY_MAP_MASK] = MU_KEY_BACKSPACE,
};

static void process_frame(mu_Context *ctx) {
    mu_begin(ctx);

    calculate_responsive_dimensions(&ui_state);

    if (ui_state.menu_open && ui_state.menu_animation < 1.0f) {
        ui_state.menu_animation += MENU_ANIMATION_STEP;
        if (ui_state.menu_animation > 1.0f) ui_state.menu_animation = 1.0f;
    } else if (!ui_state.menu_open && ui_state.menu_animation > 0.0f) {
        ui_state.menu_animation -= MENU_ANIMATION_STEP;
        if (ui_state.menu_animation < 0.0f) ui_state.menu_animation = 0.0f;
    }

    if (ui_state.menu_animation > 0.0f) {
        draw_menu(ctx, &ui_state);
    } else {
        draw_menu_button(ctx, &ui_state);
    }

    mu_end(ctx);
}

static int text_width(mu_Font font, const char *text, int len) {
    if (len == -1) { len = strlen(text); }
    return r_get_text_width(text, len);
}

static int text_height(mu_Font font) {
    return r_get_text_height();
}

static void render_commands(mu_Context *ctx) {
    mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
        switch (cmd->type) {
            case MU_COMMAND_TEXT:
                r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
                break;
            case MU_COMMAND_RECT:
                r_draw_rect(cmd->rect.rect, cmd->rect.color);
                break;
            case MU_COMMAND_ICON:
                r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
                break;
            case MU_COMMAND_CLIP:
                r_set_clip_rect(cmd->clip.rect);
                break;
        }
    }
}

static void handle_event(SDL_Event *e, mu_Context *ctx, int *running, UIState *state) {
    switch (e->type) {
        case SDL_QUIT:
            *running = 0;
            break;
            
        case SDL_WINDOWEVENT:
            switch (e->window.event) {
                case SDL_WINDOWEVENT_MOVED:
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_EXPOSED: {
                    int width, height;
                    #ifdef __APPLE__
                        SDL_GL_GetDrawableSize(window, &width, &height);
                        
                        // Update scale factor on window resize
                        int window_w, window_h;
                        SDL_GetWindowSize(window, &window_w, &window_h);
                        scale_factor = (float)width / window_w;
                    #else
                        SDL_GetWindowSize(window, &width, &height);
                    #endif
                    
                    if (width != state->window_width || height != state->window_height) {
                        ui_state_update_dimensions(state, width, height);
                        r_update_dimensions(width, height);
                        r_clear(mu_color(state->bg_color[0], state->bg_color[1], state->bg_color[2], 255));
                        process_frame(ctx);
                        render_commands(ctx);
                        r_present();
                    }
                }
                    break;
            }
            break;

        case SDL_MOUSEMOTION: {
            #ifdef __APPLE__
                mu_input_mousemove(ctx, e->motion.x * scale_factor, e->motion.y * scale_factor);
            #else
                mu_input_mousemove(ctx, e->motion.x, e->motion.y);
            #endif
            break;
        }

        case SDL_MOUSEWHEEL:
            mu_input_scroll(ctx, 0, e->wheel.y * -30);
            break;

        case SDL_TEXTINPUT:
            mu_input_text(ctx, e->text.text);
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            int b = button_map[e->button.button & KEY_MAP_MASK];
            #ifdef __APPLE__
                if (b && e->type == SDL_MOUSEBUTTONDOWN) {
                    mu_input_mousedown(ctx, e->button.x * scale_factor, e->button.y * scale_factor, b);
                }
                if (b && e->type == SDL_MOUSEBUTTONUP) {
                    mu_input_mouseup(ctx, e->button.x * scale_factor, e->button.y * scale_factor, b);
                }
            #else
                if (b && e->type == SDL_MOUSEBUTTONDOWN) {
                    mu_input_mousedown(ctx, e->button.x, e->button.y, b);
                }
                if (b && e->type == SDL_MOUSEBUTTONUP) {
                    mu_input_mouseup(ctx, e->button.x, e->button.y, b);
                }
            #endif
            break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            int c = key_map[e->key.keysym.sym & KEY_MAP_MASK];
            if (c && e->type == SDL_KEYDOWN) { mu_input_keydown(ctx, c); }
            if (c && e->type == SDL_KEYUP) { mu_input_keyup(ctx, c); }
            break;
        }
    }
}

static void cleanup(mu_Context *ctx) {
    free(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char **argv) {
    #ifdef __APPLE__
        // Set up SDL for macOS
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);

        // Add these lines for Retina display support
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    #endif

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    ui_state_init(&ui_state);
    logger_init();

    window = SDL_CreateWindow(
            TITLE_TEXT,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            ui_state.window_width, ui_state.window_height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Initialize scale factor for Retina displays
    #ifdef __APPLE__
        int drawable_w, drawable_h, window_w, window_h;
        SDL_GL_GetDrawableSize(window, &drawable_w, &drawable_h);
        SDL_GetWindowSize(window, &window_w, &window_h);
        scale_factor = (float)drawable_w / window_w;
    #endif

    if (r_init() != 0) {
        fprintf(stderr, "Renderer initialization failed\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    mu_Context *ctx = malloc(sizeof(mu_Context));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate memory for mu_Context\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    mu_init(ctx);
    ctx->text_width = text_width;
    ctx->text_height = text_height;

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            handle_event(&e, ctx, &running, &ui_state);
        }

        process_frame(ctx);

        r_clear(mu_color(ui_state.bg_color[0], ui_state.bg_color[1], ui_state.bg_color[2], 255));
        render_commands(ctx);
        r_present();

        SDL_Delay(FRAME_DELAY_MS);
    }

    cleanup(ctx);
    return 0;
}