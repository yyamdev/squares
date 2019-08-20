#include "shared.h"
#include "js.h"
#include "sin_table.h"

static s32 rng_seed;

static char strbuf[512];
static s32 strbuf_idx = 0;

extern unsigned char __heap_base; // Defined by linker.

void assert_backend(bool condition, s32 line_number, const char *file_name)
{
    if (!condition)
    {
        js_print("WASM Assert triggered!");
        js_print("File name:");
        js_print(file_name);
        js_print("Line number:");
        js_print_number(line_number);
        while (true) continue; // TODO: Print message
    }
}

s32 math_min_s32(s32 a, s32 b)
{
    return a <= b ? a : b;
}

s32 math_max_s32(s32 a, s32 b)
{
    return a >= b ? a : b;
}

s32 math_abs_s32(s32 x)
{
    if (x < 0) return -x;
    else return x;
}

f32 math_abs_f32(f32 x)
{
    if (x < 0.f) return -x;
    else return x;
}

f32 math_mod_f32(f32 x, f32 y)
{
    while (x > y) x -= y;
    return x;
}

s32 math_round_f32_to_s32(f32 x)
{
    return (s32)(x + .5f);
}

f32 math_sin(f32 x)
{
    const f32 pi2 = 3.141592f * 2.f;
    while (x < 0.f) x += pi2;
    x = math_mod_f32(x, pi2);
    s32 idx = (x * (f32)countof(math_sin_table)) / pi2;
    if (idx < 0) idx = 0;
    if (idx >= countof(math_sin_table)) idx = countof(math_sin_table) - 1;
    return math_sin_table[idx];
}

void rng_set_seed(u32 seed)
{
    rng_seed = seed;
}

u32 rng_get_u32()
{
    // xor algorithm from p. 4 of Marsaglia, "Xorshift RNGs".
    u32 x = rng_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
    rng_seed = x;
	return x;
}

u32 rng_get_u32_range(u32 min_inclusive, u32 max_inclusive)
{
    u32 range = max_inclusive - min_inclusive;
    return min_inclusive + (rng_get_u32() % (range + 1));
}

void mem_copy(void *destination, void *source, s32 bytes)
{
    ASSERT(destination != NULL);
    
    u8 *dest = (u8*)destination;
    u8 *src = (u8*)source;
    for (s32 i = 0; i < bytes; ++i)
    {
        dest[i] = src[i];
    }
}

void mem_set_u8(void *destination, s32 count, u8 value)
{
    ASSERT(destination != NULL);
    
    u8 *dest = (u8*)destination;
    for (s32 i = 0; i < count; ++i)
    {
        dest[i] = value;
    }
}

void mem_set_u32(void *destination, s32 count, u32 value)
{
    ASSERT(destination != NULL);
    
    u32 *dest = (u32*)destination;
    for (s32 i = 0; i < count; ++i)
    {
        dest[i] = value;
    }
}

void mem_set_s32(void *destination, s32 count, s32 value)
{
    ASSERT(destination != NULL);
    
    mem_set_u32(destination, count, (u32)value);
}

void *mem_alloc(s32 bytes)
{
    // TODO: Add max size and return error if heap_top would exceed it.
    // NOTE: Memory allocated by this function cannot be freed.
    static uintptr_t heap_top = (uintptr_t)&__heap_base;
    uintptr_t heap_top_before = heap_top;
    heap_top += bytes;
    ASSERT(heap_top > heap_top_before);
    return (void *)heap_top_before;
}

s32 str_count_length(const char *str)
{
    ASSERT(str != NULL);
    
    s32 len = 0;
    while (*str != '\0')
    {
        ++str;
        ++len;
    }
    return len;
}

bool str_s32_to_str(const char *str, s32 max_length, s32 number)
{
    ASSERT(str != NULL);
    ASSERT(max_length > 0);
    // TODO: Handle INT_MIN case
    
    bool is_negative = false;
    
    if (number < 0)
    {
        is_negative = true;
        number = -number; // NOTE: Won't work with INT_MIN?
    }
    
    char buffer[128];
    mem_set_u8(buffer, 128, 0);
    s32 i = 126; // TODO: Replace with ARRAY_LEN(buffer)
    s32 count = 1; // NULL-terminator
    
    if (number == 0)
    {
        buffer[i] = '0';
        i -= 1;
        count += 1;
    }
    else
    {
        while (number != 0)
        {
            buffer[i] = '0' + (number % 10);
            number /= 10;
            i -= 1;
            count += 1;
        }
        
        if (is_negative)
        {
            buffer[i] = '-';
            i -= 1;
            count += 1;
        }
    }
    
    if (count > max_length) return false; // Not enough space
    
    mem_copy((void *)str, &buffer[i + 1], count);
    
    return true;
}

void strbuf_clear(void)
{
    strbuf_idx = 0;
    
    // TODO: Shouldn't need to mem set to 0 here. Find out what is going wrong...
    mem_set_u8((void *)strbuf, countof(strbuf), 0);
}

void strbuf_push_string(const char *str)
{
    ASSERT(str != NULL);
    
    s32 str_len = str_count_length(str);
    
    ASSERT(strbuf_idx + str_len + 1 < countof(strbuf)); // TODO: Make this a regular error check?
    
    mem_copy((void*)(&strbuf[strbuf_idx]), (void*)str, str_len + 1);
    strbuf_idx += str_len; // Set strbuf_idx to be at the NULL-terminator.
}

void strbuf_push_s32(s32 num)
{
    s32 len_before = str_count_length(strbuf);
    str_s32_to_str((const char *)(&strbuf[strbuf_idx]), countof(strbuf) - strbuf_idx - 1, num);
    s32 len_after = str_count_length(strbuf);
    strbuf_idx += len_after - len_before;
}

void strbuf_push_char(u8 c)
{
    strbuf[strbuf_idx++] = c;
    strbuf[strbuf_idx++] = '\0';
}

char *strbuf_get(void)
{
    return strbuf;
}

s32 image_calculate_size(struct Image *image)
{
    ASSERT(image != NULL);
    
    return image->width * image->height * 4; // HTML5 images are always in RGBA (32 bit) format.
}

static u8 video_blend_component(u8 a, u8 b, float percent_b)
{
    // TODO: Change this function to blend 2 colors instead of individual components
    
    ASSERT(percent_b <= 1.f);
    
    float percent_a = 1.f - percent_b;
    return (u8)(((float)a * percent_a) + ((float)b * percent_b));
}

static u32 video_make_color_u32(struct Color color)
{
    return (color.a << 24) | (color.b << 16) | (color.g << 8) | color.r; // WASM is little-endian.
}

struct Color video_make_color(u8 r, u8 g, u8 b)
{
    struct Color c = {r, g, b};
    return c;
}

void video_clear_framebuffer(struct Image *framebuffer, struct Color color)
{
    ASSERT(framebuffer != NULL);
    ASSERT(framebuffer->data != NULL);
    
    color.a = 255;
    
    u32 color_u32 = video_make_color_u32(color);
    mem_set_u32(framebuffer->data, framebuffer->width * framebuffer->height, color_u32);
}

void video_draw_rect(struct Image *framebuffer, s32 rect_x, s32 rect_y, s32 rect_w, s32 rect_h, struct Color color)
{
    ASSERT(framebuffer != NULL);
    ASSERT(framebuffer->data != NULL);
    
    // TODO: Move clipping logic into one function
    s32 x_start = math_max_s32(rect_x, 0);
    s32 y_start = math_max_s32(rect_y, 0);
    
    s32 x_end = math_min_s32(rect_x + rect_w, framebuffer->width);
    s32 y_end = math_min_s32(rect_y + rect_h, framebuffer->height);
    s32 width = x_end - x_start;
    s32 height = y_end - y_start;
    
    for (int y = y_start; y < y_end; ++y)
    {
        for (int x = x_start; x < x_end; ++x)
        {
            int idx = (y * framebuffer->width + x) * 4;
            
            // Alpha blend
            u8 *framebuffer_components = (u8 *)framebuffer->data;
            float percent = (float)color.a / 255.f;
            framebuffer_components[idx + 0] = video_blend_component(framebuffer_components[idx + 0], color.r, percent);
            framebuffer_components[idx + 1] = video_blend_component(framebuffer_components[idx + 1], color.g, percent);
            framebuffer_components[idx + 2] = video_blend_component(framebuffer_components[idx + 2], color.b, percent);
            framebuffer_components[idx + 3] = 255;
        }
    }
}

void video_blit(struct Image *framebuffer, struct Image *image, s32 dest_x, s32 dest_y, s32 sub_rect_x, s32 sub_rect_y, s32 sub_rect_w, s32 sub_rect_h, enum BlitFlip flip)
{
    ASSERT(framebuffer != NULL);
    ASSERT(framebuffer->data != NULL);
    ASSERT(image != NULL);
    ASSERT(image->data != NULL);
    
    u8 *img = (u8 *)image->data;
    u8 *dest = (u8 *)framebuffer->data;
    
    s32 x_start = dest_x;
    s32 y_start = dest_y;
    s32 x_end = dest_x + sub_rect_w;
    s32 y_end = dest_y + sub_rect_h;
    s32 image_width = x_end - x_start;
    s32 image_height = y_end - y_start;
    
    for (s32 y = 0; y < image_height; ++y)
    {
        for (s32 x = 0; x < image_width; ++x)
        {
            s32 img_x = x;
            s32 img_y = y;
            if (flip & BLIT_FLIP_HOR) img_x = image_width - 1 - x;
            if (flip & BLIT_FLIP_VER) img_y = image_height - 1 - y;
            s32 image_idx = ((img_y + sub_rect_y) * image->width + (img_x + sub_rect_x)) * 4;
            
            s32 xfb = x + x_start;
            s32 yfb = y + y_start;
            
            if (xfb < 0 || xfb >= framebuffer->width ||
                yfb < 0 || yfb >= framebuffer->height)
            {
                continue;
            }
            
            s32 fb_pixel_idx = yfb * framebuffer->width + xfb;
            s32 fb_idx = fb_pixel_idx * 4;
            
            // Alpha blend
            u8 src_a = img[image_idx + 3];
            float percent_src = (float)src_a / 255.f;
            dest[fb_idx + 0] = video_blend_component(dest[fb_idx + 0], img[image_idx + 0], percent_src);
            dest[fb_idx + 1] = video_blend_component(dest[fb_idx + 1], img[image_idx + 1], percent_src);
            dest[fb_idx + 2] = video_blend_component(dest[fb_idx + 2], img[image_idx + 2], percent_src);
            dest[fb_idx + 3] = 255;
        }
    }
}

void video_draw_text(struct Image *framebuffer, struct ImageAsciiMonospacedFont *font, const char *str, s32 dest_x, s32 dest_y, enum TextAlign align)
{
    ASSERT(framebuffer != NULL);
    ASSERT(framebuffer->data != NULL);
    ASSERT(font != NULL);
    ASSERT(font->image != NULL);
    ASSERT(font->image->data != NULL);
    ASSERT(str != NULL);
    
    s32 len = str_count_length(str);
    s32 char_width = font->char_width;
    s32 char_height = font->char_height;
    
    if (align == TEXT_ALIGN_CENTER)
    {
        // TODO: Make center alignment work with text containing newlines
        dest_x = dest_x - ((len * char_width) / 2);
    }
    
    s32 x = dest_x;
    s32 y = dest_y;
    struct Image *image = font->image;
    
    for (s32 i = 0; i < len; ++i)
    {
        if (str[i] == '\n')
        {
            x = dest_x;
            y += char_height;
            continue;
        }
        
        s32 char_idx = str[i];
        s32 x_offset = (char_idx - 32) * char_width;
        
        video_blit(
            framebuffer,
            image,
            x,
            y,
            x_offset,
            0,
            char_width,
            char_height,
            BLIT_FLIP_NONE);
        
        x += char_width;
    }
}