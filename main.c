#include <SDL2/SDL.h>
#undef main
#include <SDL_opengl.h>
#include "renderer.h"
#include "microui.h"
#include "constants.h"
#include "logger.h"
#include "ui_state.h"
#include "menu.h"

SDL_Window *window = NULL;
static UIState ui_state;

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
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_EXPOSED:
                    int width, height;
                    SDL_GetWindowSize(window, &width, &height);
                    ui_state_update_dimensions(state, width, height);
                    r_update_dimensions(width, height);  // Update renderer dimensions

                    // Clear with background color
                    r_clear(mu_color(state->bg_color[0], state->bg_color[1], state->bg_color[2], 255));

                    // Process and render a new frame
                    process_frame(ctx);
                    render_commands(ctx);

                    // Present the result
                    r_present();
                    break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    r_clear(mu_color(state->bg_color[0], state->bg_color[1], state->bg_color[2], 255));
                    process_frame(ctx);
                    r_present();
                    break;
            }
            break;

        case SDL_MOUSEMOTION:
            mu_input_mousemove(ctx, e->motion.x, e->motion.y);
            break;

        case SDL_MOUSEWHEEL:
            mu_input_scroll(ctx, 0, e->wheel.y * -30);
            break;

        case SDL_TEXTINPUT:
            mu_input_text(ctx, e->text.text);
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            int b = button_map[e->button.button & KEY_MAP_MASK];
            if (b && e->type == SDL_MOUSEBUTTONDOWN) { mu_input_mousedown(ctx, e->button.x, e->button.y, b); }
            if (b && e->type == SDL_MOUSEBUTTONUP) { mu_input_mouseup(ctx, e->button.x, e->button.y, b); }
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
    SDL_Init(SDL_INIT_EVERYTHING);

    ui_state_init(&ui_state);
    logger_init();

    window = SDL_CreateWindow(
            TITLE_TEXT,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            ui_state.window_width, ui_state.window_height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );

    r_init();

    mu_Context *ctx = malloc(sizeof(mu_Context));
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