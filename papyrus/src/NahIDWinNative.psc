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

; Custo de estamina de um golpe do jogador (0 = desligado ou ja pago). mode: 0 balanco, 1 golpe forte, 2 escudo.
; rightType: codigo de GetEquippedItemType. A DLL junta o tipo de arma e a armadura. Forca do empurrao (0 = desligado).
float Function GetSwingCost(int mode, int rightType) global native
float Function GetKnockbackForce() global native

; Furia: tecla do jogador (0 = desligada), tipo de custo (0 vida, 1 estamina), pedido de ativacao.
; TryRage devolve -1 se nao pode (recarga ou BleedOut) ou o custo a pagar (>= 0).
int Function GetRageKey() global native
int Function GetRageCostType() global native
float Function TryRage() global native

; Chamada quando o jogador levanta o bloqueio: abre a janela do parry.
Function BlockStart() global native

; Armadura abranda a regeneracao de estamina: percentagem (100 = normal). Devolve -1 se estiver desligado (nao mexer).
float Function GetRegenPercent() global native

; Exaustao (estamina a zero): duracao em segundos e pedido de entrada (false = em recarga).
float Function GetExhaustSeconds() global native
bool Function TryExhaust() global native

; Vida a devolver depois de um parry sem dano.
float Function GetParryRefund() global native

; Tempo de recarga da furia em segundos (para o aviso "furia pronta").
float Function GetRageCooldown() global native

; Ler e escrever o NahIDWin.ini (usado pelo menu MCM).
float Function IniGetF(string section, string key, float defaultValue) global native
Function IniSetF(string section, string key, float value) global native
