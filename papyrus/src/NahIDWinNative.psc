Scriptname NahIDWinNative Native Hidden
{ Funcoes nativas implementadas pela NahIDWin.dll. Servem so para falar com o plugin (diagnostico e configuracao). }

; Chamada uma vez quando o script ponte arranca, para o log mostrar que o Papyrus esta vivo.
Function ScriptReady() global native

; Chamada depois de cada evento tratado. "packed" e o numero recebido no evento.
Function Ack(int packed) global native

; Duracao do BleedOut em segundos (vem do .ini).
float Function GetBleedSeconds() global native

; Chamada quando o BleedOut acabou (so para o log).
Function BleedDone(int packed) global native

; Custo de estamina por balanco do jogador (0 = desligado) e forca do empurrao (0 = desligado). Vem do .ini.
float Function GetSwingCost() global native
float Function GetKnockbackForce() global native

; Furia: tecla do jogador (0 = desligada), tipo de custo (0 vida, 1 estamina), pedido de ativacao.
; TryRage devolve -1 se nao pode (recarga ou BleedOut) ou o custo a pagar (>= 0).
int Function GetRageKey() global native
int Function GetRageCostType() global native
float Function TryRage() global native

; Chamada quando o jogador levanta o bloqueio: abre a janela do parry.
Function BlockStart() global native
