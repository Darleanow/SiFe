#include "menu.h"
#include "constants.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>

static void draw_menu_content(mu_Context *ctx, UIState *state, int menu_width, int header_height) {
    int option_height = state->window_height / OPTION_HEIGHT_RATIO;
    if (option_height < MIN_OPTION_HEIGHT) option_height = MIN_OPTION_HEIGHT;

    // Draw separator
    mu_layout_row(ctx, 1, (int[]){-1}, SEPARATOR_HEIGHT);
    mu_draw_rect(ctx, mu_rect(MENU_PADDING_X, header_height + HEADER_SEPARATOR_Y_OFFSET,
                              menu_width - MENU_CONTENT_WIDTH_OFFSET, SEPARATOR_HEIGHT),
                 ctx->style->colors[MU_COLOR_BORDER]);

    // Menu options
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

    // Color settings
    mu_layout_row(ctx, 1, (int[]){-1}, option_height);
    mu_label(ctx, "Background Color");

    mu_layout_row(ctx, 2, (int[]){SLIDER_LABEL_WIDTH, -1}, option_height);

    mu_label(ctx, "Red:");
    if (mu_slider(ctx, &state->bg_color[0], 0, 255)) { }

    mu_label(ctx, "Green:");
    if (mu_slider(ctx, &state->bg_color[1], 0, 255)) { }

    mu_label(ctx, "Blue:");
    if (mu_slider(ctx, &state->bg_color[2], 0, 255)) { }

    // Color preview
    mu_layout_row(ctx, 1, (int[]){-1}, option_height * 1.5);
    mu_Rect r = mu_layout_next(ctx);
    mu_draw_rect(ctx, r, mu_color(state->bg_color[0], state->bg_color[1], state->bg_color[2], 255));

    char color_text[32];
    snprintf(color_text, sizeof(color_text), "#%02X%02X%02X",
             (int)state->bg_color[0], (int)state->bg_color[1], (int)state->bg_color[2]);
    mu_draw_control_text(ctx, color_text, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);

    // Log section
    int log_height = state->window_height / LOG_HEIGHT_RATIO;
    if (log_height < MIN_LOG_HEIGHT) log_height = MIN_LOG_HEIGHT;

    mu_layout_row(ctx, 1, (int[]){-1}, option_height);
    mu_text(ctx, "Log Output");

    mu_layout_row(ctx, 1, (int[]){-1}, log_height);
    mu_begin_panel(ctx, "Log Panel");
    mu_Container *panel = mu_get_current_container(ctx);

    mu_draw_rect(ctx, panel->rect, mu_color(20, 20, 20, 255));
    mu_layout_row(ctx, 1, (int[]){-1}, ctx->text_height(ctx->style->font));

    const char* logbuf = get_log_buffer();
    if (logbuf[0]) {
        mu_text(ctx, logbuf);
    }

    if (is_log_updated()) {
        panel->scroll.y = panel->content_size.y;
        reset_log_updated();
    }

    mu_end_panel(ctx);
}

static void draw_menu_header(mu_Context *ctx, UIState *state, int menu_width, int header_height, int close_button_size) {
    mu_layout_row(ctx, 2, (int[]){menu_width - close_button_size - HEADER_TEXT_PADDING, close_button_size},
                  header_height);

    // Draw title
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

    // Draw close button
    mu_layout_begin_column(ctx); {
        int button_padding = (header_height - close_button_size) / DIVIDE_BY_TWO;
        mu_layout_set_next(ctx, mu_rect(menu_width - close_button_size - 5, button_padding,
                                        close_button_size, close_button_size), 0);
        if (mu_button_ex(ctx, "X", 0, MU_OPT_ALIGNCENTER)) {
            state->menu_open = 0;
        }
    }
    mu_layout_end_column(ctx);
}

void draw_menu(mu_Context *ctx, UIState *state) {
    int menu_width = state->window_width / MENU_WIDTH_RATIO;
    if (menu_width < MIN_MENU_WIDTH) menu_width = MIN_MENU_WIDTH;

    int x_pos = (int)(-menu_width + menu_width * state->menu_animation);
    x_pos = mu_clamp(x_pos, -menu_width, 0);

    int header_height = state->window_height / HEADER_HEIGHT_RATIO;
    if (header_height < MIN_HEADER_HEIGHT) header_height = MIN_HEADER_HEIGHT;
    int close_button_size = header_height - CLOSE_BUTTON_PADDING;

    if (mu_begin_window_ex(ctx, "Open Menu", mu_rect(x_pos, 0, menu_width, state->window_height),
                           MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL)) {
        mu_Container *win = mu_get_current_container(ctx);
        win->rect.x = x_pos;
        win->rect.w = menu_width;
        win->rect.h = state->window_height;

        // Draw background and header
        mu_draw_rect(ctx, mu_rect(0, 0, menu_width, state->window_height), mu_color(30, 30, 30, 255));
        mu_draw_rect(ctx, mu_rect(0, 0, menu_width, header_height), mu_color(40, 40, 40, 255));

        draw_menu_header(ctx, state, menu_width, header_height, close_button_size);
        draw_menu_content(ctx, state, menu_width, header_height);

        mu_end_window(ctx);
    }
}

void draw_menu_button(mu_Context *ctx, UIState *state) {
    if (!state->menu_open && state->menu_animation == 0.0f) {
        int button_x = state->window_width / BUTTON_X_RATIO;
        int button_y = state->window_height / BUTTON_Y_RATIO;

        if (mu_begin_window_ex(ctx, "MenuButton",
                               mu_rect(button_x, button_y, state->button_width, state->button_height),
                               MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE |
                               MU_OPT_NOSCROLL | MU_OPT_NOFRAME)) {
            mu_Container *cnt = mu_get_current_container(ctx);
            mu_draw_rect(ctx, cnt->rect, mu_color(40, 40, 40, 255));

            mu_layout_row(ctx, 1, (int[]){-1}, -1);
            if (mu_button_ex(ctx, "Open Menu", 0, MU_OPT_ALIGNCENTER | MU_OPT_NOFRAME)) {
                state->menu_open = 1;
            }

            mu_end_window(ctx);
        }
    }
}