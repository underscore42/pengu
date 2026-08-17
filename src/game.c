/* game.c - Pengu: shared state, cell model, level setup
 *
 * Fields are authored data in maps.h, not geometry.  init_level() copies the
 * current level's field into grid[][] and picks up the penguin start from
 * the '@' marker.  tools/checkmaps.py validates every field for
 * reachability and slide quality before it gets here.
 */
#include "game.h"
#include "maps.h"

/* ---- grid ---- */
u8 grid[CELLS_H][CELLS_W];

/* ---- global state ---- */
u8  state, frame, paused;
u16 score;
u8  pad_cur, pad_prev, pad_press;
u8  rand_seed;
u8  level, lives, sudden;
u16 level_frames;
u16 hatch_tmr;
u8  state_tmr, wall_cool, wall_shake;
u16 high_scores[5];

/* ---- penguin ---- */
u8 p_cx, p_cy, p_px, p_py;
u8 p_dir, p_sub, p_moving, p_face, p_anim, p_alive, p_invuln;

/* ---- sliding block ---- */
u8 sl_on, sl_cx, sl_cy, sl_px, sl_py, sl_dir, sl_sub, sl_kind;

/* ---- crushing block ---- */
u8 cr_on, cr_cx, cr_cy, cr_tmr;

/* ---- Sno-Bees ---- */
u8 bee_on[MAX_BEES];
u8 bee_cx[MAX_BEES], bee_cy[MAX_BEES];
u8 bee_px[MAX_BEES], bee_py[MAX_BEES];
u8 bee_dir[MAX_BEES], bee_sub[MAX_BEES], bee_tick[MAX_BEES];
u8 bee_anim[MAX_BEES], bee_stun[MAX_BEES];

/* ---- forward declarations ---- */
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

/* ---- RNG ----
 * (seed * 5 + 1) mod 256, full 256 cycle.  Reduction is a subtract loop
 * because cc900's signed-mod codegen is unreliable, and 5*x is (x<<2)+x
 * because there is no signed multiply.
 */
u8 cheap_rand(u8 max) {
    u8 r;
    rand_seed = ((rand_seed << 2) + rand_seed + 1) & 0xFF;
    if (max == 0) return 0;
    r = rand_seed;
    while (r >= max) r = r - max;
    return r;
}

u8 adiff(u8 a, u8 b) {
    if (a > b) return a - b;
    return b - a;
}

u8 opposite(u8 d) {
    if (d == DIR_UP)    return DIR_DOWN;
    if (d == DIR_DOWN)  return DIR_UP;
    if (d == DIR_LEFT)  return DIR_RIGHT;
    if (d == DIR_RIGHT) return DIR_LEFT;
    return DIR_NONE;
}

/* ---- cell access ----
 * Out of bounds reads as ICE, so the enclosing wall needs no cells of its
 * own: blocks stop against it and you can crush against it, exactly like a
 * real neighbour.  Costs 4 edge tiles instead of a 34-cell ring.
 */
u8 in_bounds(u8 cx, u8 cy) {
    if (cx >= CELLS_W) return 0;
    if (cy >= CELLS_H) return 0;
    return 1;
}

u8 cell_at(u8 cx, u8 cy) {
    if (!in_bounds(cx, cy)) return CELL_ICE;
    return grid[cy][cx];
}

u8 walkable(u8 cx, u8 cy) {
    if (!in_bounds(cx, cy)) return 0;
    if (grid[cy][cx] != CELL_EMPTY) return 0;
    return 1;
}

/* Unsigned coords, so a step off the top or left wraps to 255 and
 * in_bounds() rejects it.  Intentional - do not "fix" it with signed
 * arithmetic, cc900 will punish you.
 */
void step_cell(u8 d, u8 cx, u8 cy, u8 *ncx, u8 *ncy) {
    *ncx = cx;
    *ncy = cy;
    if (d == DIR_UP)         *ncy = cy - 1;
    else if (d == DIR_DOWN)  *ncy = cy + 1;
    else if (d == DIR_LEFT)  *ncx = cx - 1;
    else if (d == DIR_RIGHT) *ncx = cx + 1;
}

u8 eggs_left(void) {
    u8 x, y, n;
    n = 0;
    for (y = 0; y < CELLS_H; y++)
        for (x = 0; x < CELLS_W; x++)
            if (grid[y][x] == CELL_EGG) n++;
    return n;
}

/* maps.h is private to this module, so the table is reached through here
 * rather than leaking the header into main.c. */
u8 level_start_bees(void) {
    u8 m;
    m = level;
    while (m >= NUM_LEVELS) m = m - NUM_LEVELS;
    return lvl_start_bees[m];
}

void init_level(void) {
    u8 x, y, m;
    char c;

    m = level;
    while (m >= NUM_LEVELS) m = m - NUM_LEVELS;   /* levels wrap, arcade style */

    p_cx = 0;
    p_cy = CELLS_H - 1;

    for (y = 0; y < CELLS_H; y++) {
        for (x = 0; x < CELLS_W; x++) {
            c = ice_maps[m][y][x];
            if (c == '#')      grid[y][x] = CELL_ICE;
            else if (c == 'e') grid[y][x] = CELL_EGG;
            else if (c == '*') grid[y][x] = CELL_GEM;
            else {
                grid[y][x] = CELL_EMPTY;
                if (c == '@') {
                    p_cx = x;
                    p_cy = y;
                }
            }
        }
    }

    p_px     = p_cx << 4;
    p_py     = PF_PY0 + (p_cy << 4);
    p_dir    = DIR_NONE;
    p_face   = FACE_DOWN;
    p_sub    = 0;
    p_moving = 0;
    p_anim   = 0;
    p_alive  = 1;
    p_invuln = INVULN_TIME;

    sl_on        = 0;
    cr_on        = 0;
    paused       = 0;
    sudden       = 0;
    level_frames = 0;
    hatch_tmr    = HATCH_TIME;
    wall_cool    = 0;
    wall_shake   = 0;
}

void new_game(void) {
    score = 0;
    frame = 0;
    level = 0;
    lives = START_LIVES;
    rand_seed = 42;
    init_level();
}
