#!/usr/bin/env python3
"""Acrescenta ao NahIDWin.esp privado (que ja tem o fogo, add_fire.py) a MARCA da animacao do Supremo Rage:
  0x04000846 MGEF NahIDWin_SupAnim_mgef   (invisivel; so serve de condicao ao Dynamic Animation Replacer)
  0x04000847 SPEL NahIDWin_SupAnim        (dura 3 s; e lancado no jogador ao ativar o Supremo Rage)
Uso: python3 tools/add_animmark.py NahIDWin.esp_entrada NahIDWin.esp_saida
"""
import struct, sys
src, dst = sys.argv[1], sys.argv[2]
d = open(src, 'rb').read()
FORM_VERSION = 43
def sub(t, v=b''): return t + struct.pack('<H', len(v)) + v
def z(s): return s.encode('latin1') + b'\0'
def rec(tag, fid, subs):
    body = b''.join(subs)
    return tag + struct.pack('<I', len(body)) + struct.pack('<I', 0) + struct.pack('<I', fid) + struct.pack('<I', 0) + struct.pack('<HH', FORM_VERSION, 0) + body
# registo TES4
tsz = struct.unpack('<I', d[4:8])[0]
tes4 = d[:24 + tsz]
off = 24 + tsz
groups = []
while off < len(d):
    assert d[off:off+4] == b'GRUP'
    sz = struct.unpack('<I', d[off+4:off+8])[0]
    groups.append([d[off+8:off+12], d[off+24:off+sz]])
    off += sz
def recs(body):
    o = 0; out = []
    while o < len(body):
        n = struct.unpack('<I', body[o+4:o+8])[0]
        out.append(body[o:o+24+n]); o += 24 + n
    return out
def fid(r): return struct.unpack('<I', r[12:16])[0]
def subs(r):
    b = r[24:]; o = 0; out = []
    while o < len(b):
        t = b[o:o+4]; l = struct.unpack('<H', b[o+4:o+6])[0]; o += 6
        out.append((t, b[o:o+l])); o += l
    return out
tmpl = None
for lab, body in groups:
    if lab == b'MGEF':
        for r in recs(body):
            if fid(r) == 0x04000840: tmpl = [v for t, v in subs(r) if t == b'DATA'][0]
assert tmpl and len(tmpl) == 152, 'MGEF do brilho (0x04000840) nao encontrado: usa o pacote privado v0.10.x'
w = list(struct.unpack('<38I', tmpl))
w[6] = 0; w[8] = 0; w[23] = 0; w[24] = 0
mgef = rec(b'MGEF', 0x04000846, [sub(b'EDID', z('NahIDWin_SupAnim_mgef')), sub(b'FULL', z('NahIDWin supreme anim')), sub(b'DATA', struct.pack('<38I', *w)), sub(b'DNAM', b'\0')])
spit = struct.pack('<IIIfIIffI', 0, 0, 0, 0.0, 1, 1, 0.0, 0.0, 0)
spel = rec(b'SPEL', 0x04000847, [sub(b'EDID', z('NahIDWin_SupAnim')), sub(b'OBND', b'\0' * 12), sub(b'FULL', z('NahIDWin supreme anim')),
    sub(b'ETYP', struct.pack('<I', 0x00013F44)), sub(b'DESC', b'\0'), sub(b'SPIT', spit),
    sub(b'EFID', struct.pack('<I', 0x04000846)), sub(b'EFIT', struct.pack('<fII', 0.0, 0, 3))])
used = {fid(r) for lab, body in groups for r in recs(body)}
assert 0x04000846 not in used and 0x04000847 not in used
for g in groups:
    if g[0] == b'MGEF': g[1] += mgef
    if g[0] == b'SPEL': g[1] += spel
# cabecalho: +2 registos
ts = subs(tes4)
newt = b''
for t, v in ts:
    if t == b'HEDR':
        ver, n, nxt = struct.unpack('<fII', v)
        v = struct.pack('<fII', ver, n + 2, max(nxt, 0x848))
    newt += sub(t, v)
tes4 = tes4[:4] + struct.pack('<I', len(newt)) + tes4[8:24] + newt
out = tes4
for lab, body in groups:
    out += b'GRUP' + struct.pack('<I', len(body) + 24) + lab + struct.pack('<I', 0) + struct.pack('<HH', 0, 0) + struct.pack('<HH', 0, 0) + body
open(dst, 'wb').write(out)
print('ok: %d -> %d bytes (+MGEF 0x846, +SPEL 0x847)' % (len(d), len(out)))
