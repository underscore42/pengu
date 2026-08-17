/* maps.h - Pengu authored ice fields.
 *   #  ice block (pushable, crushable when jammed)
 *   e  ice block holding a Sno-Bee egg (crush it to kill the egg early)
 *   *  diamond block (pushable, never breaks)
 *   .  open ice
 *   @  penguin start
 *
 * 10 cells wide x 9 tall.  Edit freely; init_level() scans for '@'.
 *
 * Tuned to ~26% density - the arcade's ratio.  Denser and the longest slide
 * drops to 2-3 cells, which kills the shove.  Every field is validated by
 * tools/checkmaps.py: reachability, slide lengths, jammed count.
 */
#ifndef MAPS_H
#define MAPS_H

static const char ice_maps[NUM_LEVELS][CELLS_H][CELLS_W + 1] = {
    /* 1 - ANTARCTIC: open, forgiving, long lanes to learn the shove */
    {
        "..#....#..",
        ".#..e..#..",
        "..*..#....",
        ".#...e..#.",
        "...#...*#.",
        ".#....#..#",
        "...*..e...",
        ".#..#...#.",
        "@..#....#."
    },
    /* 2 - NIGHT ICE: lattice on the even rows, open arteries on the odd
     * ones.  More jams than level 1 without ever sealing a pocket. */
    {
        ".#.#..#.#.",
        "..e....e..",
        "#.#.#.#.#.",
        "..........",
        ".#.*..*.#.",
        "..........",
        "#.#.#.#.#.",
        "..e....e..",
        "@#.#..#.#*"
    },
    /* 3 - DESERT ISLAND: sand palette on the same tiles.  Wide open sides
     * so the Sno-Bees flank you instead of queuing up. */
    {
        "..........",
        ".#.#..#.#.",
        "..e....e..",
        "#.#.*#.#.#",
        "....##....",
        "#.#.*#.#.#",
        "..e....e..",
        ".#.#..#.#.",
        "@........*"
    },
    /* 4 - VOLCANIC: mirrored twin-column lattice with a hot core.  Long
     * lanes down the odd rows, the four diamonds sit where you can
     * actually line them up. */
    {
        "#..#..#..#",
        "...e..e...",
        ".#..##..#.",
        "..*....*..",
        "#...##...#",
        "..*....*..",
        ".#..##..#.",
        "...e..e...",
        "@..#..#..#"
    }
};

/* Per level: initial hatches, then how many mid-round hatches are allowed.
 * Level 1 starts with two Sno-Bees so the first round is learnable. */
static const u8 lvl_start_bees[NUM_LEVELS] = { 2, 3, 3, 4 };

#endif
