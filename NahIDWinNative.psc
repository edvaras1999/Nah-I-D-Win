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
