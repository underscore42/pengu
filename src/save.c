/* save.c - Pengu flash high score table
 *
 * Hard-won layout, do not rearrange:
 *   words 0-1   0xCAFEBABE  library sentinel, required by GetSavedData()
 *   word  2     game magic  (NEVER put this at word 0 - it overwrites the
 *               sentinel and the save silently never persists)
 *   word  3     format version
 *   words 4-8   five high scores
 *   word  127   checksum of words 2..126
 * Buffer is u16[256]; the BIOS transfer is 128 words / 256 bytes.
 */
#include "save.h"

#define GAME_MAGIC 0x5047      /* 'PG' */
#define SAVE_VER   0x0001

static u16 save_buf[256];

/* ---- forward declarations ---- */
void load_high_scores(void);
void save_high_scores(void);
void insert_high_score(u16 s);
u8   is_high_score(u16 s);
static u16 sum_words(void);
static void set_defaults(void);

static u16 sum_words(void) {
    u16 i, sum;
    sum = 0;
    for (i = 2; i < 127; i++) sum = sum + save_buf[i];
    return sum;
}

static void set_defaults(void) {
    high_scores[0] = 5000;
    high_scores[1] = 4000;
    high_scores[2] = 3000;
    high_scores[3] = 2000;
    high_scores[4] = 1000;
}

void load_high_scores(void) {
    u16 i, sum;

    for (i = 0; i < 256; i++) save_buf[i] = 0;
    GetSavedData((void *)save_buf);

    if (save_buf[2] != GAME_MAGIC) { set_defaults(); return; }
    if (save_buf[3] != SAVE_VER)   { set_defaults(); return; }

    sum = save_buf[127];
    save_buf[127] = 0;
    if (sum != sum_words()) { set_defaults(); return; }

    for (i = 0; i < 5; i++) high_scores[i] = save_buf[4 + i];
}

void save_high_scores(void) {
    u16 i;
    u32 *sentinel;

    /* GetSavedData leaves the sentinel in place; set it anyway so a cold
     * save is valid. */
    GetSavedData((void *)save_buf);
    sentinel = (u32 *)save_buf;
    sentinel[0] = 0xCAFEBABEUL;

    save_buf[2] = GAME_MAGIC;
    save_buf[3] = SAVE_VER;
    for (i = 0; i < 5; i++) save_buf[4 + i] = high_scores[i];
    for (i = 9; i < 127; i++) save_buf[i] = 0;
    save_buf[127] = 0;
    save_buf[127] = sum_words();

    Flash((void *)save_buf);
}

u8 is_high_score(u16 s) {
    if (s > high_scores[4]) return 1;
    return 0;
}

void insert_high_score(u16 s) {
    u8 i, j;
    for (i = 0; i < 5; i++) {
        if (s > high_scores[i]) {
            j = 4;
            while (j > i) {
                high_scores[j] = high_scores[j - 1];
                j--;
            }
            high_scores[i] = s;
            return;
        }
    }
}
