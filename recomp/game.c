/* Gods (The Bitmap Brothers, 1991; Archimedes conversion by Krisalis -
 * Richard Walker, sound effects by Shaun Hollingworth, 1992): what libroger
 * needs to know about the game.
 *
 * !Run sets Gods$Dir, runs Initmem (a utility that shrinks the RMA, system
 * sprites, font cache and RAM disc to nothing and the screen to 80K: as
 * libroger starts out, so it isn't run here), then, with a 608K WimpSlot,
 * runs Intro and, when it exits, Gods. Both are absolutes at &8000, AIF
 * images packed with Acorn's squeeze: ../tools/unpack.py runs their own
 * decompressors, and the images they unpack are what's translated and
 * loaded here (Intro: genintro/, Gods: gen/).
 *
 * The game reads its files as <Gods$Dir>.<name> or, from disc 2,
 * adfs::0.$.!Gods.<name>; the discs are merged into one !Gods (game/). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <roger.h>

extern const struct roger_image recomp_image, recompI_image;   /* gen{,intro}/entries.c */

#define GODS "../unpacked/gods"
static const char *const vars[] = {"Gods$Dir", "!Gods", NULL};
static const char *const prefixes[] = {"adfs::0.$.", NULL};

/* !Run's "run <Gods$Dir>.Gods" once Intro exits: the unpacked image at
 * &8000, run from its start as a fresh program (R13 at the top of the
 * WimpSlot). It ends the run itself (OS_Exit), or returns, which ends it here. */
/* --level=N: a new game starts at level N, as if its password had been
 * entered. Gods' workspace is at &8080 (R12 throughout): +&130 the level
 * (1-4), +&210 set when a game is started from the title (BEGIN QUEST at
 * &F5BC, or a password at &F5A8), +&270 set when it's started from a
 * password (&AFBC then sets the game up for that level; the title sets it
 * too, &F364, and BEGIN QUEST clears it again). BEGIN QUEST's game is turned
 * into level N's before the level's files are named (&AA18), the first time
 * one is started. */
#define WS          0x8080u
#define LEVEL       (WS + 0x130)
#define STARTING    (WS + 0x210)
#define BY_PASSWORD (WS + 0x270)
static int start_level, at_title;

static int arg(const char *a)
{
    if (!strncmp(a, "--level=", 8) && atoi(a + 8) >= 1 && atoi(a + 8) <= 4) start_level = atoi(a + 8);
    else return 0;
    return 1;
}

static void frame(void)
{
    if (start_level <= 1 || roger_current_image() != &recomp_image) return;
    if (RD32(BY_PASSWORD) == 1) at_title = 1;
    else if (at_title && RD32(STARTING) == 1 && RD32(LEVEL) == 1) {
        WR32(LEVEL, start_level);
        WR32(BY_PASSWORD, 1);
        start_level = 0;
    }
}

static int game_swi(uint32_t n, int x)
{
    if (n == 0x11 && roger_current_image() == &recompI_image) {
        if (roger_load_file(GODS, ROGER_LOAD) <= 0) { perror(GODS); roger_finish("no Gods"); }
        roger_set_image(&recomp_image);
        R[13] = ROGER_LOAD + 608 * 1024;
        R[12] = 0;
        roger_call_program(ROGER_LOAD, ROGER_HANDLER_RET);
        roger_finish("returned to OS");
    }
    return 0;
}

/* The headless scenario (--limit runs; instruction counts, 2000 a ms):
 * Space past the passwords notice and the credits, Return (fire) on BEGIN
 * QUEST, then X walks right through level 1's first rooms, firing */
static const struct roger_key keys[] = {
    {99, 24000000, 24400000},                       /* Space */
    {99, 44000000, 44400000},                       /* Space */
    {74, 80000000, 80400000},                       /* Return: BEGIN QUEST */
    {67, 140000000, 200000000},                     /* X: right */
    {74, 150000000, 150600000},                     /* Return: fire */
    {74, 170000000, 170600000},
};

static struct roger_config config = {
    .title = "Gods (recompiled)",
    /* the keys (the !ReadMe): Z/X left/right, ' up, / down, Return fire;
     * F1 pauses. The game also reads Acorn's joystick module (Joystick_Read,
     * &43F40: the RTFM interface's JoyMod), which a controller stands in
     * for: the stick or D-pad moves, A fires; with --joystick=off, the keys
     * above */
    .pad_map = "left=Z,right=X,up=',down=/,a=Return,start=F1",
    .joystick = ROGER_JOY_ACORN,
    .image_path = "../unpacked/intro",
    .image = &recompI_image,
    .appdir = "../game",
    .dir_prefixes = prefixes,
    .variables = vars,
    .command_line = "<Gods$Dir>.Intro",
    .initial_mode = 12,                 /* the desktop's: Intro sets its own */
    .wimpslot = 608 * 1024,             /* !Run's *WimpSlot 608k */
    .keys = keys,
    .nkeys = sizeof keys / sizeof keys[0],
    .swi = game_swi,
    .frame = frame,
    .arg = arg,
    .usage = "  --level=N         a new game (BEGIN QUEST) starts at level N (1-4)\n",
};

int main(int argc, char **argv) { roger_main(argc, argv, &config); }
