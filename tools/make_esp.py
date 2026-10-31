#!/usr/bin/env python3
"""Gera o NahIDWin.esp (e o NahIDWin.seq) sem precisar do Creation Kit.

Conteudo do ESP (mestre: Skyrim.esm):
  0x800  NahIDWin_StaggerEffect   efeito magico do tipo Stagger (os bytes de DATA vem do mod SGC Stagger on hit, que funciona na LE)
  0x801  NahIDWin_Stagger1        feitico de stagger fraco   (magnitude 0.30)
  0x802  NahIDWin_Stagger2        feitico de stagger medio   (magnitude 0.60)
  0x803  NahIDWin_Stagger3        feitico de stagger forte   (magnitude 1.00)
  0x804  NahIDWin_BridgeQuest     quest "Start Game Enabled" com o script NahIDWinBridge
  0x805  NahIDWin_MCMQuest
  0x810  EXPL faiscas do parry; 0x811..0x815 SOUN (escudo, punhos, adaga, contundente, geral); 0x821..0x825 SNDR (copiados do Inpa Sekiro Combat LE)        quest "Start Game Enabled" com o script NahIDWinMCM (menu SkyUI)

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

ID_MGEF, ID_S1, ID_S2, ID_S3, ID_QUST, ID_QUST2 = (OWN | 0x800, OWN | 0x801, OWN | 0x802, OWN | 0x803, OWN | 0x804, OWN | 0x805)

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

script2 = b'NahIDWinMCM'
vmad2 = struct.pack('<hhH', 5, 2, 1) + struct.pack('<H', len(script2)) + script2 + b'\x00' + struct.pack('<H', 0)
qust2 = rec('QUST', ID_QUST2, [
    sub('EDID', z('NahIDWin_MCMQuest')),
    sub('VMAD', vmad2),
    sub('DNAM', bytes.fromhex('110100300000000000000000')),   # Start Game Enabled
    sub('NEXT'),
    sub('ANAM', struct.pack('<I', 0)),
])

# Registos copiados do Inpa Sekiro Combat LE (efeitos do parry), com novos FormIDs. Assets (nif/wav) sao do mesmo mod.
EXPL_PARRY = bytes.fromhex('4558504cca0000000000000010080001000000002b000000454449441f007a7a7a5f4d4f44537061726b734578706c6f73696f6e5f73756363657373004f424e440c0000000000000000000000000046554c4c1d007a7a7a53656b69726f5f737061726b735f6578705f73756363657373004d4f444c1e00696e70615f73656b69726f5c537061726b735f7375636573732e6e6966004d4f44540c000200000000000000000000004441544134000000000000000000000000000000000000000000000000000000803f0000000000000c43000000000000c8424100000001000000')
SOUN_PARRY = [
    bytes.fromhex('534f554e410000000000000011080001000000002b000000454449441f007a7a7a53656b69726f5f7368645f70617272795f737563636573735f6d6b004f424e440c0000000000000000000000000053445343040021080001'),
    bytes.fromhex('534f554e420000000000000012080001000000002b0000004544494420007a7a7a53656b69726f5f666973745f70617272795f737563636573735f6d6b004f424e440c0000000000000000000000000053445343040022080001'),
    bytes.fromhex('534f554e390000000000000013080001000000002b0000004544494417007a7a7a53656b69726f5f6461675f70617272795f6d6b004f424e440c0000000000000000000000000053445343040023080001'),
    bytes.fromhex('534f554e430000000000000014080001000000002b0000004544494421007a7a7a53656b69726f5f626c756e745f70617272795f737563636573735f6d6b004f424e440c0000000000000000000000000053445343040024080001'),
    bytes.fromhex('534f554e350000000000000015080001000000002b0000004544494413007a7a7a53656b69726f5f70617272795f6d6b004f424e440c0000000000000000000000000053445343040025080001'),
]
SNDR_PARRY = [
    bytes.fromhex('534e4452c60000000000000021080001000000002b0000004544494420007a7a7a53656b69726f5f7368645f70617272795f737563636573735f73667800434e414d04000a54ef1e474e414d0400a1720100414e414d3000646174615c536f756e645c66785c696e70615f73656b69726f5c5368645f506572666563745061727279312e77617600414e414d3000646174615c536f756e645c66785c696e70615f73656b69726f5c5368645f506572666563745061727279322e776176004f4e414d04004b320e004c4e414d040000000000424e414d0600000a01029502'),
    bytes.fromhex('534e4452920000000000000022080001000000002b0000004544494421007a7a7a53656b69726f5f666973745f70617272795f737563636573735f73667800434e414d04000a54ef1e474e414d0400a1720100414e414d3100646174615c536f756e645c66785c696e70615f73656b69726f5c666973745f506572666563745061727279312e776176004f4e414d04004b320e004c4e414d040000000000424e414d0600000a01021d01'),
    bytes.fromhex('534e4452df0000000000000023080001000000002b0000004544494418007a7a7a53656b69726f5f6461675f70617272795f73667800434e414d04000a54ef1e474e414d0400a1720100414e414d2900646174615c536f756e645c66785c696e70615f73656b69726f5c6461675f5061727279312e77617600414e414d2900646174615c536f756e645c66785c696e70615f73656b69726f5c6461675f5061727279322e77617600414e414d2900646174615c536f756e645c66785c696e70615f73656b69726f5c6461675f5061727279332e776176004f4e414d04004b320e004c4e414d040000000000424e414d0600000a01027503'),
    bytes.fromhex('534e4452cc0000000000000024080001000000002b0000004544494422007a7a7a53656b69726f5f626c756e745f70617272795f737563636573735f73667800434e414d04000a54ef1e474e414d0400a1720100414e414d3200646174615c536f756e645c66785c696e70615f73656b69726f5c626c756e745f506572666563745061727279312e77617600414e414d3200646174615c536f756e645c66785c696e70615f73656b69726f5c626c756e745f506572666563745061727279322e776176004f4e414d04004b320e004c4e414d040000000000424e414d0600000a01022d02'),
    bytes.fromhex('534e44521e0100000000000025080001000000002b000000454449441c007a7a7a53656b69726f5f70617272795f737563636573735f73667800434e414d04000a54ef1e474e414d0400a1720100414e414d2c00646174615c536f756e645c66785c696e70615f73656b69726f5c506572666563745061727279312e77617600414e414d2c00646174615c536f756e645c66785c696e70615f73656b69726f5c506572666563745061727279322e77617600414e414d2c00646174615c536f756e645c66785c696e70615f73656b69726f5c506572666563745061727279332e77617600414e414d2c00646174615c536f756e645c66785c696e70615f73656b69726f5c506572666563745061727279342e776176004f4e414d04004b320e004c4e414d040000000000424e414d0600000a01021103'),
]

n_records = 1 + len(spels) + 1 + 1 + len(SOUN_PARRY) + len(SNDR_PARRY)
n_total = n_records + 6          # + os 6 grupos (o jogo conta grupos)
tes4 = rec('TES4', 0, [
    sub('HEDR', struct.pack('<fII', 1.7, n_total, 0x900)),
    sub('CNAM', z('NahIDWin')),
    sub('MAST', z('Skyrim.esm')),
    sub('DATA', struct.pack('<Q', 0)),
    sub('INTV', struct.pack('<I', 1)),
])

esp = tes4 + grup('MGEF', [mgef]) + grup('SPEL', spels) + grup('QUST', [qust]) + grup('EXPL', [EXPL_PARRY]) + grup('SOUN', SOUN_PARRY) + grup('SNDR', SNDR_PARRY)

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'dist', 'Data')
os.makedirs(os.path.join(root, 'SEQ'), exist_ok=True)
with open(os.path.join(root, 'NahIDWin.esp'), 'wb') as f:
    f.write(esp)
with open(os.path.join(root, 'SEQ', 'NahIDWin.seq'), 'wb') as f:
    f.write(struct.pack('<I', ID_QUST))
print('NahIDWin.esp: %d bytes, %d records + 3 groups; NahIDWin.seq: quest %08X' % (len(esp), n_records, ID_QUST))
