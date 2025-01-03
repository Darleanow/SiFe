#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <assert.h>
#include <stdio.h>
#include "src/Config/Renderer.h"
#include "src/Config/atlas.inl"

#define BUFFER_SIZE 16384

static GLfloat tex_buf[BUFFER_SIZE * 8];
static GLfloat vert_buf[BUFFER_SIZE * 8];
static GLubyte color_buf[BUFFER_SIZE * 16];
static GLuint index_buf[BUFFER_SIZE * 6];

static int width = 800;
static int height = 600;
static int buf_idx;

extern SDL_Window *window;

int r_init(void) {
    // Create OpenGL context first
    const SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "Failed to create OpenGL context: %s\n", SDL_GetError());
        return -1;
    }

    // Set OpenGL attributes before creating context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    /* init gl */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    /* init texture */
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
                 GL_ALPHA, GL_UNSIGNED_BYTE, atlas_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Check for errors after initialization
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL error during initialization: %d\n", error);
    }
    return 0;
}

void r_update_dimensions(const int w, const int h) {
#ifdef __APPLE__
        // Get the actual pixel dimensions for Retina displays
        int drawable_width, drawable_height;
        SDL_GL_GetDrawableSize(window, &drawable_width, &drawable_height);
        width = drawable_width;
        height = drawable_height;
        glViewport(0, 0, drawable_width, drawable_height);
#else
    width = w;
    height = h;
    glViewport(0, 0, w, h);
#endif
}

static void flush(void) {
    if (buf_idx == 0) { return; }

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTexCoordPointer(2, GL_FLOAT, 0, tex_buf);
    glVertexPointer(2, GL_FLOAT, 0, vert_buf);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, color_buf);
    glDrawElements(GL_TRIANGLES, buf_idx * 6, GL_UNSIGNED_INT, index_buf);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    buf_idx = 0;
}

static void push_quad(const mu_Rect dst, const mu_Rect src, const mu_Color color) {
    if (buf_idx >= BUFFER_SIZE) {
        flush();
        if (buf_idx >= BUFFER_SIZE) {
            fprintf(stderr, "Buffer overflow in push_quad\n");
            return;
        }
    }

    const int texvert_idx = buf_idx * 8;
    const int color_idx = buf_idx * 16;
    const int element_idx = buf_idx * 4;
    const int index_idx = buf_idx * 6;
    buf_idx++;

    /* update texture buffer */
    const float x = src.x / (float) ATLAS_WIDTH;
    const float y = src.y / (float) ATLAS_HEIGHT;
    const float w = src.w / (float) ATLAS_WIDTH;
    const float h = src.h / (float) ATLAS_HEIGHT;
    tex_buf[texvert_idx + 0] = x;
    tex_buf[texvert_idx + 1] = y;
    tex_buf[texvert_idx + 2] = x + w;
    tex_buf[texvert_idx + 3] = y;
    tex_buf[texvert_idx + 4] = x;
    tex_buf[texvert_idx + 5] = y + h;
    tex_buf[texvert_idx + 6] = x + w;
    tex_buf[texvert_idx + 7] = y + h;

    /* update vertex buffer */
    vert_buf[texvert_idx + 0] = dst.x;
    vert_buf[texvert_idx + 1] = dst.y;
    vert_buf[texvert_idx + 2] = dst.x + dst.w;
    vert_buf[texvert_idx + 3] = dst.y;
    vert_buf[texvert_idx + 4] = dst.x;
    vert_buf[texvert_idx + 5] = dst.y + dst.h;
    vert_buf[texvert_idx + 6] = dst.x + dst.w;
    vert_buf[texvert_idx + 7] = dst.y + dst.h;

    /* update color buffer */
    memcpy(color_buf + color_idx + 0, &color, 4);
    memcpy(color_buf + color_idx + 4, &color, 4);
    memcpy(color_buf + color_idx + 8, &color, 4);
    memcpy(color_buf + color_idx + 12, &color, 4);

    /* update index buffer */
    index_buf[index_idx + 0] = element_idx + 0;
    index_buf[index_idx + 1] = element_idx + 1;
    index_buf[index_idx + 2] = element_idx + 2;
    index_buf[index_idx + 3] = element_idx + 2;
    index_buf[index_idx + 4] = element_idx + 3;
    index_buf[index_idx + 5] = element_idx + 1;
}

void r_draw_rect(const mu_Rect rect, const mu_Color color) {
    push_quad(rect, atlas[ATLAS_WHITE], color);
}

void r_draw_text(const char *text, const mu_Vec2 pos, const mu_Color color) {
    mu_Rect dst = {pos.x, pos.y, 0, 0};
    for (const char *p = text; *p; p++) {
        if ((*p & 0xc0) == 0x80) { continue; }
        const int chr = mu_min((unsigned char) *p, 127);
        const mu_Rect src = atlas[ATLAS_FONT + chr];
        dst.w = src.w;
        dst.h = src.h;
        push_quad(dst, src, color);
        dst.x += dst.w;
    }
}

void r_draw_icon(const int id, const mu_Rect rect, const mu_Color color) {
    const mu_Rect src = atlas[id];
    const int x = rect.x + (rect.w - src.w) / 2;
    const int y = rect.y + (rect.h - src.h) / 2;
    push_quad(mu_rect(x, y, src.w, src.h), src, color);
}

int r_get_text_width(const char *text, int len) {
    int res = 0;
    for (const char *p = text; *p && len--; p++) {
        if ((*p & 0xc0) == 0x80) { continue; }
        const int chr = mu_min((unsigned char) *p, 127);
        res += atlas[ATLAS_FONT + chr].w;
    }
    return res;
}

int r_get_text_height(void) {
    return 18;
}

void r_set_clip_rect(const mu_Rect rect) {
    flush();
    glScissor(rect.x, height - (rect.y + rect.h), rect.w, rect.h);
}

void r_clear(const mu_Color color) {
    flush();
    glClearColor(color.r / 255., color.g / 255., color.b / 255., color.a / 255.);
    glClear(GL_COLOR_BUFFER_BIT);
}

void r_present(void) {
    flush();
    SDL_GL_SwapWindow(window);
}
