#!/usr/bin/env python3
"""The two programs, unsqueezed by their own decompressors.

    unpack.py           game/!Gods/Intro -> unpacked/intro
                        game/!Gods/Gods  -> unpacked/gods, and its object
                        handlers -> unpacked/gods.roots

!Run runs Intro and then Gods, both absolutes at &8000. Each is an AIF
image packed by Acorn's squeeze: its first word branches to the
decompressor at its end, which unpacks the image in place, puts a NOP over
that first word and returns to &8004. Here that runs under SuperSpider's
tools/arm2.py, until it returns. The unpacked AIF header then gives the
image's size (read-only + read-write; neither has zero-initialised data),
and the image is translated and run as it is then.

The object handlers: each level (1-4) has two lists of object types, its
halves' (the a and b files), at &2F8F8 + 8 * (level - 1) (&2F8D4 picks
them). A list is records of four words - flags, a handler, sizes, a second
handler - ending with 16 bytes of &FF. The game copies a record's words
byte by byte into each object it makes and calls the handlers through the
object (&10708: orr pc,r2,r3,lsl #24), so the analysis can't see them:
every handler of every level is a root.
"""
import os
import struct
import sys

SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))
sys.path.insert(0, os.path.join(SS, 'tools'))
import arm2  # noqa: E402

TOP = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
APP = os.path.join(TOP, 'game', '!Gods')
OUT = os.path.join(TOP, 'unpacked')
BASE, DONE, STACK = 0x8000, 0x8004, 0x7F000
AIF_NOP = 0xFB000000
OBJECT_LISTS = 0x2F8F8          # Gods: each level's two lists of object types


def find(d, name):
    """a RISC OS file in a host directory: any case, any ,load-exec suffix"""
    for f in os.listdir(d):
        if f.split(',')[0].lower() == name.lower():
            return os.path.join(d, f)
    raise FileNotFoundError(os.path.join(d, name))


def unsqueeze(name, packed):
    m = arm2.ARM(memsize=0x1000000)
    m.mem[BASE:BASE + len(packed)] = packed
    m.r[13], m.r[15] = STACK, BASE

    def swi(m, n, pc):
        sys.exit('unpack: %s calls SWI &%X at &%X while unsqueezing (not the known release?)' % (name, n, pc))
    m.swi_handler = swi
    while m.r[15] != DONE:
        m.step()
        if m.steps > 20_000_000:
            raise RuntimeError('unpack: %s: no end, pc=&%X' % (name, m.r[15]))
    head = struct.unpack_from('<9I', m.mem, BASE)
    if head[0] != AIF_NOP or head[8]:
        sys.exit('unpack: %s: not a squeezed AIF image as expected' % name)
    return bytes(m.mem[BASE:BASE + head[5] + head[6]])


def handlers(image):
    """the object handlers in the levels' lists, with where each was found"""
    word = lambda a: struct.unpack_from('<I', image, a - BASE)[0]
    found = {}
    for level in range(4):
        for half in range(2):
            rec = word(OBJECT_LISTS + 8 * level + 4 * half)
            while word(rec) != 0xFFFFFFFF:
                for field in (4, 12):
                    found.setdefault(word(rec + field), '%d%s &%X+%d' % (level + 1, 'ab'[half], rec, field))
                rec += 16
    for h in found:
        if not (BASE <= h < BASE + len(image)) or h & 3:
            sys.exit('unpack: object handler &%X (%s) outside Gods: not the known release?' % (h, found[h]))
    return found


def main():
    os.makedirs(OUT, exist_ok=True)
    for name in ('Intro', 'Gods'):
        data = unsqueeze(name, open(find(APP, name), 'rb').read())
        open(os.path.join(OUT, name.lower()), 'wb').write(data)
        print('%s: %d bytes at &%X-&%X' % (name.lower(), len(data), BASE, BASE + len(data)))
    hs = handlers(data)
    with open(os.path.join(OUT, 'gods.roots'), 'w') as f:
        f.write('# the object handlers in the levels\' lists (tools/unpack.py)\n')
        for h in sorted(hs):
            f.write('&%x\t# object handler, level %s\n' % (h, hs[h]))
    print('gods.roots: %d object handlers' % len(hs))


if __name__ == '__main__':
    main()
