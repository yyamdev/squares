#ifndef SHARED__H
#define SHARED__H

#include "types.h"
#include <stdbool.h>

#define countof(x)(sizeof(x) / sizeof(x[0]))

enum TextAlign
{
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_COUNT,
};

struct Color
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct Image
{
    void *data;
    s32 width;
    s32 height;
};

struct ImageAsciiMonospacedFont
{
    struct Image *image;
    s32 char_width;
    s32 char_height;
};

enum BlitFlip
{
    BLIT_FLIP_NONE = 0,
    BLIT_FLIP_HOR = 1,
    BLIT_FLIP_VER = 2,
};

// Asserts
void assert_backend(bool condition, s32 line_number, const char *file_name);
#define ASSERT(cond) (assert_backend(cond, __LINE__, __FILE__))

// Math
s32 math_min_s32(s32 a, s32 b);
s32 math_max_s32(s32 a, s32 b);
s32 math_abs_s32(s32 x);
f32 math_abs_f32(f32 x);
f32 math_mod_f32(f32 x, f32 y);
s32 math_round_f32_to_s32(f32 x);
f32 math_sin(f32 x); // In radians

// Random number generation
void rng_set_seed(u32 seed);
u32 rng_get_u32();
u32 rng_get_u32_range(u32 min_inclusive, u32 max_inclusive);

// Memory
void mem_copy(void *destination, void *source, s32 bytes);
void mem_set_u8(void *destination, s32 count, u8 value);
void mem_set_u32(void *destination, s32 count, u32 value);
void mem_set_s32(void *destination, s32 count, s32 value);
void *mem_alloc(s32 bytes);

// Strings
s32 str_count_length(const char *str); // Not including NULL-terminator
bool str_s32_to_str(const char *str, s32 max_length, s32 number);

void strbuf_clear(void);
void strbuf_push_string(const char *str);
void strbuf_push_s32(s32 num);
void strbuf_push_char(u8 c);
char *strbuf_get(void);

// Image
s32 image_calculate_size(struct Image *image);

// Rendering
struct Color video_make_color(u8 r, u8 g, u8 b);
void video_clear_framebuffer(struct Image *framebuffer, struct Color color);
void video_draw_rect(struct Image *framebuffer, s32 rect_x, s32 rect_y, s32 rect_w, s32 rect_h, struct Color color);
void video_blit(struct Image *framebuffer, struct Image *image, s32 dest_x, s32 dest_y, s32 sub_rect_x, s32 sub_rect_y, s32 sub_rect_w, s32 sub_rect_h, enum BlitFlip flip);
void video_draw_text(struct Image *framebuffer, struct ImageAsciiMonospacedFont *font, const char *str, s32 dest_x, s32 dest_y, enum TextAlign align);

#endif