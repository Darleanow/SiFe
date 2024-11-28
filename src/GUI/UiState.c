#include "UiState.h"
#include "src/Constants.h"

void ui_state_init(UIState *state) {
    state->window_width = DEFAULT_WINDOW_WIDTH;
    state->window_height = DEFAULT_WINDOW_HEIGHT;
    state->menu_open = 0;
    state->menu_animation = 0.0f;
    state->button_width = MIN_BUTTON_WIDTH;
    state->button_height = MIN_BUTTON_HEIGHT;
    state->bg_color[0] = 19;
    state->bg_color[1] = 19;
    state->bg_color[2] = 19;
}

void ui_state_update_dimensions(UIState *state, int width, int height) {
    state->window_width = width;
    state->window_height = height;
    calculate_responsive_dimensions(state);
}

void calculate_responsive_dimensions(UIState *state) {
    state->button_width = state->window_width / BUTTON_WIDTH_RATIO;
    if (state->button_width < MIN_BUTTON_WIDTH) state->button_width = MIN_BUTTON_WIDTH;
    if (state->button_width > MAX_BUTTON_WIDTH) state->button_width = MAX_BUTTON_WIDTH;

    state->button_height = state->window_height / BUTTON_HEIGHT_RATIO;
    if (state->button_height < MIN_BUTTON_HEIGHT) state->button_height = MIN_BUTTON_HEIGHT;
    if (state->button_height > MAX_BUTTON_HEIGHT) state->button_height = MAX_BUTTON_HEIGHT;
}