#include <SDL2/SDL.h>
#undef main
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
void r_init(void); // This should be modified in renderer.c to use the global window

static void write_log(const char *text) {
  if (logbuf[0]) { strcat(logbuf, "\n"); }
  strcat(logbuf, text);
  logbuf_updated = 1;
}

static void menu_window(mu_Context *ctx) {
  int menu_width = window_width / 4;
  // Calculate x position for animation but clamp it at 0
  int x_pos = (int)(-menu_width + (menu_width * menu_animation));
  x_pos = mu_clamp(x_pos, 0, 0);  // Clamp between 0 and 0 to prevent sliding

  int header_height = 40;
  int close_button_size = 30;

  if (mu_begin_window_ex(ctx, "Open Menu", mu_rect(x_pos, 0, menu_width, window_height),
                        MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL)) {
    mu_Container *win = mu_get_current_container(ctx);
    win->rect.x = x_pos;  // Ensure x position is maintained
    win->rect.w = menu_width;

    // Draw custom background
    mu_draw_rect(ctx, mu_rect(0, 0, menu_width, window_height), mu_color(30, 30, 30, 255));

    // Draw header background
    mu_draw_rect(ctx, mu_rect(0, 0, menu_width, header_height), mu_color(40, 40, 40, 255));

    // Layout for header row - split into title area and close button
    mu_layout_row(ctx, 2, (int[]) { menu_width - close_button_size - 10, close_button_size }, header_height);

    // Title area
    mu_layout_begin_column(ctx);
    {
        // Calculate text dimensions for centering
        int title_width = ctx->text_width(ctx->style->font, "SiFe", 4);
        int title_height = ctx->text_height(ctx->style->font);

        // Calculate centered position within the title area
        int text_x = ((menu_width - close_button_size - 10) - title_width) / 2;
        int text_y = (header_height - title_height) / 2;

        // Draw the title text
        mu_draw_text(ctx, ctx->style->font, "SiFe", 4,
                    mu_vec2(text_x, text_y),
                    mu_color(230, 230, 230, 255));
    }
    mu_layout_end_column(ctx);

    // Close button
    mu_layout_begin_column(ctx);
    {
        // Calculate centered position for the close button
        int button_padding = (header_height - close_button_size) / 2;
        mu_layout_set_next(ctx, mu_rect(menu_width - close_button_size - 5, button_padding,
                                      close_button_size, close_button_size), 0);

        if (mu_button_ex(ctx, "X", 0, MU_OPT_ALIGNCENTER)) {
            menu_open = 0;
        }
    }
    mu_layout_end_column(ctx);

    // Separator line below header
    mu_layout_row(ctx, 1, (int[]) { -1 }, 1);
    mu_draw_rect(ctx, mu_rect(10, header_height + 5, menu_width - 20, 1),
                 ctx->style->colors[MU_COLOR_BORDER]);

    // Menu content
    mu_layout_row(ctx, 1, (int[]) { -1 }, 30);
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

    // Add spacing before the slider section
    mu_layout_row(ctx, 1, (int[]) { -1 }, 20);  // Spacing row

    // Add a separator line using layout
    mu_layout_row(ctx, 1, (int[]) { -1 }, 1);
    mu_Rect sep = mu_layout_next(ctx);
    mu_draw_rect(ctx, mu_rect(10, sep.y, menu_width - 20, 1),
                 ctx->style->colors[MU_COLOR_BORDER]);

    // Add title for the slider section
    mu_layout_row(ctx, 1, (int[]) { -1 }, 30);
    mu_label(ctx, "Background Color");

    // Add slider controls with labels
    static float bg_color[3] = { 19, 19, 19 };  // Starting values matching your current bg
    mu_layout_row(ctx, 2, (int[]) { 50, -1 }, 25);

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

    // Add color preview box
    mu_layout_row(ctx, 1, (int[]) { -1 }, 40);
    mu_Rect r = mu_layout_next(ctx);
    mu_draw_rect(ctx, r, mu_color(bg_color[0], bg_color[1], bg_color[2], 255));

    // Add RGB value text
    char color_text[32];
    sprintf(color_text, "#%02X%02X%02X", (int)bg_color[0], (int)bg_color[1], (int)bg_color[2]);
    mu_draw_control_text(ctx, color_text, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);

    mu_end_window(ctx);
  }
}

static void draw_button(mu_Context *ctx) {
  // Only draw button if menu is not open
  if (!menu_open && menu_animation == 0.0f) {
    if (mu_begin_window_ex(ctx, "MenuButton", mu_rect(10, 10, button_width, button_height),
                          MU_OPT_NOCLOSE | MU_OPT_NOTITLE | MU_OPT_NORESIZE | MU_OPT_NOSCROLL | MU_OPT_NOFRAME)) {

      // Set dark background for the entire container
      mu_Container *cnt = mu_get_current_container(ctx);
      mu_draw_rect(ctx, cnt->rect, mu_color(40, 40, 40, 255));

      mu_layout_row(ctx, 1, (int[]) { -1 }, -1);  // Use -1 for height to fill container
      if (mu_button_ex(ctx, "Open Menu", 0, MU_OPT_ALIGNCENTER | MU_OPT_NOFRAME)) {
        menu_open = 1;
      }

      mu_end_window(ctx);
                          }
  }
}

static void process_frame(mu_Context *ctx) {
  mu_begin(ctx);

  // Update menu animation
  if (menu_open && menu_animation < 1.0f) {
    menu_animation += 0.15f;
    if (menu_animation > 1.0f) menu_animation = 1.0f;
  } else if (!menu_open && menu_animation > 0.0f) {
    menu_animation -= 0.15f;
    if (menu_animation < 0.0f) menu_animation = 0.0f;
  }

  // Draw menu if it's animating or fully open
  if (menu_animation > 0.0f) {
    menu_window(ctx);
  } else {
    // Only draw button if menu is fully closed
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
  // Init SDL
  SDL_Init(SDL_INIT_EVERYTHING);

  // Create single window
  window = SDL_CreateWindow(
    "SiFe",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    window_width, window_height,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
  );

  // Initialize renderer with existing window
  r_init();

  // Init microui
  mu_Context *ctx = malloc(sizeof(mu_Context));
  mu_init(ctx);
  ctx->text_width = text_width;
  ctx->text_height = text_height;

  int running = 1;
  while (running) {
    // Handle SDL events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = 0;
          break;

        case SDL_WINDOWEVENT:
          if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
            window_width = e.window.data1;
            window_height = e.window.data2;
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
    mu_Command *cmd = NULL;
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

  // Cleanup
  free(ctx);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}