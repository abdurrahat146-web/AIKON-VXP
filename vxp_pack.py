#!/usr/bin/env python3
"""
Own implementation of the MediaTek MRE .vxp packaging step: fixes the
resource-table offsets the compiler leaves relative, then appends the
app's tag table (name, version, IMSI, permissions, etc.) after the ELF
image. Tag format: https://lpcwiki.miraheze.org/wiki/MediaTek_MRE/Tag_format
Written independently from that public spec (not copied from any
third-party build script) so it's code I can vouch for running.
"""
import sys, struct, json

def main():
    axf_path, out_path, manifest_path = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(axf_path, 'rb') as f:
        axf = bytearray(f.read())
    with open(manifest_path, 'r') as f:
        cfg = json.loads(f.read())

    def set_u32(pos, value):
        axf[pos:pos+4] = struct.pack('<I', value & 0xFFFFFFFF)
    def get_u32(pos):
        return struct.unpack_from('<I', axf, pos)[0]

    marker = bytes([0x41,0x70,0x70,0x4C,0x6F,0x67,0x6F,0x2E,0x69,0x6D,0x67,
                     0x00,0x30,0x00,0x00,0x00,0xE0])
    res_offset = axf.find(marker)
    if res_offset == -1:
        raise RuntimeError("resource marker not found in AXF")

    pos = res_offset
    while True:
        while axf[pos] != 0:
            pos += 1
        pos += 1
        if axf[pos - 2] == 0:
            break
        off = get_u32(pos)
        set_u32(pos, off + res_offset)
        pos += 8

    res2_offset = get_u32(pos) + res_offset
    set_u32(pos, res2_offset)

    pos = res2_offset
    while True:
        rid = get_u32(pos); pos += 4
        off = get_u32(pos)
        set_u32(pos, off + res_offset)
        pos += 4
        if rid == 0xFFFFFFFF:
            break

    tags = bytearray()
    def a32(v):
        tags.extend(struct.pack('<I', v & 0xFFFFFFFF))
    def int_tag(tid, v):
        a32(tid); a32(4); a32(v)
    def int_array_tag(tid, vals):
        a32(tid); a32(len(vals) * 4)
        for v in vals: a32(v)
    def str_tag(tid, s):
        enc = s.encode('utf-16-le')
        a32(tid); a32(len(enc) + 2); tags.extend(enc); tags.extend(b'\x00\x00')
    def ascii_tag(tid, s):
        enc = s.encode('ascii')
        a32(tid); a32(len(enc) + 1); tags.extend(enc); tags.append(0)
    def str_array_tag(tid, vals):
        a32(tid)
        len_pos = len(tags)
        a32(0)
        total = 0
        for i, v in enumerate(vals, start=1):
            enc = v.encode('utf-16-le')
            a32(i); a32(len(enc) + 2); tags.extend(enc); tags.extend(b'\x00\x00')
            total += len(enc) + 10
        tags[len_pos:len_pos+4] = struct.pack('<I', total)

    tags_pos = len(axf)
    str_tag(0x01, cfg["vendor"])
    str_tag(0x04, cfg["name"])
    int_tag(0x05, cfg["version"])
    int_tag(0x0f, cfg["required_ram"])
    ascii_tag(0x12, cfg["imsi"])
    str_tag(0x17, cfg["description"])
    int_tag(0x18, cfg["run_in_background"])
    str_array_tag(0x19, cfg["app_name_multilang"])

    a32(0x13); a32(0xD0)
    for perm in range(5000, 5026):
        a32(perm); a32(1)

    int_tag(0x02, 0xFFFFFFFF)
    int_tag(0x03, 1)
    int_array_tag(0x06, [0,0,0,0,0,0])
    int_array_tag(0x07, [0,0,0,0,0,0])
    int_tag(0x10, 0)
    int_tag(0x11, 8457)
    int_tag(0x16, 1)
    int_tag(0x1c, 0)
    int_tag(0x21, 6)
    int_tag(0x22, 0)
    int_tag(0x23, 1)
    int_tag(0x25, 0)
    int_tag(0x29, 0)
    int_tag(0x2a, 0)
    int_tag(0x2c, 0)
    a32(0x2d); a32(0xFFFFFFFF)
    str_tag(0x2e, "")
    int_tag(0x2f, 0)
    int_tag(0x31, 0)
    int_tag(0x32, 0)
    int_tag(0x33, 0)

    a32(0); a32(0)
    tags.extend(bytes([0xB4,0x56,0x44,0x45,0x31,0x30]))
    a32(1)
    for _ in range(16): a32(0)
    a32(tags_pos); a32(0); a32(0)

    with open(out_path, 'wb') as f:
        f.write(axf)
        f.write(tags)
    print(f"wrote {out_path}: {len(axf)} (axf) + {len(tags)} (tags) bytes")

if __name__ == "__main__":
    main()
