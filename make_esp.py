#!/usr/bin/env python3
"""Gera o NahIDWin.esp (e o NahIDWin.seq) sem precisar do Creation Kit.

Conteudo do ESP (mestre: Skyrim.esm):
  0x800  NahIDWin_StaggerEffect   efeito magico do tipo Stagger (os bytes de DATA vem do mod SGC Stagger on hit, que funciona na LE)
  0x801  NahIDWin_Stagger1        feitico de stagger fraco   (magnitude 0.30)
  0x802  NahIDWin_Stagger2        feitico de stagger medio   (magnitude 0.60)
  0x803  NahIDWin_Stagger3        feitico de stagger forte   (magnitude 1.00)
  0x804  NahIDWin_BridgeQuest     quest "Start Game Enabled" com o script NahIDWinBridge

Uso:  python3 tools/make_esp.py        (escreve em dist/Data/)
Para mudar a forca dos staggers, edite MAGNITUDES e gere de novo.
"""
import os, struct

MAGNITUDES = (0.30, 0.60, 1.00)
OWN = 0x01000000            # indice do proprio plugin (1 mestre: Skyrim.esm)
FORM_VERSION = 43

def sub(tag, data=b''):
    return tag.encode() + struct.pack('<H', len(data)) + data

def z(s):
    return s.encode('latin1') + b'\x00'

def rec(tag, form_id, subs, flags=0):
    body = b''.join(subs)
    return (tag.encode() + struct.pack('<I', len(body)) + struct.pack('<I', flags) +
            struct.pack('<I', form_id) + struct.pack('<I', 0) + struct.pack('<HH', FORM_VERSION, 0) + body)

def grup(label, recs):
    body = b''.join(recs)
    return (b'GRUP' + struct.pack('<I', len(body) + 24) + label.encode() + struct.pack('<I', 0) +
            struct.pack('<HH', 0, 0) + struct.pack('<HH', 0, 0) + body)

# DATA do efeito magico: copiado do SGC (arquetipo 0x21 = Stagger, lancamento Toque, Dispara e Esquece)
MGEF_DATA = bytes.fromhex(
    '018000006f12833a00000000ffffffffffffffff00002a46000000000000803f'
    '000000000000000000000000000000000000000000000000000000000000000021000000ffffffff'
    '00000000000000000100000001000000ffffffff0000000000000000000000000000000000000000'
    '0000803f000000000000000000000000000000000000000000000000000000000000000000000000')
assert len(MGEF_DATA) == 152

ID_MGEF, ID_S1, ID_S2, ID_S3, ID_QUST = (OWN | 0x800, OWN | 0x801, OWN | 0x802, OWN | 0x803, OWN | 0x804)

mgef = rec('MGEF', ID_MGEF, [
    sub('EDID', z('NahIDWin_StaggerEffect')),
    sub('FULL', z('NahIDWin Stagger effect')),
    sub('MDOB', bytes.fromhex('a5350400')),
    sub('DATA', MGEF_DATA),
    sub('DNAM', b'\x00'),
])

def spell(form_id, edid, name, magnitude):
    spit = struct.pack('<IIIfIIffI', 0, 1, 0, 0.0, 1, 1, 0.0, 0.0, 0)     # custo 0, tipo Feitico, Dispara e Esquece, Toque
    return rec('SPEL', form_id, [
        sub('EDID', z(edid)),
        sub('OBND', b'\x00' * 12),
        sub('FULL', z(name)),
        sub('ETYP', struct.pack('<I', 0x00013F44)),
        sub('DESC', b'\x00'),
        sub('SPIT', spit),
        sub('EFID', struct.pack('<I', ID_MGEF)),
        sub('EFIT', struct.pack('<fII', magnitude, 0, 0)),
    ])

spels = [spell(ID_S1 , 'NahIDWin_Stagger1', 'NahIDWin Stagger (weak)',   MAGNITUDES[0]),
         spell(ID_S2 , 'NahIDWin_Stagger2', 'NahIDWin Stagger (medium)', MAGNITUDES[1]),
         spell(ID_S3 , 'NahIDWin_Stagger3', 'NahIDWin Stagger (strong)', MAGNITUDES[2])]

script = b'NahIDWinBridge'
vmad = struct.pack('<hhH', 5, 2, 1) + struct.pack('<H', len(script)) + script + b'\x00' + struct.pack('<H', 0)
qust = rec('QUST', ID_QUST, [
    sub('EDID', z('NahIDWin_BridgeQuest')),
    sub('VMAD', vmad),
    sub('DNAM', bytes.fromhex('110100300000000000000000')),   # Start Game Enabled (como no SGC)
    sub('NEXT'),
    sub('ANAM', struct.pack('<I', 0)),
])

n_records = 1 + len(spels) + 1
n_total = n_records + 3          # + os 3 grupos (o jogo conta grupos)
tes4 = rec('TES4', 0, [
    sub('HEDR', struct.pack('<fII', 1.7, n_total, 0x805)),
    sub('CNAM', z('NahIDWin')),
    sub('MAST', z('Skyrim.esm')),
    sub('DATA', struct.pack('<Q', 0)),
    sub('INTV', struct.pack('<I', 1)),
])

esp = tes4 + grup('MGEF', [mgef]) + grup('SPEL', spels) + grup('QUST', [qust])

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'dist', 'Data')
os.makedirs(os.path.join(root, 'SEQ'), exist_ok=True)
with open(os.path.join(root, 'NahIDWin.esp'), 'wb') as f:
    f.write(esp)
with open(os.path.join(root, 'SEQ', 'NahIDWin.seq'), 'wb') as f:
    f.write(struct.pack('<I', ID_QUST))
print('NahIDWin.esp: %d bytes, %d records + 3 groups; NahIDWin.seq: quest %08X' % (len(esp), n_records, ID_QUST))
