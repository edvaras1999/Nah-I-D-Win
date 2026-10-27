# NAH-I-D-WIN (NahIDWin)

**Poise + Postura** para Skyrim **Legendary Edition** (1.9.32.0 + SKSE 1.7.3), separados da stamina.

- **Poise** (barra rapida): cada golpe tira poise. Sem quebrar: stagger *leve* (golpe normal) ou *medio* (golpe forte / escudo). Quebrou: stagger *pesado*.
- **Postura** (barra lenta, estilo Sekiro): cada golpe enche-a, ela baixa sozinha. Encheu por completo: **BleedOut** (ajoelhado e indefeso).
- Golpes bloqueados enchem as duas barras so um pouco e nao dao stagger. A dificuldade da postura esta no `.ini` (`[Posture] Difficulty`).

Estado: **v0.3.0** (ainda nao testado no jogo). Novo: estamina por balanco (jogador) e por golpe (NPCs), empurrao no stagger pesado em condicoes especificas, barra de risco no TrueHUD. Ainda por fazer: bloqueio perfeito (Inpa Sekiro), dano extra durante o BleedOut, MCM, segunda barra propria.

## Como compilar (tudo no GitHub)
1. Cria um repositorio novo e envia estes arquivos mantendo as pastas.
2. Aba **Actions** > "Build NahIDWin" > **Run workflow** (tambem corre sozinho a cada envio).
3. No fim, baixa o artefacto **NahIDWin-instalacao**: e um zip pronto para instalar (MO2).
   Se algum passo falhar, baixa tambem os artefactos `dll` e `papyrus` (tem os logs) e envia-os.

## Instalar para testar
Instala o zip pelo MO2 e ativa **NahIDWin.esp**. Precisa de SKSE 1.7.3. Para ver barras: TrueHUD LE com as "special bars" ligadas.
Configuracao: `Data/SKSE/Plugins/NahIDWin.ini` (relido sozinho). Log: `Documentos/My Games/Skyrim/SKSE/NahIDWin.log`.

## Como funciona o stagger
- `StaggerMode=0` (por omissao): a DLL faz as contas e, **quando e preciso reagir (qualquer stagger ou BleedOut)**, envia um evento de mod.
  O script `NahIDWinBridge` (poucas linhas) recebe-o e lanca um de tres feiticos de stagger do jogo (leve/medio/pesado); no BleedOut toca a animacao de ajoelhado durante `BleedOutSeconds`.
- `StaggerMode=1` (experimental): sem Papyrus. A DLL tenta tocar a animacao de stagger. Serve para o diagnostico.

## Diagnostico
O log traz: cada golpe com as contas, o resumo a cada 30 s (eventos enviados, confirmados pelo script, atraso medio e maximo),
avisos se o script nao responde, e um despejo das funcoes de animacao do jogador. Envia o log depois de uma luta.

## Licenca
GPL-3.0 (ver `nahidwin/LICENSE`). A base de compilacao e os enderecos vem do FreeClimb LE / SoftTarget LE. Os registos do efeito de stagger
seguem o mod "SGC Stagger on hit". Referencias: doodlum "Poise" (SE) e a API do TrueHUD LE (Mhacks79, baseada no TrueHUD de Ersh).
