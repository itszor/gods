Gods (The Bitmap Brothers, 1991; Archimedes conversion by Krisalis, 1992) - static recompilation
============================================================================================

A recipe: this repository holds no game data. It rebuilds the game as a
native executable from your own copy, recognised by content (recipe.json),
so it can be shared without distributing the game.

    ./build.sh "Gods (19xx)(Krisalis)(Disc 1 of 2).adf" "Gods (19xx)(Krisalis)(Disc 2 of 2).adf"
    cd recomp && ./gods

build.sh runs recipe.json's steps with SuperSpider's tools/weave.py
(~/stuff/superspider, or $SUPERSPIDER; $SS below): it extracts the discs
and merges them into one !Gods, unsqueezes the two programs with their own
decompressors (tools/unpack.py), analyses and translates them, and links
them with libroger (~/stuff/libroger, or make ROGER=...). Needs python3,
OCaml/dune, gcc and SDL3.

The inputs: TOSEC's two ADFS E discs, "Gods (19xx)(Krisalis)(Disc 1 of
2).adf" (!Gods: Initmem, Intro, Gods, the title's fonts and pictures) and
"(Disc 2 of 2).adf" (!Gods again: the levels 001-004, their graphics PI1
and data Dat, the high score table). Disc 1 also holds the RTFM and serial
port joystick drivers, and, at its root, "Gods": GodsCheat 1.13 (1992, BjD
& AjC), a cheat module someone added to this dump; it isn't used.

Not an input: TOSEC's "Gods (19xx)(Krisalis).zip", a hard disc installation
made from a cracked copy. Its Gods is wrapped in another layer (it
decrypts itself and reads the ROM and the disc before the squeezed program
runs), and its 002.calfilsb is damaged: 720 of its back-references point
past what's been decoded, where disc 2's copy decodes cleanly, as all its
other compressed files do. weave.py scores it 97% and won't build from it.

What it makes:

    extracted/disc1, disc2   the discs, via weave.py
    game/!Gods               libroger's appdir (../game): disc 2's !Gods, then disc 1's over it
    unpacked/intro, gods     tools/unpack.py: the programs as their decompressors leave them
    unpacked/gods.roots      ...and Gods' object handlers, as roots for its analysis
    analysis/*.json          SuperSpider's analyses; gods.roots: entry points it can't find
    recomp/gen*/             the C translations
    recomp/gods              the game

Committed: recipe.json, build.sh, tools/unpack.py, analysis/gods.roots,
recomp/Makefile, recomp/game.c, recomp/gods.ini, this file.

Playing
-------

    ./gods                  as !Run: the passwords notice, the credits (with the
                            intro's music), the title
    ./gods --level=3        BEGIN QUEST starts at level 3 (1-4), as if its password
                            had been entered (so it opens in the shop)
    ./gods --crop           less letterboxing on a widescreen monitor: rows 0-210
                            (recomp/gods.ini)

Keys (the !ReadMe): Z left, X right, ' up, / down, Return fire. F1 pauses
(and on the title page redefines the keys), F2 turns the sound on and off,
Ctrl-Escape quits the game, and on the title page returns to the desktop
(ends the run). A game controller stands in for the RTFM joystick (Acorn's
Joystick_Read, which the game reads): the stick or D-pad moves, A fires,
Start pauses; with --joystick=off it gives the keys above.

High scores and passwords are saved to disc 2 (Bin.hiscores): here into
recomp/savedata, read back from there.

How the game is put together
----------------------------

The Amiga/ST game, converted by Richard Walker: the level files are packed
with a simple LZ scheme (&308C4 unpacks them), and the graphics are
Amiga-style planes converted as they're loaded.

!Run sets Gods$Dir, runs Initmem with no WimpSlot (a utility that shrinks
the RMA, system sprites, font cache and RAM disc to nothing and the screen
to 80K - as libroger starts out, so it isn't run) and then, with 608K,
Intro and Gods in turn. Both are AIF images packed with Acorn's squeeze;
their first word branches to the decompressor at the end, which unpacks the
image in place and returns to &8004.

- Intro (&8000-&78DF0 unpacked): the passwords notice, until a key, then
  the Krisalis/Renegade and Richard Walker credits, with music: a voice
  generator of its own ("Tracker", &74A2C) on four channels, started with
  Sound_Control. Then OS_Exit.
- Gods (&8000-&92608): the game. Its workspace is at &8080 (R12 all
  through). It reads its files as <Gods$Dir>.<name>, or adfs::0.$.!Gods.<name>
  after asking for disc 2, through one routine (&30158), and drives the
  sound itself (a device vector, Sound_Configure). It switches routines on
  and off by storing a NOP or mov pc,lr over a word of their code (nineteen
  of them), and its objects carry their handlers' addresses, copied byte
  by byte from per-level tables at &2F8F8.

What's translated (recomp/Makefile, make regen): Gods (gen/) and Intro
(genintro/), both for &8000. game.c runs Intro as the program and, at its
OS_Exit, loads Gods' unpacked image and runs it, as !Run's next line
would; Gods' OS_Exit ends the run.

Everything is translated: headless runs of each level (--level=N, random
walking, climbing, jumping and firing for 6000M instructions: the shop,
each level's first parts, deaths, game over, the high score table and the
name entry, and a new game from the title) reach no untranslated code; one
self-modified word is interpreted, as the original runs it. The later parts
of levels 2-4, their guardians and the ending weren't reached in testing;
their code is translated as far as the analysis finds it.

The entry points the analysis can't find (analysis/gods.roots, and the
handlers tools/unpack.py writes):

- the object handlers: each level's two lists of object types, records of
  flags, a handler, sizes and a second handler, which the game copies into
  each object byte by byte and calls through it (&10708) - 34 handlers,
  read from the image by tools/unpack.py;
- the returns from two dispatchers (&2D808, &2DE94) that jump through a
  handler address read from an object, leaving their caller's return
  address on the stack for the handler to pop; each has one caller;
- &1D340's jump back into a run of ten three-instruction blocks (mov pc,r9
  with r9 = &1D3BC - 12n), whose bne's differ, so they aren't identical
  words to find as a run.

What SuperSpider and libroger needed
------------------------------------

SuperSpider:

- S1: a self-modified word that code stores a return into (a constant
  mov pc,lr stored to its address) may return there, when LR is still the
  function's return address; such words are carried from round to round.
  Gods calls &B838 with a return patched in at &B91C, which would otherwise
  run on into a branch away (the password start and the shop, &25CF4 on,
  were missed).
- S1: the rule that a self-modified transfer may be rewritten into an
  instruction that falls through (FervourMode's, for words pc-relative
  stores write) also covers a word that a constant instruction which falls
  through is stored into at its address, through a register (Gods' str
  r0,[r1] with r1 = pc + 8, switching a routine on with a NOP over its mov
  pc,lr: &1E8CC). Such words are carried from round to round too.
- S1: a jump table read with a word load steps through whole words
  whatever stride the index was given (&18C48: ldr pc,[r8,r2] after mov
  r2,r2,lsl #2 ; and r2,r2,#&ff left a stride of 1, and the scan stopped
  at the second, misaligned entry).
- gen: a computed add pc,pc,X with X = a mov'd constant times something
  (mov r7,#100 ; mul r6,r7,r6 ; add pc,pc,r6: &57760) enters its unrolled
  body at a + 8 and each multiple of the constant after, as well as at the
  blocks aligned to the end of the repeated run; the repetition runs on 18
  words past the last block, so the end-aligned entries alone were wrong.

libroger: a Sound_Control (or OS_Word 7) amplitude below -15 starts the
sound, with the amplitude's low four bits, rather than being ignored: the
intro starts its music with -21. How RISC OS treats these isn't checked
against its sources; the intro is silent otherwise.

Regression: in every other game, make regen, the generated C compared
with the C before the changes, and headless runs (60M instructions: the
event log, VDU stream, screen, registers and --wav output) compared with
runs of the binaries from before. The generated C is the same in all but
Stunt Racer 2000, where the self-modified fall-through rule finds 56 lines'
worth more of its core (&13BA8, &13BF8: words it stores mov r0,r0 into);
the headless runs are byte-identical in all 21.

Not supported: the serial port joystick interface (!SerialPor's module); a
game controller stands in for the RTFM one.
