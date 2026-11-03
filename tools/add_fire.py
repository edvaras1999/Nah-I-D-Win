#!/usr/bin/env python3
"""Acrescenta ao NahIDWin.esp privado (fundido com o Inpa) o efeito de FOGO do Supremo Rage:
  0x04000844 MGEF NahIDWin_SupFire_mgef   (luz, shader, arte de lancamento e de impacto do jogo base; so referencias a formas do Skyrim.esm)
  0x04000845 SPEL NahIDWin_SupFire
As referencias (luz 0x140EC, shader 0x2ACD8, arte 0x36342 e 0x2ACD7) sao as do manto de fogo usado pelo DDDaggerWeaponArt, todas do Skyrim.esm.
Uso: python3 tools/add_fire.py NahIDWin.esp_entrada NahIDWin.esp_saida
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
w[6] = 0x000140EC; w[8] = 0x0002ACD8; w[23] = 0x00036342; w[24] = 0x0002ACD7
mgef = rec(b'MGEF', 0x04000844, [sub(b'EDID', z('NahIDWin_SupFire_mgef')), sub(b'FULL', z('NahIDWin supreme fire')), sub(b'DATA', struct.pack('<38I', *w)), sub(b'DNAM', b'\0')])
spit = struct.pack('<IIIfIIffI', 0, 0, 0, 0.0, 1, 1, 0.0, 0.0, 0)
spel = rec(b'SPEL', 0x04000845, [sub(b'EDID', z('NahIDWin_SupFire')), sub(b'OBND', b'\0' * 12), sub(b'FULL', z('NahIDWin supreme fire')),
    sub(b'ETYP', struct.pack('<I', 0x00013F44)), sub(b'DESC', b'\0'), sub(b'SPIT', spit),
    sub(b'EFID', struct.pack('<I', 0x04000844)), sub(b'EFIT', struct.pack('<fII', 0.0, 0, 30))])
used = {fid(r) for lab, body in groups for r in recs(body)}
assert 0x04000844 not in used and 0x04000845 not in used
for g in groups:
    if g[0] == b'MGEF': g[1] += mgef
    if g[0] == b'SPEL': g[1] += spel
# cabecalho: +2 registos
ts = subs(tes4)
newt = b''
for t, v in ts:
    if t == b'HEDR':
        ver, n, nxt = struct.unpack('<fII', v)
        v = struct.pack('<fII', ver, n + 2, max(nxt, 0x846))
    newt += sub(t, v)
tes4 = tes4[:4] + struct.pack('<I', len(newt)) + tes4[8:24] + newt
out = tes4
for lab, body in groups:
    out += b'GRUP' + struct.pack('<I', len(body) + 24) + lab + struct.pack('<I', 0) + struct.pack('<HH', 0, 0) + struct.pack('<HH', 0, 0) + body
open(dst, 'wb').write(out)
print('ok: %d -> %d bytes (+MGEF 0x844, +SPEL 0x845)' % (len(d), len(out)))
