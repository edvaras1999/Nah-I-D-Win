# PoiseLE

Sistema de **poise** para Skyrim **Legendary Edition** (1.9.32.0 + SKSE 1.7.3), inspirado nos mods de poise da SE.
Cada ator tem uma barra de poise; os golpes tiram poise; quando chega a zero o ator sofre stagger; o poise regenera.
A **stamina nao e tocada**. Nao ha scripts em ciclo.

Estado: **v0.1.0, fase 1, com diagnostico** (ainda nao testado no jogo).

## Como compilar (tudo no GitHub)
1. Cria um repositorio novo e envia estes arquivos mantendo as pastas.
2. Aba **Actions** > "Build PoiseLE" > **Run workflow** (tambem corre sozinho a cada envio).
3. No fim, baixa o artefacto **PoiseLE-instalacao**: e um zip pronto para instalar (MO2).
   Se algum passo falhar, baixa tambem os artefactos `dll` e `papyrus` (tem os logs) e envia-os.

## Instalar para testar
Instala o zip pelo MO2 e ativa **PoiseLE.esp**. Precisa de SKSE 1.7.3. Para ver barras: TrueHUD LE com as "special bars" ligadas.
Configuracao: `Data/SKSE/Plugins/PoiseLE.ini` (relido sozinho). Log: `Documentos/My Games/Skyrim/SKSE/PoiseLE.log`.

## Como funciona o stagger
- `StaggerMode=0` (por omissao): a DLL faz as contas e, **so quando o poise quebra**, envia um evento de mod.
  O script `PoiseLEBridge` (poucas linhas) recebe-o e lanca um de tres feiticos de stagger do proprio jogo (fraco/medio/forte).
- `StaggerMode=1` (experimental): sem Papyrus. A DLL tenta tocar a animacao de stagger. Serve para o diagnostico.

## Diagnostico
O log traz: cada golpe com as contas, o resumo a cada 30 s (eventos enviados, confirmados pelo script, atraso medio e maximo),
avisos se o script nao responde, e um despejo das funcoes de animacao do jogador. Envia o log depois de uma luta.

## Licenca
GPL-3.0 (ver `poise-le/LICENSE`). A base de compilacao e os enderecos vem do FreeClimb LE / SoftTarget LE. Os registos do efeito de stagger
seguem o mod "SGC Stagger on hit". Referencias: doodlum "Poise" (SE) e a API do TrueHUD LE (Mhacks79, baseada no TrueHUD de Ersh).
