#ifndef JS__H
#define JS__H

#include "types.h"

// Key codes
#define JS_KEY_CODE_SPACE 32

// Exported functions
void js_on_startup(void);
void js_on_frame(void);

void js_on_keyboard_event(s32 ascii_code, s32 new_state);
void *js_on_image_loaded(s32 id, s32 width, s32 height);

// Imported functions
extern void js_print(const char* msg);
extern void js_print_number(s32 number);
extern void js_show_alert(const char* msg);

extern s32 js_get_time_ms(void);
extern u32 js_get_unix_time(void); // TODO: Use 64 bit inetegr when WASM standard is updated

extern void js_canvas_resize(s32 w, s32 h, f32 scale);
extern void js_set_framebuffer(void *address);

extern void js_asset_load_image(const char *url, s32 id);
extern s32 js_asset_load_audio(const char *url);
extern s32 js_asset_count_loaded(void);

extern void js_audio_play(s32 id);
extern void js_audio_pause(s32 id);
extern void js_audio_stop(s32 id);
extern s32 js_audio_get_time(s32 id);

extern void js_localstore_set_s32(const char *key, s32 value);
extern s32 js_localstore_get_s32(const char *key);

#endif
