# NAH-I-D-WIN (NahIDWin)

**Poise + Postura** para Skyrim **Legendary Edition** (1.9.32.0 + SKSE 1.7.3), separados da stamina.

- **Poise** (barra rapida): cada golpe tira poise. Sem quebrar: stagger *leve* (golpe normal) ou *medio* (golpe forte / escudo). Quebrou: stagger *pesado*.
- **Postura** (barra lenta, estilo Sekiro): cada golpe enche-a, ela baixa sozinha. Encheu por completo: **BleedOut** (ajoelhado e indefeso).
- Golpes bloqueados enchem as duas barras so um pouco e nao dao stagger. A dificuldade da postura esta no `.ini` (`[Posture] Difficulty`).

Estado: **v0.7.0** (ainda nao testado no jogo). Novo: o parry passou a ser uma **janela de imunidade total** aberta ao carregar no bloqueio (250 ms, recarga 200 ms, so golpes de frente), decidida so pelo tempo e sem devolver vida; o bloqueio normal do jogo fica intacto e apenas gasta estamina ao atacante; no parry o atacante leva postura, stagger leve e perde estamina (sem mexer no poise). Efeitos (faiscas/sons) proprios no NahIDWin.esp: os ficheiros .nif/.wav vem do pacote `NahIDWin-efeitos-inpa.zip` (instalar a parte, nao vai para o repositorio).

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


## v0.9.0 - menu unico (herda o menu do Inpa Sekiro Combat)
- `papyrus/src/NahIDWinMCM.psc` agora `extends zzzSekiro_MCM`: as paginas "MOD Setting" e "Game Setting" (com o interruptor MOD On/Off) sao do Inpa,
  sem alteracoes; as paginas "Poise and Posture", "Block and Rage" e "Stamina, HUD, System" sao as nossas. A pagina inicial fica sem imagem.
  O interruptor MOD On/Off do Inpa tambem poe `General Enabled` no .ini.
- `papyrus/stubs/zzzSekiro_MCM.psc` e so um apoio para compilar. O script verdadeiro vem do pacote privado (Inpa).
- `tools/make_esp.py` ja nao cria quest de menu: o menu so existe com o ESP fundido (`tools/merge_inpa.py`).
- Ressurreicao desligada por omissao; cloak neutralizado; os feitiços de NPC e aliados vao por SPID (NahIDWin_DISTR.ini, obrigatorio).

## v0.9.1
- Removida a pasta `dist/Data/meshes` (25 ficheiros .hkx de 0 bytes substituíam as animações verdadeiras de bloqueio e stagger).
- Postura quebrada num NPC = `poisebreak` do Inpa (stagger grande, som, janela de execução com animação do jogo base), disparado pela NOSSA postura, sem mexer na estamina do alvo. Sem o Inpa (ou no jogador) fica a animação de ajoelhado.
- O SPID geral foi retirado: a DLL avisa o script ponte quando um ator entra em combate (`NahIDWin_Combatant`) e o script dá-lhe o feitiço do Inpa (aliado ou inimigo) uma vez. O SPID serve só para o perk de bloqueio dos NPCs.
- `PowerStart` declarado em NahIDWinNative.psc (o log da v0.9.0 avisava que faltava).

## v0.9.2
- O pacote do Actions deixou de levar NahIDWin.esp e .seq (vêm só do pacote privado), para não haver dois ESP com o mesmo nome. O .ini do repositório já vem com `[Inpa] Mode=1`.

## v0.10.0 - menu proprio (3 abas), sem herdar do Inpa
- `NahIDWinMCM.psc` e um menu novo (SKI_ConfigBase), no estilo do Inpa (cabecalhos laranja, MOD On/Off), com tres abas: MOD Setting, Game Setting e Advanced.
  As opcoes do Inpa gravam nos mesmos globais dele e aplicam os mesmos Game Settings (o Inpa reaplica-os ao carregar); as nossas gravam no .ini.
- MOD On/Off liga/desliga Inpa + NahIDWin (General Enabled).
- Removido do menu: Regenerate Stamina, Posture Damage Mult do Inpa, Ressurreicao, PowerAttack stamina limit, modos de execucao do VioLens, barras extras.
- Novo: `[NpcBlock] ChanceMin` (chance de bloqueio dos NPCs, reposta pela ponte apos o Inpa); sliders Combat Stamina Regen e Stamina Regen Delay.
- `zzzSekiro_MCM.pex` ja nao e usado nem enviado. Stub novo: GlobalVariable.

## v0.10.1
- Estamina baixa deixa de quebrar a postura do jogador: `tools/pexpatch_stamina.py` neutraliza, nos scripts do Inpa (`zzzSekiro_player_main.pex`, `zzzSekiro_player_parry.pex`), a regra "estamina <= 10 ao levar golpe -> deathblow_ME". So a nossa barra de postura quebra a postura.
- Janela de imunidade do jogador depois de um stagger do jogo (`[Stagger] PlayerImmuneAfterSeconds`, 1.5 s; 0 = desligado), via `bNoStagger`. Staggers pesados do mod passam sempre.
- Furia dos NPCs com visual: `[Rage] NPCVisual` 0 nada, 1 brilho, 2 simbolo vermelho, 3 os dois (luz e desenho do Inpa, sem o script de ataque perigoso). Novo evento `NahIDWin_Rage`.

## v0.10.2
- Visual da furia tambem no jogador (`[Rage] PlayerVisual`, mesma escala 0-3). A ponte escreve no log `rage visual: mode N glow ok/MISSING symbol ok/MISSING`.
- Diagnostico das quedas do jogador: linhas `PLAYER ANIM: staggerStart/recoilStart/recoilLargeStart/bleedOutStart/KnockDown/ragdollStart` no log, na ordem em que acontecem, entre os golpes e custos de estamina.

## v0.11.0 - HUD proprio (fase 1)

- Novo `Data/Interface/NahIDWinHUD.swf` (ActionScript 2), carregado pela DLL dentro do "HUD Menu" (como o moreHUD).
- Mostra **barra de postura** e **barra de furia** (tempo restante enquanto ativa; recarga caso contrario) no jogador (por baixo da estamina) e no alvo da mira (por baixo da barra de vida do inimigo, ancorada a ela). Icone losango piscante quando a furia esta ativa.
- Poise continua na barra especial do TrueHUD.
- Nova seccao `[HUD]` no .ini e bloco "HUD (barras proprias)" no menu (Advanced).
- Fase 2 (por fazer): alvo por raio em volta da mira quando nao ha lock-on.
- Log: procura `scaleform interface: NahIDWinHUD registered` e `hud widget: NahIDWinHUD.swf load requested (ok)`.

### v0.11.0 (correcoes apos o primeiro teste)
- Menu: a tabela de opcoes antiga ficava guardada no save e a seccao "HUD (own bars)" aparecia vazia. Agora a tabela e refeita quando muda a revisao.
- HUD do alvo: se nao ha lock-on usa o alvo debaixo da mira (crosshair); se a barra de vida do jogo estiver escondida (TrueHUD) usa a posicao fixa `TargetFallbackX/Y`. Log: `hud widget: target ...`.
- Bridge: re-regista eventos de mod/animacao a cada load (saves antigos nao tinham o evento da furia); log `bridge 0.11.0: ...`.
- O custo de vida da furia deixou de ser reduzido pela propria furia.

## v0.12.0

- **Furia**: reducao extra de dano (25-50%, por cima de toda a resistencia; so afeta o dano que ainda chega a vida), bonus de dano (10-30%, armas e magias), custo em vida 10-30% (omissao 20%), imunidade a stagger durante a furia (jogador e NPCs). Chaves novas no `.ini` `[Rage]`: `LifeCostPercent`, `ExtraReduction`, `NPCExtraReduction`, `DamageBonus`, `NPCDamageBonus`, `StaggerImmune` (as antigas `DamageReductionPercent` deixam de ser usadas).
- **Golpes fortes**: o jogador nao leva stagger durante `[Stagger] PowerAttackImmuneSeconds` (2 s) depois de iniciar um golpe forte.
- **HUD**: barras do alvo em posicao fixa por defeito (`TargetAnchor=0`); escondem-se com qualquer menu aberto.
- Diagnostico: mais eventos de animacao no log (`PLAYER ANIM`).
- Por fazer: modo manual com aura de fogo (G liga/desliga, 10 s, 2%/s de vida, animacao via DAR).

## v0.13.0

- **Queda ao quebrar o poise de um inimigo (causa raiz)**: `Actor.PushActorAway(alvo, forca)` empurra o argumento para longe de quem chama. O script chamava `vitima.PushActorAway(atacante)`, ou seja empurrava o ATACANTE (o jogador). Agora e `atacante.PushActorAway(vitima)`.
- **Escudo de stagger na furia (jogador e NPCs)**: a DLL encadeia a funcao de notificacao do grafo de animacao e ignora `staggerStart` / `recoilStart`... enquanto o ator esta em furia. Os nossos proprios staggers passam sempre. `[Rage] StaggerImmune=0` desliga. A imunidade em golpes fortes foi retirada.
- **Barras estilo Sekiro**: a postura enche dos dois lados para o centro (`[HUD] BarStyle=1`; 0 = plana). Posicao por preset (esquerda/centro/direita x cima/centro/baixo) + ajuste X/Y em pixeis, para jogador e alvo, no menu e no ini.
- Log: `stagger shield: hook installed...` e `shield blocked=N` no SUMMARY.

## v0.14.0

- **Vidas extra do jogador** (`[Lives]`): 2a vida no nivel 30, 3a no nivel 60. Quando o dano mataria o jogador, a DLL deixa-o a 1 de vida, repoe 50%, da 3 s de invulnerabilidade e o Bridge tira 25% do ouro. Enchem ao dormir numa cama (esperar nao conta). Guardado no save pelo Bridge (`livesUsed`).
- **Supremo Rage** (`[Supreme]`, tecla H, ligar/desligar): 10 s, custo 2% da vida por segundo, aura que queima inimigos hostis num raio, mais tudo o que a furia normal da. Recarga de 500 s.
- **HUD**: losangos das vidas extra (laranja = disponivel, cinzento = gasta) e estrela do Supremo Rage (laranja = pronto, cinzento = recarga, pisca = ativo) junto das barras.
- **Menu retratil**: cada bloco abre/fecha (estado em `[MenuOpen]` do ini) e as opcoes de uma funcao desligada ficam escondidas.
- Pendente: efeito visual de fogo na aura (por agora brilho + simbolo da furia) e a animacao.

## v0.15.0

- **Bleedout fiavel**: o Papyrus repete o pedido de `BleedoutStart` ate o grafo de animacao o aceitar (a DLL diz se aceitou; se falhar 6 vezes o personagem e empurrado ao chao). Durante o bleedout (e na janela ate a animacao arrancar) o escudo de stagger bloqueia stagger leve e recuos, que cancelavam o bleedout.
- **Vida extra com bleedout**: ao gastar uma vida o jogador cai em bleedout, fica invulneravel e levanta-se com a vida reposta.
- **Bosses com 2 vidas** (`[Bosses]`): NPC unico (flag do jogo) com nivel minimo (30), nao companheiro. Ao gastar a 1a vida volta com a vida (100%) e cai em bleedout. Execucao e dragoes/gigantes ficam para a v0.16.0.
- **HUD**: vidas extra em circulos verdes (jogador) e vermelhos (bosses); estrela laranja do Supremo Rage. Postura estilo Sekiro agora enche do centro para as extremidades e pisca vermelho quando cheia.
- **TrueHUD**: a barra especial passa a ser o POISE (a descer; a zero: stagger pesado). `Bar=0` por omissao; a postura fica nas barras proprias.
- **Menu/ini**: gasto de estamina por golpe do jogador e do NPC, ligado, 10 e 10 por omissao.
