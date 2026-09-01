/* game.h - Pengu: geometry, cell model, shared state
 * Studio So Not Kansai / underscore42
 *
 * Single screen, no scrolling.  160x152 splits as:
 *   tile row 0        HUD band (8px).  Deliberately dark: scroll-plane
 *                     index 0 is opaque and col0 is forced to 0, so text
 *                     glyphs carry a black cell.  Reads as an arcade bezel.
 *   tile rows 1-18    playfield, 18 rows = 9 cell rows x 16px = 144px
 *                     8 + 144 = 152 exactly.
 *
 * Cell (cx,cy) -> tile (cx<<1, PF_Y0 + (cy<<1))
 *              -> pixel (cx<<4, PF_PY0 + (cy<<4))
 */
#ifndef GAME_H
#define GAME_H

#include "ngpc.h"
#include "library.h"

/* ---- states ---- */
#define STATE_TITLE  0
#define STATE_GAME   1
#define STATE_DEAD   2
#define STATE_CLEAR  3
#define STATE_OVER   4
#define STATE_SCORES 5

/* ---- geometry ---- */
#define CELLS_W  10
#define CELLS_H   9
#define PF_Y0     1
#define PF_PY0    8
#define HUD_Y     0

/* ---- cell contents ---- */
#define CELL_EMPTY 0
#define CELL_ICE   1
#define CELL_GEM   2
#define CELL_EGG   3    /* ice block with a Sno-Bee egg in it.  Behaves like
                         * ice for pushing and blocking; crush it and the egg
                         * dies before it hatches. */

/* ---- directions ---- */
#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

/* ---- facings, index into the penguin tile table ---- */
#define FACE_DOWN  0
#define FACE_UP    1
#define FACE_RIGHT 2
#define FACE_LEFT  3

/* ---- scroll plane 1 palettes ---- */
#define PAL_KANA_B 0
#define PAL_KANA_W 1
#define PAL_ICE    2
#define PAL_GEM    3
#define PAL_FIELD  4
#define PAL_EDGE   5
#define PAL_EGG    6    /* same ICE tiles, animated palette - an egg block
                         * costs zero tiles, it just pulses */

/* ---- sprite palettes: slots 0-3 ONLY.  4-7 wrap and clobber. ---- */
#define SP_PENG 0
#define SP_ICE  1
#define SP_BEE  2
#define SP_TEXT 3

/* ---- sprite slots ---- */
#define SPR_PENG   0    /* 4 */
#define SPR_SLIDE  4    /* 4 */
#define SPR_CRUSH  8    /* 4 */
#define SPR_BEE   12    /* MAX_BEES * 4 = 24 */
#define SPR_TEXT  40    /* up to 8 */

/* ---- speeds and timings ---- */
#define WALK_STEP   2   /* px/frame: 8 frames per cell */
#define SLIDE_STEP  4   /* px/frame: 4 frames per cell - faster than you
                         * walk, which is what sells the shove */
#define CRUSH_TIME  16
#define BEE_STEP    2
#define BEE_TICK    2   /* move every 2nd frame: half the penguin's pace */
#define BEE_TICK_SD 1   /* sudden death: matches the penguin */
#define STUN_TIME   120
#define INVULN_TIME 60
#define WALL_COOL   24
#define HATCH_TIME  300
#define SUDDEN_AT   3600  /* 60 seconds */
#define MAX_BEES    6
#define START_LIVES 3
#define NUM_LEVELS  4

/* ---- grid ---- */
extern u8 grid[CELLS_H][CELLS_W];

/* ---- global state ---- */
extern u8  state, frame, paused;
extern u8  skip;    /* frames of input to swallow after a state change,
                     * so one press cannot bleed through a transition */
extern u16 score;
extern u8  pad_cur, pad_prev, pad_press;
extern u8  rand_seed;
extern u8  level, lives, sudden;
extern u16 level_frames;
extern u16 hatch_tmr;
extern u8  state_tmr, wall_cool, wall_shake;
extern u16 high_scores[5];

/* ---- penguin ---- */
extern u8 p_cx, p_cy, p_px, p_py;
extern u8 p_dir, p_sub, p_moving, p_face, p_anim, p_alive, p_invuln;

/* ---- the sliding block: exactly one at a time ---- */
extern u8 sl_on, sl_cx, sl_cy, sl_px, sl_py, sl_dir, sl_sub, sl_kind;

/* ---- the crushing block ---- */
extern u8 cr_on, cr_cx, cr_cy, cr_tmr;

/* ---- Sno-Bees ---- */
extern u8 bee_on[MAX_BEES];
extern u8 bee_cx[MAX_BEES], bee_cy[MAX_BEES];
extern u8 bee_px[MAX_BEES], bee_py[MAX_BEES];
extern u8 bee_dir[MAX_BEES], bee_sub[MAX_BEES], bee_tick[MAX_BEES];
extern u8 bee_anim[MAX_BEES], bee_stun[MAX_BEES];

/* ---- helpers ---- */
u8   cheap_rand(u8 max);
u8   adiff(u8 a, u8 b);
u8   opposite(u8 d);
u8   cell_at(u8 cx, u8 cy);
u8   in_bounds(u8 cx, u8 cy);
u8   walkable(u8 cx, u8 cy);
void step_cell(u8 d, u8 cx, u8 cy, u8 *ncx, u8 *ncy);
u8   eggs_left(void);
u8   level_start_bees(void);
void init_level(void);
void new_game(void);

#endif
