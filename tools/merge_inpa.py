#!/usr/bin/env python3
"""Funde o ESP do Inpa Sekiro Combat LE com os registos proprios do NahIDWin num unico NahIDWin.esp.

USO (uso privado - o ESP/assets do Inpa NAO vao para o repositorio publico):
    python3 tools/merge_inpa.py <inpa_le.esp> <pasta_saida_Data>

O que faz:
  * remove o mestre VioLens.esp (renumera o indice proprio 5 -> 4) e descarta os 10 registos do VioLens
    (execucoes/killmove) e as 2 sobreposicoes de IDLE que apontavam para eles;
  * acrescenta os nossos MGEF/SPEL/QUST (staggers + ponte + MCM; FormIDs 0x800..0x805);
  * poe os globais de poise/postura do Inpa a 0 (quem gere Poise/Postura e o NahIDWin);
  * escreve NahIDWin.seq com todas as quests "Start Game Enabled" e o ficheiro NahIDWin_DISTR.ini (SPID).
"""
import os, re, struct, sys

SRC, OUT = sys.argv[1], sys.argv[2]
d = open(SRC, 'rb').read()
HERE = os.path.dirname(os.path.abspath(__file__))

# ---------- registos proprios (reutiliza make_esp.py com indice 4) ----------
mk = open(os.path.join(HERE, 'make_esp.py')).read().split('n_records =')[0]
mk = mk.replace('OWN = 0x01000000', 'OWN = 0x04000000')
mk = re.sub(r"EXPL_PARRY = .*?\n(?=\n|n_records)", '', mk, flags=re.S) if False else mk
ns = {'__file__': os.path.join(HERE, 'make_esp.py')}
exec(compile(mk, 'make_esp', 'exec'), ns)
OWN_MGEF, OWN_SPELS, OWN_QUSTS = ns['mgef'], ns['spels'], [ns['qust'], ns['qust2']]

# ---------- ler o ESP do Inpa ----------
tes4_sz = struct.unpack('<I', d[4:8])[0]
tes4_flags = struct.unpack('<I', d[8:12])[0]
tes4_body = d[24:24 + tes4_sz]
off = 24 + tes4_sz
groups = []          # (label, [registos em bytes])
while off < len(d):
    assert d[off:off + 4] == b'GRUP'
    sz = struct.unpack('<I', d[off + 4:off + 8])[0]
    label = d[off + 8:off + 12]
    p, rl = off + 24, []
    while p < off + sz:
        assert d[p:p + 4] != b'GRUP', 'grupo aninhado inesperado'
        rs = struct.unpack('<I', d[p + 4:p + 8])[0]
        assert not (struct.unpack('<I', d[p + 8:p + 12])[0] & 0x40000), 'registo comprimido inesperado'
        rl.append(d[p:p + 24 + rs]); p += 24 + rs
    groups.append((label, rl)); off += sz

def fid_of(r): return struct.unpack('<I', r[12:16])[0]
def subrecs(r):
    body, o, out = r[24:], 0, []
    while o < len(body):
        t = body[o:o + 4]; l = struct.unpack('<H', body[o + 4:o + 6])[0]; out.append((t, body[o + 6:o + 6 + l])); o += 6 + l
    return out

# ---------- remover o VioLens (indice 4) ----------
vl = {fid_of(r) for _, rl in groups for r in rl if fid_of(r) >> 24 == 4}
def refs_vl(r):
    for t, v in subrecs(r):
        for i in range(len(v) - 3):
            if struct.unpack('<I', v[i:i + 4])[0] in vl: return True
    return False
drop = set()
for _, rl in groups:
    for r in rl:
        if fid_of(r) in vl or refs_vl(r): drop.add(fid_of(r))
print('registos removidos (VioLens + dependentes):', len(drop), ['%08X' % x for x in sorted(drop)])
groups = [(l, [r for r in rl if fid_of(r) not in drop]) for l, rl in groups]

# ---------- renumerar indice proprio 5 -> 4 ----------
own = {fid_of(r) for _, rl in groups for r in rl if fid_of(r) >> 24 == 5}
def renum(v):
    b = bytearray(v)
    for i in range(len(b) - 3):
        x = struct.unpack_from('<I', b, i)[0]
        if x in own: struct.pack_into('<I', b, i, (x & 0xFFFFFF) | 0x04000000)
    return bytes(b)
def fix(r):
    fid = fid_of(r)
    if fid in own: fid = (fid & 0xFFFFFF) | 0x04000000
    body = b''.join(t + struct.pack('<H', len(v)) + renum(v) for t, v in subrecs(r))
    assert len(body) == len(r) - 24
    return r[:12] + struct.pack('<I', fid) + r[16:24] + body
groups = [(l, [fix(r) for r in rl]) for l, rl in groups]

# aviso: referencias a 05xxxxxx que nao sao registos nossos (nao deviam existir)
stale = 0
for _, rl in groups:
    for r in rl:
        for t, v in subrecs(r):
            if t in (b'EDID', b'MODL', b'FULL', b'DESC', b'MODT', b'FNAM'): continue
            for i in range(0, len(v) - 3, 4):
                if v[i + 3] == 5 and struct.unpack('<I', v[i:i + 4])[0] >> 24 == 5: stale += 1
print('referencias 05xxxxxx por converter (esperado 0):', stale)

# ---------- nome do menu do Inpa no SkyUI ----------
OLD = b'\x12\x00Inpa Sekiro Combat'
NEWNAME = b'NAH-I-D-WIN - Inpa (Parry/Block)'
def rename_menu(r):
    if fid_of(r) != 0x041D0F75: return r
    s = subrecs(r)
    body = b''
    for t, v in s:
        if t == b'VMAD':
            assert v.count(OLD) == 1
            v = v.replace(OLD, struct.pack('<H', len(NEWNAME)) + NEWNAME)
        body += t + struct.pack('<H', len(v)) + v
    print('  menu do Inpa renomeado para', NEWNAME.decode())
    return r[:4] + struct.pack('<I', len(body)) + r[8:24] + body
groups = [(l, [rename_menu(r) for r in rl] if l == b'QUST' else rl) for l, rl in groups]

# ---------- globais por omissao ----------
SETS = {b'zzzSekiro_poisesystem': 0.0, b'zzzSekiro_posture_dmg_mult': 0.0, b'zzzSekiro_posture_dmg_mult_player': 0.0}
def setglob(r):
    s = subrecs(r)
    e = [v for t, v in s if t == b'EDID'][0].rstrip(b'\0')
    if e not in SETS: return r
    body = b''.join(t + struct.pack('<H', len(v)) + (struct.pack('<f', SETS[e]) if t == b'FLTV' else v) for t, v in s)
    print('  global %s -> %s' % (e.decode(), SETS[e]))
    return r[:4] + struct.pack('<I', len(body)) + r[8:24] + body
groups = [(l, [setglob(r) for r in rl] if l == b'GLOB' else rl) for l, rl in groups]

# ---------- juntar os nossos registos ----------
add = {b'MGEF': [OWN_MGEF], b'SPEL': OWN_SPELS, b'QUST': OWN_QUSTS}
groups = [(l, rl + add.get(l, [])) for l, rl in groups]

# quests Start Game Enabled
quests = []
for l, rl in groups:
    if l == b'QUST':
        for r in rl:
            dn = [v for t, v in subrecs(r) if t == b'DNAM']
            if dn and struct.unpack('<H', dn[0][:2])[0] & 1: quests.append(fid_of(r))
print('quests start-game-enabled:', ['%08X' % q for q in quests])

# ---------- cabecalho ----------
def sub(t, v=b''): return t + struct.pack('<H', len(v)) + v
hedr = [v for t, v in subrecs(d[:24 + tes4_sz]) if t == b'HEDR'][0]
nrec = sum(len(rl) for _, rl in groups) + len(groups)
maxid = max(fid_of(r) & 0xFFFFFF for _, rl in groups for r in rl if fid_of(r) >> 24 == 4)
tes4_new = (sub(b'HEDR', struct.pack('<fII', 1.7, nrec, maxid + 1)) + sub(b'CNAM', b'NahIDWin\0') +
            sub(b'SNAM', b'NahIDWin: Poise/Postura + Inpa Sekiro Combat (fundido)\0'))
for m in (b'Skyrim.esm', b'Update.esm', b'Dawnguard.esm', b'Dragonborn.esm'):
    tes4_new += sub(b'MAST', m + b'\0') + sub(b'DATA', b'\0' * 8)
tes4_new += sub(b'INTV', struct.pack('<I', 1))
tes4 = d[:4] + struct.pack('<I', len(tes4_new)) + struct.pack('<I', tes4_flags) + d[12:24] + tes4_new

out = tes4
for l, rl in groups:
    body = b''.join(rl)
    out += b'GRUP' + struct.pack('<I', len(body) + 24) + l + struct.pack('<I', 0) + struct.pack('<HH', 0, 0) + struct.pack('<HH', 0, 0) + body

os.makedirs(os.path.join(OUT, 'SEQ'), exist_ok=True)
open(os.path.join(OUT, 'NahIDWin.esp'), 'wb').write(out)
open(os.path.join(OUT, 'SEQ', 'NahIDWin.seq'), 'wb').write(b''.join(struct.pack('<I', q) for q in sorted(quests)))
open(os.path.join(OUT, 'NahIDWin_DISTR.ini'), 'w').write(
    ';NahIDWin (Inpa fundido) - distribuicao SPID do perk de bloqueio dos NPCs\n'
    ';Requer o Spell Perk Item Distributor. (Original do Inpa: 0x00E92761)\n'
    'Perk = 0x00E92761 - NahIDWin.esp | NONE | NONE | NONE | NONE | NONE | 100\n')
print('NahIDWin.esp: %d bytes, %d registos, %d grupos' % (len(out), nrec - len(groups), len(groups)))
