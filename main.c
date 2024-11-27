#include <SDL2/SDL.h>
#undef main
#include <SDL_opengl.h>
#include <stdio.h>
#include "renderer.h"
#include "microui.h"
#include <string.h>

static char logbuf[64000];
static int logbuf_updated = 0;
static float bg[3] = {19, 19, 19};

// Window dimensions
static int window_width = 800;
static int window_height = 600;

// Menu state
static int menu_open = 0;
static float menu_animation = 0.0f;
static int button_width = 130;
static int button_height = 25;

// Store window globally so renderer can access it
SDL_Window *window = NULL;

// Constants for UI dimensions and animation

// Menu dimensions
static const int MENU_WIDTH_RATIO = 4;
static const int MIN_MENU_WIDTH = 200;
static const int HEADER_HEIGHT_RATIO = 15;
static const int MIN_HEADER_HEIGHT = 40;
static const int CLOSE_BUTTON_PADDING = 10;
static const int MIN_LOG_HEIGHT = 100;
static const int LOG_HEIGHT_RATIO = 4;
static const int OPTION_HEIGHT_RATIO = 20;
static const int MIN_OPTION_HEIGHT = 30;
static const int SLIDER_LABEL_WIDTH = 50;

// Button dimensions
static const int BUTTON_WIDTH_RATIO = 6;
static const int MIN_BUTTON_WIDTH = 130;
static const int MAX_BUTTON_WIDTH = 200;
static const int BUTTON_HEIGHT_RATIO = 24;
static const int MIN_BUTTON_HEIGHT = 25;
static const int MAX_BUTTON_HEIGHT = 40;

// Button position ratios
static const int BUTTON_X_RATIO = 80;
static const int BUTTON_Y_RATIO = 60;

// Animation
static const float MENU_ANIMATION_STEP = 0.15f; // Menu animation speed

// Frame delay
static const int FRAME_DELAY_MS = 16; // Delay between frames in milliseconds

// Other constants
static const int DIVIDE_BY_TWO = 2;
static const int SEPARATOR_HEIGHT = 1;
static const int HEADER_SEPARATOR_Y_OFFSET = 5;
static const int MENU_PADDING_X = 10;
static const int MENU_CONTENT_WIDTH_OFFSET = 20;
static const int HEADER_TEXT_PADDING = 10;
static const char *TITLE_TEXT = "SiFe";

// Modified r_init declaration to accept window
void r_init(void);

static void write_log(const char *text) {
    if (logbuf[0]) {
        if (strcat_s(logbuf, sizeof(logbuf), "\n") != 0) {
            fprintf(stderr, "Failed to append newline to log\n");
            return;
        }
    }

    if (strcat_s(logbuf, sizeof(logbuf), text) != 0) {
        fprintf(stderr, "Failed to append text to log\n");
        return;
    }

    logbuf_updated = 1;
}

static void calculate_responsive_dimensions(void) {
    int calculated_menu_width = window_width / MENU_WIDTH_RATIO;
    if (calculated_menu_width < MIN_MENU_WIDTH) calculated_menu_width = MIN_MENU_WIDTH;

    // Button dimensions scale with window size but have minimums and maximums
    button_width = window_width / BUTTON_WIDTH_RATIO;
    if (button_width < MIN_BUTTON_WIDTH) button_width = MIN_BUTTON_WIDTH;
    if (button_width > MAX_BUTTON_WIDTH) button_width = MAX_BUTTON_WIDTH;

    button_height = window_height / BUTTON_HEIGHT_RATIO;
    if (button_height < MIN_BUTTON_HEIGHT) button_height = MIN_BUTTON_HEIGHT;
    if (button_height > MAX_BUTTON_HEIGHT) button_height = MAX_BUTTON_HEIGHT;
}

static void menu_window(mu_Context *ctx) {
    int menu_width = window_width / MENU_WIDTH_RATIO;
    if (menu_width < MIN_MENU_WIDTH) menu_width = MIN_MENU_WIDTH;

    int x_pos = (int) (-menu_width + menu_width * menu_animation);
    x_pos = mu_clamp(x_pos, -menu_width, 0);

    int header_height = window_height / HEADER_HEIGHT_RATIO;
    if (header_height < MIN_HEADER_HEIGHT) header_height = MIN_HEADER_HEIGHT;
    int close_button_size = header_height - CLOSE_BUTTON_PADDING;

    // Calculate log window height
    int log_height = window_height / LOG_HEIGHT_RATIO;
    if (log_height < MIN_LOG_HEIGHT) log_height = MIN_LOG_HEIGHT;

    if (mu_begin_window_ex(ctx, "Open Menu", mu_rect(x_pos, 0, menu_width, window_height),
                           MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL)) {
        mu_Container *win = mu_get_current_container(ctx);
        win->rect.x = x_pos;
        win->rect.w = menu_width;
        win->rect.h = window_height;

        mu_draw_rect(ctx, mu_rect(0, 0, menu_width, window_height), mu_color(30, 30, 30, 255));
        mu_draw_rect(ctx, mu_rect(0, 0, menu_width, header_height), mu_color(40, 40, 40, 255));

        // Header section
        mu_layout_row(ctx, 2, (int[]){menu_width - close_button_size - HEADER_TEXT_PADDING, close_button_size},
                      header_height);

        mu_layout_begin_column(ctx); {
            int title_width = ctx->text_width(ctx->style->font, TITLE_TEXT, strlen(TITLE_TEXT));
            int title_height = ctx->text_height(ctx->style->font);
            int text_x = (menu_width - close_button_size - HEADER_TEXT_PADDING - title_width) / DIVIDE_BY_TWO;
            int text_y = (header_height - title_height) / DIVIDE_BY_TWO;

            mu_draw_text(ctx, ctx->style->font, TITLE_TEXT, strlen(TITLE_TEXT),
                         mu_vec2(text_x, text_y),
                         mu_color(230, 230, 230, 255));
        }
        mu_layout_end_column(ctx);

        mu_layout_begin_column(ctx); {
            int button_padding = (header_height - close_button_size) / DIVIDE_BY_TWO;
            mu_layout_set_next(ctx, mu_rect(menu_width - close_button_size - 5, button_padding,
                                            close_button_size, close_button_size), 0);
            if (mu_button_ex(ctx, "X", 0, MU_OPT_ALIGNCENTER)) {
                menu_open = 0;
            }
        }
        mu_layout_end_column(ctx);

        // Main menu section
        int option_height = window_height / OPTION_HEIGHT_RATIO;
        if (option_height < MIN_OPTION_HEIGHT) option_height = MIN_OPTION_HEIGHT;

        mu_layout_row(ctx, 1, (int[]){-1}, SEPARATOR_HEIGHT);
        mu_draw_rect(ctx, mu_rect(MENU_PADDING_X, header_height + HEADER_SEPARATOR_Y_OFFSET,
                                  menu_width - MENU_CONTENT_WIDTH_OFFSET, SEPARATOR_HEIGHT),
                     ctx->style->colors[MU_COLOR_BORDER]);

        mu_layout_row(ctx, 1, (int[]){-1}, option_height);
        mu_text(ctx, "Menu Options");

        if (mu_button(ctx, "Option 1")) {
            write_log("Selected Option 1");
        }
        if (mu_button(ctx, "Option 2")) {
            write_log("Selected Option 2");
        }
        if (mu_button(ctx, "Option 3")) {
            write_log("Selected Option 3");
        }

        mu_layout_row(ctx, 1, (int[]){-1}, option_height / DIVIDE_BY_TWO);

        mu_layout_row(ctx, 1, (int[]){-1}, SEPARATOR_HEIGHT);
        mu_Rect sep = mu_layout_next(ctx);
        mu_draw_rect(ctx, mu_rect(MENU_PADDING_X, sep.y, menu_width - MENU_CONTENT_WIDTH_OFFSET, SEPARATOR_HEIGHT),
                     ctx->style->colors[MU_COLOR_BORDER]);

        // Color settings section
        mu_layout_row(ctx, 1, (int[]){-1}, option_height);
        mu_label(ctx, "Background Color");

        static float bg_color[3] = {19, 19, 19};
        mu_layout_row(ctx, 2, (int[]){SLIDER_LABEL_WIDTH, -1}, option_height);

        mu_label(ctx, "Red:");
        if (mu_slider(ctx, &bg_color[0], 0, 255)) {
            bg[0] = bg_color[0];
        }

        mu_label(ctx, "Green:");
        if (mu_slider(ctx, &bg_color[1], 0, 255)) {
            bg[1] = bg_color[1];
        }

        mu_label(ctx, "Blue:");
        if (mu_slider(ctx, &bg_color[2], 0, 255)) {
            bg[2] = bg_color[2];
        }

        mu_layout_row(ctx, 1, (int[]){-1}, option_height * 1.5);
        mu_Rect r = mu_layout_next(ctx);
        mu_draw_rect(ctx, r, mu_color(bg_color[0], bg_color[1], bg_color[2], 255));

        char color_text[32];
        sprintf_s(color_text, sizeof(color_text), "#%02X%02X%02X", (int)bg_color[0], (int)bg_color[1], (int)bg_color[2]);
        mu_draw_control_text(ctx, color_text, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);

        // Log window section
        mu_layout_row(ctx, 1, (int[]){-1}, SEPARATOR_HEIGHT);
        mu_Rect log_sep = mu_layout_next(ctx);
        mu_draw_rect(ctx, mu_rect(MENU_PADDING_X, log_sep.y, menu_width - MENU_CONTENT_WIDTH_OFFSET, SEPARATOR_HEIGHT),
                     ctx->style->colors[MU_COLOR_BORDER]);

        mu_layout_row(ctx, 1, (int[]){-1}, option_height);
        mu_text(ctx, "Log Output");

        // Log window with scroll
        mu_layout_row(ctx, 1, (int[]){-1}, log_height);
        mu_begin_panel(ctx, "Log Panel");
        mu_Container *panel = mu_get_current_container(ctx);

        // Draw log background
        mu_draw_rect(ctx, panel->rect, mu_color(20, 20, 20, 255));

        // Set up text layout
        mu_layout_row(ctx, 1, (int[]){-1}, ctx->text_height(ctx->style->font));

        // Draw log content
        if (logbuf[0]) {
            mu_text(ctx, logbuf);
        }

        // Auto-scroll to bottom when log is updated
        if (logbuf_updated) {
            panel->scroll.y = panel->content_size.y;
            logbuf_updated = 0;
        }

        mu_end_panel(ctx);

        mu_end_window(ctx);
    }
}

static void draw_button(mu_Context *ctx) {
    if (!menu_open && menu_animation == 0.0f) {
        int button_x = window_width / BUTTON_X_RATIO;
        int button_y = window_height / BUTTON_Y_RATIO;

        if (mu_begin_window_ex(ctx, "MenuButton",
                               mu_rect(button_x, button_y, button_width, button_height),
                               MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE |
                               MU_OPT_NOSCROLL | MU_OPT_NOFRAME)) {
            mu_Container *cnt = mu_get_current_container(ctx);
            mu_draw_rect(ctx, cnt->rect, mu_color(40, 40, 40, 255));

            mu_layout_row(ctx, 1, (int[]){-1}, -1);
            if (mu_button_ex(ctx, "Open Menu", 0, MU_OPT_ALIGNCENTER | MU_OPT_NOFRAME)) {
                menu_open = 1;
            }

            mu_end_window(ctx);
        }
    }
}

static void process_frame(mu_Context *ctx) {
    mu_begin(ctx);

    calculate_responsive_dimensions();

    if (menu_open && menu_animation < 1.0f) {
        menu_animation += MENU_ANIMATION_STEP;
        if (menu_animation > 1.0f) menu_animation = 1.0f;
    } else if (!menu_open && menu_animation > 0.0f) {
        menu_animation -= MENU_ANIMATION_STEP;
        if (menu_animation < 0.0f) menu_animation = 0.0f;
    }

    if (menu_animation > 0.0f) {
        menu_window(ctx);
    } else {
        draw_button(ctx);
    }

    mu_end(ctx);
}

#define KEY_MAP_MASK 0xff

static const char button_map[256] = {
    [ SDL_BUTTON_LEFT & KEY_MAP_MASK ] = MU_MOUSE_LEFT,
    [ SDL_BUTTON_RIGHT & KEY_MAP_MASK ] = MU_MOUSE_RIGHT,
    [ SDL_BUTTON_MIDDLE & KEY_MAP_MASK ] = MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
    [ SDLK_LSHIFT & KEY_MAP_MASK ] = MU_KEY_SHIFT,
    [ SDLK_RSHIFT & KEY_MAP_MASK ] = MU_KEY_SHIFT,
    [ SDLK_LCTRL & KEY_MAP_MASK ] = MU_KEY_CTRL,
    [ SDLK_RCTRL & KEY_MAP_MASK ] = MU_KEY_CTRL,
    [ SDLK_LALT & KEY_MAP_MASK ] = MU_KEY_ALT,
    [ SDLK_RALT & KEY_MAP_MASK ] = MU_KEY_ALT,
    [ SDLK_RETURN & KEY_MAP_MASK ] = MU_KEY_RETURN,
    [ SDLK_BACKSPACE & KEY_MAP_MASK ] = MU_KEY_BACKSPACE,
};

static int text_width(mu_Font font, const char *text, int len) {
    if (len == -1) { len = strlen(text); }
    return r_get_text_width(text, len);
}

static int text_height(mu_Font font) {
    return r_get_text_height();
}

int main(int argc, char **argv) {
    SDL_Init(SDL_INIT_EVERYTHING);

    window = SDL_CreateWindow(
        "SiFe",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height,
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
            switch (e.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_WINDOWEVENT:
                    switch (e.window.event) {
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        case SDL_WINDOWEVENT_MAXIMIZED:
                        case SDL_WINDOWEVENT_RESTORED:
                        case SDL_WINDOWEVENT_EXPOSED:
                            SDL_GetWindowSize(window, &window_width, &window_height);

                            r_update_dimensions(window_width, window_height);

                            calculate_responsive_dimensions();

                            r_clear(mu_color(bg[0], bg[1], bg[2], 255));

                            glViewport(0, 0, window_width, window_height);

                            SDL_GL_SwapWindow(window);
                            break;

                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            // Request immediate redraw when window regains focus
                            r_clear(mu_color(bg[0], bg[1], bg[2], 255));
                            process_frame(ctx);
                            r_present();
                            break;
                    }
                    break;

                case SDL_MOUSEMOTION:
                    mu_input_mousemove(ctx, e.motion.x, e.motion.y);
                    break;
                case SDL_MOUSEWHEEL:
                    mu_input_scroll(ctx, 0, e.wheel.y * -30);
                    break;
                case SDL_TEXTINPUT:
                    mu_input_text(ctx, e.text.text);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP: {
                    int b = button_map[e.button.button & KEY_MAP_MASK];
                    if (b && e.type == SDL_MOUSEBUTTONDOWN) { mu_input_mousedown(ctx, e.button.x, e.button.y, b); }
                    if (b && e.type == SDL_MOUSEBUTTONUP) { mu_input_mouseup(ctx, e.button.x, e.button.y, b); }
                    break;
                }

                case SDL_KEYDOWN:
                case SDL_KEYUP: {
                    int c = key_map[e.key.keysym.sym & KEY_MAP_MASK];
                    if (c && e.type == SDL_KEYDOWN) { mu_input_keydown(ctx, c); }
                    if (c && e.type == SDL_KEYUP) { mu_input_keyup(ctx, c); }
                    break;
                }
            }
        }

        process_frame(ctx);

        r_clear(mu_color(bg[0], bg[1], bg[2], 255));
        mu_Command *cmd = NULL;
        while (mu_next_command(ctx, &cmd)) {
            switch (cmd->type) {
                case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color);
                    break;
                case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color);
                    break;
                case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color);
                    break;
                case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect);
                    break;
            }
        }
        r_present();

        SDL_Delay(FRAME_DELAY_MS);
    }

    free(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
