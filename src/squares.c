#include "types.h"
#include "js.h"
#include "shared.h"
#include <stdbool.h>

#define CANVAS_WIDTH 64
#define CANVAS_HEIGHT 64
#define CANVAS_SCALE 6

#define TEXT_Y_MIDDLE ((CANVAS_HEIGHT / 2) - 4)

#define LEVEL_COUNT 4
#define LEVEL_MAX_WIDTH 1024
#define LEVEL_MAX_HEIGHT 64
#define LEVEL_TILE_COUNT (LEVEL_MAX_WIDTH * LEVEL_MAX_HEIGHT)
#define LEVEL_MAX_FINISH_TILES 32
#define LEVEL_MAX_SPIKE_TILES 512
#define LEVEL_MAX_MOVING_BLOCK_TILES 512

#define BPM_TO_BEAT_LEN_MS(x) (60000.f / x)

#define SPACE_TO_MOVE false

#define PRINT_SIZE_OF_LEVEL_STRUCT false

#define PLAY_COWBELL false

#define GOD_MODE false

enum ImageId
{
    IMAGE_ID_FONT_SMALL,
    IMAGE_ID_TEST_LEVEL,
    IMAGE_ID_MENU_BG,
    IMAGE_ID_LEVEL_1,
    IMAGE_ID_LEVEL_2,
    IMAGE_ID_LEVEL_3,
    IMAGE_ID_LEVEL_4,
    IMAGE_ID_WALL_1,
    IMAGE_ID_WALL_2,
    IMAGE_ID_WALL_3,
    IMAGE_ID_WALL_4,
    IMAGE_ID_PLAYER,
    IMAGE_ID_FINISH,
    IMAGE_ID_MOVING_BLOCK,
    IMAGE_ID_SPIKES_DOWN,
    IMAGE_ID_SPIKES_UP,
    IMAGE_ID_HOG,
    IMAGE_ID_YYAM,
    IMAGE_ID_COUNT,
};

enum FontId
{
    FONT_ID_SMALL,
    FONT_ID_COUNT,
};

enum AudioId
{
    AUDIO_ID_TITLE_SONG,
    AUDIO_ID_LEVEL_1_SONG,
    AUDIO_ID_LEVEL_2_SONG,
    AUDIO_ID_LEVEL_3_SONG,
    AUDIO_ID_LEVEL_4_SONG,
    AUDIO_ID_COWBELL,
    AUDIO_ID_SPLASH,
    AUDIO_ID_FAIL,
    AUDIO_ID_COUNT,
};

enum StateId
{
    STATE_ID_PRE_LOAD,
    STATE_ID_LOADING,
    STATE_ID_SPLASH,
    STATE_ID_TITLE,
    STATE_ID_SELECT,
    STATE_ID_PLAY,
    STATE_ID_WIN,
    STATE_ID_LOSE,
    STATE_ID_COUNT,
};

struct LevelEntity
{
    s32 tile_x;
    s32 tile_y;
    s32 tick_capacitor;
};

struct Level
{
    s32 width;
    s32 height;
    s32 player_pos_start_x;
    s32 player_pos_start_y;
    bool walls[LEVEL_TILE_COUNT];
    
    struct LevelEntity finish[LEVEL_MAX_FINISH_TILES];
    s32 finish_count;
    
    struct LevelSpike
    {
        struct LevelEntity entity;
        bool is_up_start;
        bool is_up;
    } spikes[LEVEL_MAX_SPIKE_TILES];
    s32 spikes_count;
    
    struct LevelMovingBlock
    {
        struct LevelEntity entity;
        s32 tile_x_start;
        s32 tile_y_start;
        s32 y_direction_start;
        s32 y_direction;
    } moving_blocks[LEVEL_MAX_MOVING_BLOCK_TILES];
    s32 moving_blocks_count;
};

#if PRINT_SIZE_OF_LEVEL_STRUCT
// https://stackoverflow.com/questions/20979565/how-can-i-print-the-result-of-sizeof-at-compile-time-in-c
char (*__kaboom)[sizeof(struct Level)] = 1;
#endif

static struct Image framebuffer;
static s32 keyboard_state[255] = {0};
static struct Image image[IMAGE_ID_COUNT] = {0};
static struct ImageAsciiMonospacedFont font[FONT_ID_COUNT] = {0};
static s32 audio[AUDIO_ID_COUNT] = {0};
static s32 last_frame_start_time_ms = 0;
static s32 last_frame_duration_ms = 0;
static f32 delta_time_s = 0.f;

void on_frame_state_pre_load(void);
void on_frame_state_loading(void);
void on_frame_state_splash(void);
void on_frame_state_title(void);
void on_frame_state_select(void);
void on_frame_state_play(void);
void on_frame_state_win(void);
void on_frame_state_lose(void);

void draw_menu_bg(void);
bool load_level_from_image(enum ImageId image_id);
void draw_level(void);
void restart_level(void);
void draw_8x8_tile(enum ImageId image_id, s32 x, s32 y);
bool *get_level_wall_at_pos(s32 x, s32 y);
struct LevelSpike *get_level_spike_at_pos(s32 x, s32 y);
struct LevelMovingBlock *get_level_moving_block_at_pos(s32 x, s32 y);
struct LevelEntity *get_level_finish_at_pos(s32 x, s32 y);

static enum StateId current_state = STATE_ID_PRE_LOAD;
void (*state_on_frame[STATE_ID_COUNT])(void) = {
    [STATE_ID_PRE_LOAD] = on_frame_state_pre_load,
    [STATE_ID_LOADING] = on_frame_state_loading,
    [STATE_ID_SPLASH] = on_frame_state_splash,
    [STATE_ID_TITLE] = on_frame_state_title,
    [STATE_ID_SELECT] = on_frame_state_select,
    [STATE_ID_PLAY] = on_frame_state_play,
    [STATE_ID_WIN] = on_frame_state_win,
    [STATE_ID_LOSE] = on_frame_state_lose,
};

static struct Level *current_level = NULL;
static f32 level_beat_len_ms[LEVEL_COUNT] = {
    BPM_TO_BEAT_LEN_MS(100.f),
    BPM_TO_BEAT_LEN_MS(120.f),
    BPM_TO_BEAT_LEN_MS(140.f),
    BPM_TO_BEAT_LEN_MS(150.f),
};
static enum ImageId level_wall_image[LEVEL_COUNT] = {
    IMAGE_ID_WALL_1,
    IMAGE_ID_WALL_2,
    IMAGE_ID_WALL_3,
    IMAGE_ID_WALL_4,
};
static enum AudioId level_music_audio[LEVEL_COUNT] = {
    AUDIO_ID_LEVEL_1_SONG,
    AUDIO_ID_LEVEL_2_SONG,
    AUDIO_ID_LEVEL_3_SONG,
    AUDIO_ID_LEVEL_4_SONG,
};
static f32 current_level_beat_len_ms;
static enum ImageId current_level_wall_image_id;
static enum AudioId current_level_music_audio_id;
static s32 player_tile_pos_x;
static s32 player_tile_pos_y;
static f32 camera_pos_x;
static f32 camera_pos_y;
static s32 level_start_time_ms; // TODO: Remove if unused
static f32 last_tick_time_ms;
static s32 player_tick_capacitor;
static f32 hog_pos = 200.f;
static s32 hog_timer_start_ms;
static s32 level_idx_unlocked;
static s32 splash_timer_start_ms;

void js_on_keyboard_event(s32 ascii_code, s32 new_state)
{
    if (keyboard_state[ascii_code] == 0 && new_state == 1) keyboard_state[ascii_code] = 2;
    else keyboard_state[ascii_code] = new_state;
}

void *js_on_image_loaded(s32 id, s32 width, s32 height)
{
    image[id].width = width;
    image[id].height = height;
    image[id].data = mem_alloc(image_calculate_size(&image[id]));
    ASSERT(image[id].data != NULL);
    return image[id].data;
}

void js_on_startup(void)
{
    rng_set_seed(js_get_unix_time());
    
    level_idx_unlocked = js_localstore_get_s32("squares_progres");
    level_idx_unlocked = 500;
    
    js_canvas_resize(CANVAS_WIDTH, CANVAS_HEIGHT, CANVAS_SCALE);
    
    framebuffer.data = mem_alloc(CANVAS_WIDTH * CANVAS_HEIGHT * 4);
    framebuffer.width = CANVAS_WIDTH;
    framebuffer.height = CANVAS_HEIGHT;
    js_set_framebuffer(framebuffer.data);
    
    // Set up font structs.
    font[FONT_ID_SMALL] = (struct ImageAsciiMonospacedFont) {
        .image = &image[IMAGE_ID_FONT_SMALL],
        .char_width = 6,
        .char_height = 8,
    };
    
    // Begin async loading of assets needed before showing the loading screen.
    js_asset_load_image("assets/font_6x8.png", IMAGE_ID_FONT_SMALL);
    
    // Allocate space for levels.
    current_level = mem_alloc(sizeof(*current_level));
    mem_set_u8((void*)current_level, sizeof(*current_level), 0);
}

void js_on_frame(void)
{
    s32 frame_start_time_ms = js_get_time_ms();
    
    delta_time_s = (f32)(frame_start_time_ms - last_frame_start_time_ms) / 1000.f;
    last_frame_start_time_ms = frame_start_time_ms;
    
    state_on_frame[current_state]();
    
    last_frame_duration_ms = js_get_time_ms() - frame_start_time_ms;
    
    for (int i = 0; i < 256; ++i) if (keyboard_state[i] == 2) keyboard_state[i] = 1;
}

void on_frame_state_pre_load(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    // Wait until assets required for the loading screen have been loaded.
    if (js_asset_count_loaded() == 1)
    {
        // Begin async loading of all remaining assets.
        js_asset_load_image("assets/test_level.png", IMAGE_ID_TEST_LEVEL);
        js_asset_load_image("assets/menu_bg.png", IMAGE_ID_MENU_BG);
        js_asset_load_image("assets/level_1.png", IMAGE_ID_LEVEL_1);
        js_asset_load_image("assets/level_2.png", IMAGE_ID_LEVEL_2);
        js_asset_load_image("assets/level_3.png", IMAGE_ID_LEVEL_3);
        js_asset_load_image("assets/level_4.png", IMAGE_ID_LEVEL_4);
        js_asset_load_image("assets/wall_1.png", IMAGE_ID_WALL_1);
        js_asset_load_image("assets/wall_2.png", IMAGE_ID_WALL_2);
        js_asset_load_image("assets/wall_3.png", IMAGE_ID_WALL_3);
        js_asset_load_image("assets/wall_4.png", IMAGE_ID_WALL_4);
        js_asset_load_image("assets/player.png", IMAGE_ID_PLAYER);
        js_asset_load_image("assets/finish.png", IMAGE_ID_FINISH);
        js_asset_load_image("assets/moving_block.png", IMAGE_ID_MOVING_BLOCK);
        js_asset_load_image("assets/spikes_down.png", IMAGE_ID_SPIKES_DOWN);
        js_asset_load_image("assets/spikes_up.png", IMAGE_ID_SPIKES_UP);
        js_asset_load_image("assets/hog.png", IMAGE_ID_HOG);
        js_asset_load_image("assets/yyam.png", IMAGE_ID_YYAM);
        
        audio[AUDIO_ID_TITLE_SONG] = js_asset_load_audio("assets/title.ogg");
        audio[AUDIO_ID_LEVEL_1_SONG] = js_asset_load_audio("assets/level_1_song.ogg");
        audio[AUDIO_ID_LEVEL_2_SONG] = js_asset_load_audio("assets/level_2_song.ogg");
        audio[AUDIO_ID_LEVEL_3_SONG] = js_asset_load_audio("assets/level_3_song.ogg");
        audio[AUDIO_ID_LEVEL_4_SONG] = js_asset_load_audio("assets/level_4_song.ogg");
        audio[AUDIO_ID_COWBELL] = js_asset_load_audio("assets/cowbell.ogg");
        audio[AUDIO_ID_SPLASH] = js_asset_load_audio("assets/splash.ogg");
        audio[AUDIO_ID_FAIL] = js_asset_load_audio("assets/fail.ogg");
        
        current_state = STATE_ID_LOADING;
    }
}

void on_frame_state_loading(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    // Wait for all assets to load.
    s32 asset_target = IMAGE_ID_COUNT + AUDIO_ID_COUNT;
    s32 asset_count = js_asset_count_loaded();
    
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "LOADING...", CANVAS_WIDTH / 2, 16, TEXT_ALIGN_CENTER);
    
    strbuf_clear();
    strbuf_push_s32((s32)(((f32)asset_count / (f32)asset_target) * 100.f));
    strbuf_push_string("%");
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], strbuf_get(), CANVAS_WIDTH / 2, 16 + 8, TEXT_ALIGN_CENTER);
    
    if (asset_count == asset_target)
    {
        video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "Press any", CANVAS_WIDTH / 2, 40, TEXT_ALIGN_CENTER);
        video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "key", CANVAS_WIDTH / 2, 48, TEXT_ALIGN_CENTER);
        
        // Wait for any key to be pressed.
        for (int i = 0; i < countof(keyboard_state); ++i)
        {
            if (keyboard_state[i] == 2)
            {
                current_state = STATE_ID_SPLASH;
                splash_timer_start_ms = js_get_time_ms();
            }
        }
    }
}

void on_frame_state_splash(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(8, 20, 30));
    
    video_blit(
        &framebuffer,
        &image[IMAGE_ID_YYAM],
        17,
        28,
        0,
        0,
        30,
        7,
        BLIT_FLIP_NONE);
    
    static bool has_played_sound = false;
    s32 splash_time_ms = js_get_time_ms() - splash_timer_start_ms;
    
    if (splash_time_ms < 500)
    {
        struct Color overlay_color = {0, 0, 0, 194};
        video_draw_rect(&framebuffer, 0, 0, 64, 64, overlay_color);
    }
    
    if (splash_time_ms > 500 && !has_played_sound)
    {
        has_played_sound = true;
        js_audio_play(audio[AUDIO_ID_SPLASH]);
    }
    
    if (splash_time_ms > 3000)
    {
        current_state = STATE_ID_TITLE;
        hog_timer_start_ms = js_get_time_ms();
        js_audio_stop(audio[AUDIO_ID_SPLASH]);
        js_audio_play(audio[AUDIO_ID_TITLE_SONG]);
    }
    
    for (int i = 0; i < countof(keyboard_state); ++i)
    {
        if (keyboard_state[i] == 2)
        {
            current_state = STATE_ID_TITLE;
            hog_timer_start_ms = js_get_time_ms();
            js_audio_stop(audio[AUDIO_ID_SPLASH]);
            js_audio_play(audio[AUDIO_ID_TITLE_SONG]);
        }
    }
}

// Helper function for animating the letters on the title screen.
static s32 letter_get_pos(f32 angle)
{
    return (s32)(12.f + (math_sin(angle) * 8.f));
}

void on_frame_state_title(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    // Easter egg.
    if (js_get_time_ms() - hog_timer_start_ms > 15000)
    {
        hog_timer_start_ms = js_get_time_ms();
        hog_pos = -30.f;
    }
    
    draw_menu_bg();
    
    // Draw wavey text.
    const s32 spacing = 9;
    const s32 x_offset = 1;
    const f32 ang_space = .7f;
    static f32 ang = 0.f;
    ang += 5.f * delta_time_s;
    
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "S", x_offset + spacing * 0, letter_get_pos(ang + 0 * ang_space), TEXT_ALIGN_LEFT);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "Q", x_offset + spacing * 1, letter_get_pos(ang + 1 * ang_space), TEXT_ALIGN_LEFT);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "U", x_offset + spacing * 2, letter_get_pos(ang + 2 * ang_space), TEXT_ALIGN_LEFT);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "A", x_offset + spacing * 3, letter_get_pos(ang + 3 * ang_space), TEXT_ALIGN_LEFT);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "R", x_offset + spacing * 4, letter_get_pos(ang + 4 * ang_space), TEXT_ALIGN_LEFT);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "E", x_offset + spacing * 5, letter_get_pos(ang + 5 * ang_space), TEXT_ALIGN_LEFT);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "S", x_offset + spacing * 6, letter_get_pos(ang + 6 * ang_space), TEXT_ALIGN_LEFT);
    
    // Draw flashing text.
    static bool is_text_visible = true;
    static float time_capacitor_s = 0.f;
    time_capacitor_s += delta_time_s;
    if (time_capacitor_s > 0.5f)
    {
        time_capacitor_s -= 0.5f;
        is_text_visible = !is_text_visible;
    }
    if (is_text_visible)
    {
        video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "Press any", CANVAS_WIDTH / 2, 40, TEXT_ALIGN_CENTER);
        video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "key", CANVAS_WIDTH / 2, 48, TEXT_ALIGN_CENTER);
    }
    
    // Wait for any key to be pressed.
    for (int i = 0; i < countof(keyboard_state); ++i)
    {
        if (keyboard_state[i] == 2)
        {
            current_state = STATE_ID_SELECT;
        }
    }
}

void on_frame_state_select(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    draw_menu_bg();
    
    static s32 level_idx = 0;
    
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "SELECT", CANVAS_WIDTH / 2, 4, TEXT_ALIGN_CENTER);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "STAGE", CANVAS_WIDTH / 2, 4 + 8, TEXT_ALIGN_CENTER);
    if (level_idx > level_idx_unlocked)
    {
        video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "LOCKED", CANVAS_WIDTH / 2, CANVAS_HEIGHT - 12, TEXT_ALIGN_CENTER);
    }
    
    strbuf_clear();
    strbuf_push_string("< ");
    strbuf_push_s32(level_idx + 1);
    strbuf_push_string(" >");
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], strbuf_get(), CANVAS_WIDTH / 2, TEXT_Y_MIDDLE + 8, TEXT_ALIGN_CENTER);
    
    if (keyboard_state[37] == 2 && level_idx > 0) level_idx -= 1;
    if (keyboard_state[39] == 2 && level_idx < LEVEL_COUNT - 1) level_idx += 1;
    
    if (keyboard_state[13] == 2 && level_idx <= level_idx_unlocked)
    {
        current_state = STATE_ID_PLAY;
        js_audio_stop(audio[AUDIO_ID_TITLE_SONG]);
        enum ImageId level_image_id;
        if (level_idx == 0) level_image_id = IMAGE_ID_LEVEL_1;
        else if (level_idx == 1) level_image_id = IMAGE_ID_LEVEL_2;
        else if (level_idx == 2) level_image_id = IMAGE_ID_LEVEL_3;
        else if (level_idx == 3) level_image_id = IMAGE_ID_LEVEL_4;
        else ASSERT(false);
        current_level_beat_len_ms = level_beat_len_ms[level_idx];
        current_level_wall_image_id = level_wall_image[level_idx];
        current_level_music_audio_id = level_music_audio[level_idx];
        bool success = load_level_from_image(level_image_id);
        ASSERT(success);
        restart_level();
    }
}

void on_frame_state_play(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    s32 time_now_ms = js_get_time_ms();
    //s32 time_now_ms = js_audio_get_time(audio[AUDIO_ID_LEVEL_1_SONG]);
    
    // Handle quitting to menu.
    if (keyboard_state[27] == 2)
    {
        current_state = STATE_ID_SELECT;
        js_audio_play(audio[AUDIO_ID_TITLE_SONG]);
        js_audio_stop(audio[current_level_music_audio_id]);
        return;
    }
    
    // Handle player collisions with entities.
    s32 player_tile_idx = player_tile_pos_y * current_level->width + player_tile_pos_x;
    struct LevelEntity *finish = get_level_finish_at_pos(player_tile_pos_x, player_tile_pos_y);
    struct LevelSpike *spikes = get_level_spike_at_pos(player_tile_pos_x, player_tile_pos_y);
    struct LevelMovingBlock *moving_block = get_level_moving_block_at_pos(player_tile_pos_x, player_tile_pos_y);
    if (current_level->walls[player_tile_idx] || (spikes != NULL && spikes->is_up) || moving_block != NULL)
    {
        if (!GOD_MODE)
        {
            current_state = STATE_ID_LOSE;
            js_audio_stop(audio[current_level_music_audio_id]);
            js_audio_play(audio[AUDIO_ID_FAIL]);
            return;
        }
    }
    if (finish != NULL)
    {
        current_state = STATE_ID_WIN;
        level_idx_unlocked += 1;
        js_localstore_set_s32("squares_progress", level_idx_unlocked);
        return;
    }
    
    // Handle ticks.
    f32 next_tick_time_ms = last_tick_time_ms + (current_level_beat_len_ms / 4.f);
    if ((f32)time_now_ms >= next_tick_time_ms)
    {
        last_tick_time_ms = next_tick_time_ms;
        
        player_tick_capacitor += 1;
        for (int i = 0; i < current_level->finish_count; ++i) current_level->finish[i].tick_capacitor += 1;
        for (int i = 0; i < current_level->spikes_count; ++i) current_level->spikes[i].entity.tick_capacitor += 1;
        for (int i = 0; i < current_level->moving_blocks_count; ++i) current_level->moving_blocks[i].entity.tick_capacitor += 1;
    }
    
    // Handle player movement.
    if ((!SPACE_TO_MOVE && player_tick_capacitor >= 4) ||
        (SPACE_TO_MOVE && keyboard_state[32] == 2))
    {
#if PLAY_COWBELL
        js_audio_play(audio[AUDIO_ID_COWBELL]);
#endif
        player_tile_pos_x += 1;
        player_tick_capacitor = 0;
    }
    if (keyboard_state[38] == 2) player_tile_pos_y -= 1;
    if (keyboard_state[40] == 2) player_tile_pos_y += 1;
    
    // Handle moving blocks.
    for (int i = 0; i < current_level->moving_blocks_count; ++i)
    {
        if (current_level->moving_blocks[i].entity.tick_capacitor >= 4)
        {
            current_level->moving_blocks[i].entity.tick_capacitor -= 4;
            
            bool *wall = get_level_wall_at_pos(
                current_level->moving_blocks[i].entity.tile_x,
                current_level->moving_blocks[i].entity.tile_y + current_level->moving_blocks[i].y_direction);
            
            if (wall != NULL) current_level->moving_blocks[i].y_direction = -current_level->moving_blocks[i].y_direction;
            
            wall = get_level_wall_at_pos(
                current_level->moving_blocks[i].entity.tile_x,
                current_level->moving_blocks[i].entity.tile_y + current_level->moving_blocks[i].y_direction);
            
            if (wall == NULL) current_level->moving_blocks[i].entity.tile_y += current_level->moving_blocks[i].y_direction;
        }
    }
    
    // Handle spikes.
    for (int i = 0; i < current_level->spikes_count; ++i)
    {
        if (current_level->spikes[i].entity.tick_capacitor >= 4)
        {
            current_level->spikes[i].entity.tick_capacitor -= 4;
            current_level->spikes[i].is_up = !current_level->spikes[i].is_up;
        }
    }
    
    draw_level();
    
    // Move camera towards centering on the player.
    f32 camera_target_x = (f32)((player_tile_pos_x * 8) - (CANVAS_WIDTH / 2 - 4));
    f32 camera_target_y = (f32)((player_tile_pos_y * 8) - (CANVAS_HEIGHT / 2 - 4));
    f32 camera_delta_x = camera_target_x - camera_pos_x;
    f32 camera_delta_y = camera_target_y - camera_pos_y;
    camera_pos_x += camera_delta_x * .25f;
    camera_pos_y += camera_delta_y * .25f;
}

void on_frame_state_win(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    draw_level();
    
    struct Color overlay_color = {0, 0, 0, 196};
    video_draw_rect(&framebuffer, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, overlay_color);
    
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "LEVEL", CANVAS_WIDTH / 2, 4, TEXT_ALIGN_CENTER);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "COMPLETE", CANVAS_WIDTH / 2, 4 + 8, TEXT_ALIGN_CENTER);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "ESC: Menu", CANVAS_WIDTH / 2, 4 + 32, TEXT_ALIGN_CENTER);
    
    if (keyboard_state[27] == 2)
    {
        current_state = STATE_ID_SELECT;
        js_audio_stop(audio[current_level_music_audio_id]);
        js_audio_play(audio[AUDIO_ID_TITLE_SONG]);
    }
}

void on_frame_state_lose(void)
{
    video_clear_framebuffer(&framebuffer, video_make_color(0, 0, 0));
    
    draw_level();
    
    struct Color overlay_color = {0, 0, 0, 196};
    video_draw_rect(&framebuffer, 0, 0, CANVAS_WIDTH, CANVAS_HEIGHT, overlay_color);
    
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "GAME OVER", CANVAS_WIDTH / 2, 4, TEXT_ALIGN_CENTER);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "RTN: Again", CANVAS_WIDTH / 2, 4 + 24, TEXT_ALIGN_CENTER);
    video_draw_text(&framebuffer, &font[FONT_ID_SMALL], "ESC: Menu", CANVAS_WIDTH / 2, 4 + 24 + 9, TEXT_ALIGN_CENTER);
    
    if (keyboard_state[13] == 2)
    {
        current_state = STATE_ID_PLAY;
        restart_level();
    }
    
    if (keyboard_state[27] == 2)
    {
        js_audio_play(audio[AUDIO_ID_TITLE_SONG]);
        current_state = STATE_ID_SELECT;
    }
}

bool load_level_from_image(enum ImageId image_id)
{
    bool found_player_start = false;
    
    current_level->width = image[image_id].width;
    current_level->height = image[image_id].height;
    
    if (current_level->width > LEVEL_MAX_WIDTH ||
        current_level->height > LEVEL_MAX_HEIGHT)
    {
        strbuf_clear();
        strbuf_push_string("Error loading level with image id ");
        strbuf_push_s32(image_id);
        strbuf_push_string(". Level dimensions are too big!");
        js_show_alert(strbuf_get());
        return false;
    }
    
    struct Color *pixels = (struct Color*)image[image_id].data;
    
    current_level->finish_count = 0;
    current_level->spikes_count = 0;
    current_level->moving_blocks_count = 0;
    
    for (int i = 0; i < LEVEL_TILE_COUNT; ++i) current_level->walls[i] = false;
    
    for (int i = 0; i < current_level->width * current_level->height; ++i)
    {
        s32 tile_x = i % current_level->width;
        s32 tile_y = i / current_level->width;
        
        // Empty space.
        if (pixels[i].r == 0 &&
            pixels[i].g == 0 &&
            pixels[i].b == 0)
        {
            continue;
        }
        
        // Wall.
        if (pixels[i].r == 255 &&
            pixels[i].g == 255 &&
            pixels[i].b == 255)
        {
            current_level->walls[i] = true;
            continue;
        }
        
        // Player start.
        // TODO: Add error when multiple player starts found.
        if (pixels[i].r == 34 &&
            pixels[i].g == 177 &&
            pixels[i].b == 76)
        {
            current_level->player_pos_start_x = tile_x;
            current_level->player_pos_start_y = tile_y;
            continue;
        }
        
        // Finish.
        if (pixels[i].r == 36 &&
            pixels[i].g == 123 &&
            pixels[i].b == 21)
        {
            int idx = current_level->finish_count;
            current_level->finish[idx].tile_x = tile_x;
            current_level->finish[idx].tile_y = tile_y;
            current_level->finish_count += 1;
            continue;
        }
        
        // Moving block. (Up direction)
        if (pixels[i].r == 255 &&
            pixels[i].g == 218 &&
            pixels[i].b == 91)
        {
            int idx = current_level->moving_blocks_count;
            current_level->moving_blocks[idx].y_direction_start = -1;
            current_level->moving_blocks[idx].y_direction = current_level->moving_blocks[idx].y_direction_start;
            current_level->moving_blocks[idx].entity.tile_x = tile_x;
            current_level->moving_blocks[idx].entity.tile_y = tile_y;
            current_level->moving_blocks[idx].tile_x_start = tile_x;
            current_level->moving_blocks[idx].tile_y_start = tile_y;
            current_level->moving_blocks_count += 1;
            continue;
        }
        
        // Moving block. (Down direction)
        if (pixels[i].r == 138 &&
            pixels[i].g == 107 &&
            pixels[i].b == 0)
        {
            int idx = current_level->moving_blocks_count;
            current_level->moving_blocks[idx].y_direction_start = 1;
            current_level->moving_blocks[idx].y_direction = current_level->moving_blocks[idx].y_direction_start;
            current_level->moving_blocks[idx].entity.tile_x = tile_x;
            current_level->moving_blocks[idx].entity.tile_y = tile_y;
            current_level->moving_blocks[idx].tile_x_start = tile_x;
            current_level->moving_blocks[idx].tile_y_start = tile_y;
            current_level->moving_blocks_count += 1;
            continue;
        }
        
        // Moving block. (Random direction)
        if (pixels[i].r == 255 &&
            pixels[i].g == 201 &&
            pixels[i].b == 14)
        {
            int idx = current_level->moving_blocks_count;
            
            u32 direction = rng_get_u32_range(0, 1);
            if (direction == 0) current_level->moving_blocks[idx].y_direction_start = -1;
            if (direction == 1) current_level->moving_blocks[idx].y_direction_start = 1;
            
            current_level->moving_blocks[idx].y_direction = current_level->moving_blocks[idx].y_direction_start;
            current_level->moving_blocks[idx].entity.tile_x = tile_x;
            current_level->moving_blocks[idx].entity.tile_y = tile_y;
            current_level->moving_blocks[idx].tile_x_start = tile_x;
            current_level->moving_blocks[idx].tile_y_start = tile_y;
            current_level->moving_blocks_count += 1;
            continue;
        }
        
        // Spikes.
        if (pixels[i].r == 127 &&
            pixels[i].g == 127 &&
            pixels[i].b == 127)
        {
            int idx = current_level->spikes_count;
            current_level->spikes[idx].is_up_start = false;
            current_level->spikes[idx].entity.tile_x = tile_x;
            current_level->spikes[idx].entity.tile_y = tile_y;
            current_level->spikes_count += 1;
            continue;
        }
        
        // Off-beat spikes.
        if (pixels[i].r == 195 &&
            pixels[i].g == 195 &&
            pixels[i].b == 195)
        {
            int idx = current_level->spikes_count;
            current_level->spikes[idx].is_up_start = true;
            current_level->spikes[idx].entity.tile_x = tile_x;
            current_level->spikes[idx].entity.tile_y = tile_y;
            current_level->spikes_count += 1;
            continue;
        }
        
        // If we get here then we found an invalid pixel.
        strbuf_clear();
        strbuf_push_string("Error loading level with image id ");
        strbuf_push_s32(image_id);
        strbuf_push_string(". Found invalid pixel at position (");
        strbuf_push_s32(i % current_level->width);
        strbuf_push_string(", ");
        strbuf_push_s32(i / current_level->width);
        strbuf_push_string(").");
        js_show_alert(strbuf_get());
        return false;
    }
    
    // TODO: Add error when no player starts found.
    
    return true;
}

void draw_level(void)
{
    // Draw level at camera position.
    s32 offset_x = -math_round_f32_to_s32(camera_pos_x);
    s32 offset_y = -math_round_f32_to_s32(camera_pos_y);
    
    // Draw walls.
    for (int i = 0; i < LEVEL_TILE_COUNT; ++i)
    {
        s32 tile_x = i % current_level->width;
        s32 tile_y = i / current_level->width;
        s32 pos_x = (tile_x * 8) + offset_x;
        s32 pos_y = (tile_y * 8) + offset_y;
        
        if (pos_x < -8 || pos_y < -8 ||
            pos_x >= CANVAS_WIDTH || pos_y >= CANVAS_HEIGHT)
        {
            // Skip this tile if it is completely outside the screen.
            continue;
        }
        
        if (current_level->walls[i]) draw_8x8_tile(current_level_wall_image_id, pos_x, pos_y);
    }
    
    // Draw finishes.
    for (int i = 0; i < current_level->finish_count; ++i)
    {
        s32 pos_x = offset_x + (current_level->finish[i].tile_x * 8);
        s32 pos_y = offset_y + (current_level->finish[i].tile_y * 8);
        
        if (pos_x < -8 || pos_y < -8 ||
            pos_x >= CANVAS_WIDTH || pos_y >= CANVAS_HEIGHT)
        {
            // Skip this tile if it is completely outside the screen.
            continue;
        }
        
        draw_8x8_tile(IMAGE_ID_FINISH, pos_x, pos_y);
    }
    
    // Draw spikes.
    for (int i = 0; i < current_level->spikes_count; ++i)
    {
        s32 pos_x = offset_x + (current_level->spikes[i].entity.tile_x * 8);
        s32 pos_y = offset_y + (current_level->spikes[i].entity.tile_y * 8);
        
        if (pos_x < -8 || pos_y < -8 ||
            pos_x >= CANVAS_WIDTH || pos_y >= CANVAS_HEIGHT)
        {
            // Skip this tile if it is completely outside the screen.
            continue;
        }
        
        if (current_level->spikes[i].is_up) draw_8x8_tile(IMAGE_ID_SPIKES_UP, pos_x, pos_y);
        else draw_8x8_tile(IMAGE_ID_SPIKES_DOWN, pos_x, pos_y);
    }
    
    // Draw moving blocks.
    for (int i = 0; i < current_level->moving_blocks_count; ++i)
    {
        s32 pos_x = offset_x + (current_level->moving_blocks[i].entity.tile_x * 8);
        s32 pos_y = offset_y + (current_level->moving_blocks[i].entity.tile_y * 8);
        
        if (pos_x < -8 || pos_y < -8 ||
            pos_x >= CANVAS_WIDTH || pos_y >= CANVAS_HEIGHT)
        {
            // Skip this tile if it is completely outside the screen.
            continue;
        }
        
        draw_8x8_tile(IMAGE_ID_MOVING_BLOCK, pos_x, pos_y);
    }
    
    // Draw player.
    {
        s32 pos_x = (player_tile_pos_x * 8) + offset_x;
        s32 pos_y = (player_tile_pos_y * 8) + offset_y;
        draw_8x8_tile(IMAGE_ID_PLAYER, pos_x, pos_y);
    }
}

void restart_level(void)
{
    player_tick_capacitor = 0;
    
    for (int i = 0; i < current_level->finish_count; ++i) current_level->finish[i].tick_capacitor = 0;
    
    for (int i = 0; i < current_level->spikes_count; ++i)
    {
        current_level->spikes[i].entity.tick_capacitor = 0;
        current_level->spikes[i].is_up = current_level->spikes[i].is_up_start;
    }
    
    for (int i = 0; i < current_level->moving_blocks_count; ++i)
    {
        current_level->moving_blocks[i].entity.tick_capacitor = 0;
        current_level->moving_blocks[i].entity.tile_x = current_level->moving_blocks[i].tile_x_start;
        current_level->moving_blocks[i].entity.tile_y = current_level->moving_blocks[i].tile_y_start;
        current_level->moving_blocks[i].y_direction = current_level->moving_blocks[i].y_direction_start;
    }
    
    player_tile_pos_x = current_level->player_pos_start_x;
    player_tile_pos_y = current_level->player_pos_start_y;
    
    camera_pos_x = (f32)((player_tile_pos_x * 8) - (CANVAS_WIDTH / 2 - 4));
    camera_pos_y = (f32)((player_tile_pos_y * 8) - (CANVAS_HEIGHT / 2 - 4));
    
    js_audio_play(audio[current_level_music_audio_id]);
    level_start_time_ms = js_get_time_ms();
    //level_start_time_ms = 0;
    //level_start_time_ms = js_audio_get_time(audio[AUDIO_ID_LEVEL_1_SONG]);
    last_tick_time_ms = (f32)level_start_time_ms;
    //last_tick_time_ms = 0;
    //sync_offset_ms = 0;
    //last_sync_time_ms = 0;
}

void draw_8x8_tile(enum ImageId image_id, s32 x, s32 y)
{
    video_blit(
        &framebuffer,
        &image[image_id],
        x,
        y,
        0,
        0,
        8,
        8,
        BLIT_FLIP_NONE);
}

bool *get_level_wall_at_pos(s32 x, s32 y)
{
    s32 idx = y * current_level->width + x;
    if (current_level->walls[idx]) return &current_level->walls[idx];
    else return NULL;
}

struct LevelSpike *get_level_spike_at_pos(s32 x, s32 y)
{
    for (int i = 0; i < current_level->spikes_count; ++i)
    {
        if (current_level->spikes[i].entity.tile_x == x &&
            current_level->spikes[i].entity.tile_y == y)
        {
            return &current_level->spikes[i];
        }
    }
    return NULL;
}

struct LevelMovingBlock *get_level_moving_block_at_pos(s32 x, s32 y)
{
    for (int i = 0; i < current_level->moving_blocks_count; ++i)
    {
        if (current_level->moving_blocks[i].entity.tile_x == x &&
            current_level->moving_blocks[i].entity.tile_y == y)
        {
            return &current_level->moving_blocks[i];
        }
    }
    return NULL;
}

struct LevelEntity *get_level_finish_at_pos(s32 x, s32 y)
{
    for (int i = 0; i < current_level->finish_count; ++i)
    {
        if (current_level->finish[i].tile_x == x &&
            current_level->finish[i].tile_y == y)
        {
            return &current_level->finish[i];
        }
    }
    return NULL;
}

void draw_menu_bg(void)
{
    static float pos = 0.f;
    
    pos -= 30.f * delta_time_s;
    if (pos < -24.f) pos += 24.f;
    
    if (hog_pos < 100.f)
    {
        hog_pos += 40.f * delta_time_s;
        video_blit(
            &framebuffer,
            &image[IMAGE_ID_HOG],
            (s32)hog_pos,
            (s32)hog_pos,
            0,
            0,
            20,
            20,
            BLIT_FLIP_NONE);
    }
    
    video_blit(
        &framebuffer,
        &image[IMAGE_ID_MENU_BG],
        (s32)pos,
        (s32)pos,
        0,
        0,
        88,
        88,
        BLIT_FLIP_NONE);
    
    struct Color overlay_color = {0, 0, 0, 194};
    video_draw_rect(&framebuffer, 0, 0, 64, 64, overlay_color);
}