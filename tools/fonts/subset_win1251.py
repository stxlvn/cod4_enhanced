"""Cut a font to the characters of Windows-1251 (the code page of the Russian CoD4's font tables) for cod4coop/fonts.

    python -I tools/fonts/subset_win1251.py <in.ttf|in.otf> <out>

Keeps every name record (the OFL licence lines among them) and all layout features. Some h1-mod / h2-mod font files carry a
cmap format 4 subtable whose last segments point past its glyph array (CJK ranges in defaultBold.otf); fontTools refuses to
read such a table, so those segments are first mapped to glyph 0 in a copy of the bytes (the full format 12 subtable stays).
Needs fontTools (pip install fonttools).
"""
import io
import struct
import sys

from fontTools import subset
from fontTools.ttLib import TTFont


def win1251_codepoints():
    cps = set(range(0x20, 0x7F))
    for b in range(0x80, 0x100):
        try:
            cps.add(ord(bytes([b]).decode("cp1251")))
        except UnicodeDecodeError:
            pass
    return sorted(cps)


def fix_format4(data):
    d = bytearray(data)
    ntab = struct.unpack_from(">H", d, 4)[0]
    cmap_off = None
    for i in range(ntab):
        tag, _cs, off, _ln = struct.unpack_from(">4sIII", d, 12 + 16 * i)
        if tag == b"cmap":
            cmap_off = off
    if cmap_off is None:
        return bytes(d), 0
    _ver, nsub = struct.unpack_from(">HH", d, cmap_off)
    fixed = 0
    seen = set()
    for i in range(nsub):
        _pid, _eid, so = struct.unpack_from(">HHI", d, cmap_off + 4 + 8 * i)
        o = cmap_off + so
        if o in seen or struct.unpack_from(">H", d, o)[0] != 4:
            continue
        seen.add(o)
        length = struct.unpack_from(">H", d, o + 2)[0]
        segx2 = struct.unpack_from(">H", d, o + 6)[0]
        ends_o = o + 14
        starts_o = ends_o + segx2 + 2
        delta_o = starts_o + segx2
        range_o = delta_o + segx2
        for s in range(segx2 // 2):
            end = struct.unpack_from(">H", d, ends_o + 2 * s)[0]
            start = struct.unpack_from(">H", d, starts_o + 2 * s)[0]
            ro = struct.unpack_from(">H", d, range_o + 2 * s)[0]
            if ro and range_o + 2 * s + ro + 2 * (end - start) + 2 > o + length:
                struct.pack_into(">H", d, range_o + 2 * s, 0)
                struct.pack_into(">H", d, delta_o + 2 * s, (-start) % 0x10000)  # start + delta = 0 (glyph 0)
                fixed += 1
    return bytes(d), fixed


def main(src, dst):
    data, fixed = fix_format4(open(src, "rb").read())
    font = TTFont(io.BytesIO(data))
    options = subset.Options()
    options.name_IDs = ["*"]
    options.name_languages = ["*"]
    options.layout_features = ["*"]
    options.notdef_outline = True
    sub = subset.Subsetter(options)
    sub.populate(unicodes=win1251_codepoints())
    sub.subset(font)
    font.save(dst)
    print("%s -> %s (%d cmap segments repaired)" % (src, dst, fixed))


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
