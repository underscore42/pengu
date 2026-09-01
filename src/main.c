/* main.c - Pengu (NGPC) - Studio So Not Kansai / underscore42
 *
 * A Pengo homage.  Single screen, 10x9 cells of 16px, four fields, one tile
 * set retinted four ways.  Walk into a block and it slides; if it is jammed,
 * A crushes it.  Kill Sno-Bees by sliding a block into them, crushing their
 * egg before it hatches, or punching the wall to stun one and walking over
 * it.  Clear every Sno-Bee AND every egg to take the round.
 *
 * Link order matters: main.c first.
 */
#define CARTHDR_IMPL
#include "carthdr.h"

#include "game.h"
#include "screen.h"
#include "entities.h"
#include "snobee.h"
#include "tiles.h"
#include "kana.h"
#include "sound.h"
#include "save.h"

/* ---- forward declarations: every function, or cc900 assumes int and then
 * conflicts with the void definition ---- */
void main(void);
static void enter_title(void);
static void enter_scores(void);
static void enter_level(void);
static void enter_dead(void);
static void enter_clear(void);
static void enter_over(void);
static void run_title(void);
static void run_scores(void);
static void run_game(void);
static void run_dead(void);
static void run_clear(void);
static void run_over(void);
static void start_bees(void);
static void start_new_game(void);

/* Hatch the round's opening Sno-Bees straight out of egg blocks, arcade
 * style - they are not placed, they emerge. */
static void start_bees(void) {
    u8 i, n;
    bees_reset();
    n = level_start_bees();
    for (i = 0; i < n; i++) hatch_one();
}

static void start_new_game(void) {
    new_game();
    enter_level();
}

static void enter_title(void) {
    state = STATE_TITLE;
    skip  = 10;
    state_tmr = 0;
    clear_all_sprites();
    draw_title();
    anim_title();   /* same reason as enter_level: the debounce frames return
                     * before run_title() draws, so seed the chase sprites */
}

static void enter_scores(void) {
    state = STATE_SCORES;
    skip  = 10;
    state_tmr = 200;
    draw_scores();
}

static void enter_level(void) {
    state = STATE_GAME;
    skip  = 10;
    clear_all_sprites();
    init_level();
    apply_theme(level);
    wipe_screen();
    draw_playfield();
    init_hud();
    draw_hud();
    start_bees();
    /* The skip frames below return before run_game() draws, so put the
     * actors on screen once here - otherwise the field sits empty for the
     * length of the debounce. */
    draw_player();
    draw_bees();
    PlaySound(SND_START);
}

static void enter_dead(void) {
    state = STATE_DEAD;
    skip  = 6;
    state_tmr = 90;
    PlaySound(SND_DIE);
}

static void enter_clear(void) {
    state = STATE_CLEAR;
    skip  = 6;
    state_tmr = 120;
    /* time bonus: what's left of the sudden-death clock */
    if (level_frames < SUDDEN_AT)
        score = score + ((SUDDEN_AT - level_frames) >> 5);
    show_banner(s_clear);
    PlaySound(SND_CLEAR);
}

static void enter_over(void) {
    hide_player();
    state = STATE_OVER;
    skip  = 10;
    state_tmr = 150;
    show_banner(s_gameover);
    if (is_high_score(score)) {
        insert_high_score(score);
        save_high_scores();
    }
}

static void run_title(void) {
    anim_title();
    if (pad_press & J_A)      start_new_game();
    if (pad_press & J_OPTION) enter_scores();
}

static void run_scores(void) {
    if (state_tmr > 0) state_tmr = state_tmr - 1;
    if (state_tmr == 0)        enter_title();
    if (pad_press & J_OPTION)  enter_title();
    if (pad_press & J_A)       enter_title();
}

static void run_game(void) {
    if (pad_press & J_OPTION) {
        if (paused) paused = 0;
        else        paused = 1;
        show_pause(paused);
    }
    if (paused) return;

    level_frames = level_frames + 1;
    if (level_frames >= SUDDEN_AT) sudden = 1;

    pulse_egg_palette();

    /* Mid-round hatching keeps the pressure on, and makes crushing an egg
     * block a real decision rather than a fallback. */
    if (hatch_tmr > 0) hatch_tmr = hatch_tmr - 1;
    if (hatch_tmr == 0) {
        hatch_one();
        hatch_tmr = HATCH_TIME;
        if (sudden) hatch_tmr = HATCH_TIME >> 1;
    }

    update_player();
    update_slide();
    update_crush();
    update_bees();

    draw_player();
    draw_slide();
    draw_crush();
    draw_bees();
    draw_hud();

    if (!p_alive) {
        enter_dead();
        return;
    }
    /* round is won only when every Sno-Bee AND every egg is gone */
    if (bees_alive() == 0 && eggs_left() == 0) enter_clear();
}

static void run_dead(void) {
    /* penguin blinks out */
    if (state_tmr & 4) {
        UnsetSprite(SPR_PENG);
        UnsetSprite(SPR_PENG + 1);
        UnsetSprite(SPR_PENG + 2);
        UnsetSprite(SPR_PENG + 3);
    } else {
        draw_player();
    }

    if (state_tmr > 0) state_tmr = state_tmr - 1;
    if (state_tmr > 0) return;

    hide_player();   /* he is dead; do not leave him standing under the banner */

    if (lives > 0) {
        lives = lives - 1;
        enter_level();
    } else {
        enter_over();
    }
}

static void run_clear(void) {
    if (state_tmr > 0) state_tmr = state_tmr - 1;
    if (state_tmr > 0) return;
    clear_sprite_kana();
    level = level + 1;
    enter_level();
}

static void run_over(void) {
    if (state_tmr > 0) state_tmr = state_tmr - 1;
    if (state_tmr > 0) return;
    clear_sprite_kana();
    enter_scores();
}

void main(void) {
    InitNGPC();

    /* Order is load bearing: palettes, then the BIOS font, then our tiles,
     * then wipe, then draw.  Wiping before the font call bleeds the
     * Japanese charset through. */
    setup_palettes();
    SysSetSystemFont();
    install_tiles();
    wipe_screen();

    sound_init();
    load_high_scores();

    pad_cur   = 0;
    pad_prev  = 0;
    pad_press = 0;
    skip      = 0;
    score     = 0;
    frame     = 0;
    level     = 0;
    lives     = START_LIVES;
    rand_seed = 42;

    clear_all_sprites();
    enter_title();

    for (;;) {
        WaitVsync();

        pad_prev  = pad_cur;
        pad_cur   = JOYPAD & 0x7F;
        pad_press = pad_cur & ~pad_prev;

        frame = frame + 1;
        /* seed the RNG from how long the player took to press start */
        if (state == STATE_TITLE) rand_seed = rand_seed + 1;

        /* Input is read ABOVE this line so pad_prev stays correct across the
         * gap; then swallow the debounce frames.  Stops one A press walking
         * through title -> game, or clear -> next level. */
        if (skip > 0) {
            skip = skip - 1;
            continue;
        }

        if (state == STATE_TITLE)       run_title();
        else if (state == STATE_SCORES) run_scores();
        else if (state == STATE_GAME)   run_game();
        else if (state == STATE_DEAD)   run_dead();
        else if (state == STATE_CLEAR)  run_clear();
        else                            run_over();
    }
}
