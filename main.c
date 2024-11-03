#include <SDL2/SDL.h>
#undef main
#include <SDL_opengl.h>
#include <stdio.h>
#include "renderer.h"
#include "microui.h"
#include <string.h>

static char logbuf[64000];
static int logbuf_updated = 0;
static float bg[3] = { 19, 19, 19 };

// Window dimensions
static int window_width = 800;
static int window_height = 600;

// Menu state
static int menu_open = 0;
static float menu_animation = 0.0f;
static int button_width = 130;
static int button_height = 25;

// Store window globally so renderer can access it
SDL_Window* window = nullptr;

// Modified r_init declaration to accept window
void r_init(void);

static void write_log(const char *text) {
  if (logbuf[0]) { strcat(logbuf, "\n"); }
  strcat(logbuf, text);
  logbuf_updated = 1;
}

// Calculate responsive dimensions
static void calculate_responsive_dimensions(void) {
  // Menu width is 1/4 of window width, with minimum of 200px
  int calculated_menu_width = window_width / 4;
  if (calculated_menu_width < 200) calculated_menu_width = 200;

  // Button dimensions scale with window size but have minimums
  button_width = window_width / 6;
  if (button_width < 130) button_width = 130;
  if (button_width > 200) button_width = 200;

  button_height = window_height / 24;
  if (button_height < 25) button_height = 25;
  if (button_height > 40) button_height = 40;
}

static void menu_window(mu_Context *ctx) {
  int menu_width = window_width / 4;
  if (menu_width < 200) menu_width = 200;

  int x_pos = (int)(-menu_width + (menu_width * menu_animation));
  x_pos = mu_clamp(x_pos, -menu_width, 0);

  int header_height = window_height / 15;
  if (header_height < 40) header_height = 40;
  int close_button_size = header_height - 10;

  if (mu_begin_window_ex(ctx, "Open Menu", mu_rect(x_pos, 0, menu_width, window_height),
                        MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL)) {
    mu_Container *win = mu_get_current_container(ctx);
    win->rect.x = x_pos;
    win->rect.w = menu_width;
    win->rect.h = window_height;

    mu_draw_rect(ctx, mu_rect(0, 0, menu_width, window_height), mu_color(30, 30, 30, 255));
    mu_draw_rect(ctx, mu_rect(0, 0, menu_width, header_height), mu_color(40, 40, 40, 255));

    mu_layout_row(ctx, 2, (int[]) { menu_width - close_button_size - 10, close_button_size }, header_height);

    mu_layout_begin_column(ctx);
    {
        int title_width = ctx->text_width(ctx->style->font, "SiFe", 4);
        int title_height = ctx->text_height(ctx->style->font);
        int text_x = ((menu_width - close_button_size - 10) - title_width) / 2;
        int text_y = (header_height - title_height) / 2;

        mu_draw_text(ctx, ctx->style->font, "SiFe", 4,
                    mu_vec2(text_x, text_y),
                    mu_color(230, 230, 230, 255));
    }
    mu_layout_end_column(ctx);

    mu_layout_begin_column(ctx);
    {
        int button_padding = (header_height - close_button_size) / 2;
        mu_layout_set_next(ctx, mu_rect(menu_width - close_button_size - 5, button_padding,
                                      close_button_size, close_button_size), 0);
        if (mu_button_ex(ctx, "X", 0, MU_OPT_ALIGNCENTER)) {
            menu_open = 0;
        }
    }
    mu_layout_end_column(ctx);

    // Responsive spacing and layout
    int option_height = window_height / 20;
    if (option_height < 30) option_height = 30;

    mu_layout_row(ctx, 1, (int[]) { -1 }, 1);
    mu_draw_rect(ctx, mu_rect(10, header_height + 5, menu_width - 20, 1),
                 ctx->style->colors[MU_COLOR_BORDER]);

    mu_layout_row(ctx, 1, (int[]) { -1 }, option_height);
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

    mu_layout_row(ctx, 1, (int[]) { -1 }, option_height / 2);  // Spacing row

    mu_layout_row(ctx, 1, (int[]) { -1 }, 1);
    mu_Rect sep = mu_layout_next(ctx);
    mu_draw_rect(ctx, mu_rect(10, sep.y, menu_width - 20, 1),
                 ctx->style->colors[MU_COLOR_BORDER]);

    mu_layout_row(ctx, 1, (int[]) { -1 }, option_height);
    mu_label(ctx, "Background Color");

    static float bg_color[3] = { 19, 19, 19 };
    mu_layout_row(ctx, 2, (int[]) { 50, -1 }, option_height);

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

    mu_layout_row(ctx, 1, (int[]) { -1 }, option_height * 1.5);
    mu_Rect r = mu_layout_next(ctx);
    mu_draw_rect(ctx, r, mu_color(bg_color[0], bg_color[1], bg_color[2], 255));

    char color_text[32];
    sprintf(color_text, "#%02X%02X%02X", (int)bg_color[0], (int)bg_color[1], (int)bg_color[2]);
    mu_draw_control_text(ctx, color_text, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);

    mu_end_window(ctx);
  }
}

static void draw_button(mu_Context *ctx) {
  if (!menu_open && menu_animation == 0.0f) {
    // Scale button position with window size
    int button_x = window_width / 80;  // 1.25% of window width
    int button_y = window_height / 60; // 1.67% of window height

    if (mu_begin_window_ex(ctx, "MenuButton",
                          mu_rect(button_x, button_y, button_width, button_height),
                          MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE |
                          MU_OPT_NOSCROLL | MU_OPT_NOFRAME)) {

      mu_Container *cnt = mu_get_current_container(ctx);
      mu_draw_rect(ctx, cnt->rect, mu_color(40, 40, 40, 255));

      mu_layout_row(ctx, 1, (int[]) { -1 }, -1);
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
    menu_animation += 0.15f;
    if (menu_animation > 1.0f) menu_animation = 1.0f;
  } else if (!menu_open && menu_animation > 0.0f) {
    menu_animation -= 0.15f;
    if (menu_animation < 0.0f) menu_animation = 0.0f;
  }

  if (menu_animation > 0.0f) {
    menu_window(ctx);
  } else {
    draw_button(ctx);
  }

  mu_end(ctx);
}

static const char button_map[256] = {
  [ SDL_BUTTON_LEFT   & 0xff ] =  MU_MOUSE_LEFT,
  [ SDL_BUTTON_RIGHT  & 0xff ] =  MU_MOUSE_RIGHT,
  [ SDL_BUTTON_MIDDLE & 0xff ] =  MU_MOUSE_MIDDLE,
};

static const char key_map[256] = {
  [ SDLK_LSHIFT      & 0xff ] = MU_KEY_SHIFT,
  [ SDLK_RSHIFT      & 0xff ] = MU_KEY_SHIFT,
  [ SDLK_LCTRL       & 0xff ] = MU_KEY_CTRL,
  [ SDLK_RCTRL       & 0xff ] = MU_KEY_CTRL,
  [ SDLK_LALT        & 0xff ] = MU_KEY_ALT,
  [ SDLK_RALT        & 0xff ] = MU_KEY_ALT,
  [ SDLK_RETURN      & 0xff ] = MU_KEY_RETURN,
  [ SDLK_BACKSPACE   & 0xff ] = MU_KEY_BACKSPACE,
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
          int b = button_map[e.button.button & 0xff];
          if (b && e.type == SDL_MOUSEBUTTONDOWN) { mu_input_mousedown(ctx, e.button.x, e.button.y, b); }
          if (b && e.type == SDL_MOUSEBUTTONUP) { mu_input_mouseup(ctx, e.button.x, e.button.y, b); }
          break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          int c = key_map[e.key.keysym.sym & 0xff];
          if (c && e.type == SDL_KEYDOWN) { mu_input_keydown(ctx, c); }
          if (c && e.type == SDL_KEYUP) { mu_input_keyup(ctx, c); }
          break;
        }
      }
    }

    process_frame(ctx);

    r_clear(mu_color(bg[0], bg[1], bg[2], 255));
    mu_Command *cmd = nullptr;
    while (mu_next_command(ctx, &cmd)) {
      switch (cmd->type) {
        case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
        case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
        case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
      }
    }
    r_present();

    SDL_Delay(16);
  }


  free(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}