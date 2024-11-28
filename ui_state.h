#ifndef UI_STATE_H
#define UI_STATE_H

typedef struct {
    int window_width;
    int window_height;
    int menu_open;
    float menu_animation;
    int button_width;
    int button_height;
    float bg_color[3];
} UIState;

void ui_state_init(UIState *state);
void ui_state_update_dimensions(UIState *state, int width, int height);
void calculate_responsive_dimensions(UIState *state);

#endif
