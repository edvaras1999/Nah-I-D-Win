Scriptname NahIDWinBridge extends Quest
{ NAH-I-D-WIN - ponte minima. A DLL faz todas as contas (poise e postura) e envia UM evento de mod
  ("NahIDWin_Stagger") so quando e preciso reagir. Este script recebe-o e:
   tipo 1,2,3 = stagger leve / medio / pesado  -> lanca um de tres feiticos de stagger do jogo
   tipo 4     = BleedOut (a postura quebrou)   -> toca a animacao de ajoelhado durante alguns segundos
  Nao ha ciclos nem consultas repetidas. }

Spell sp1
Spell sp2
Spell sp3

Event OnInit()
	RegisterForModEvent("NahIDWin_Stagger", "OnNahStagger")
	NahIDWinNative.ScriptReady()
EndEvent

; sender = a vitima. numArg = sequencia * 8 + tipo (1..4). strArg = form id do agressor (decimal, pode ser negativo).
Function OnNahStagger(string eventName, string strArg, float numArg, Form sender)
	int packed = numArg as int
	int kind = packed % 8
	Actor victim = sender as Actor
	if victim
		if kind >= 4
			DoBleedOut(victim, packed)
			return
		endif
		Spell sp = GetSpellForTier(kind)
		if sp
			Actor attacker = None
			int attackerId = strArg as int
			if attackerId != 0
				attacker = Game.GetForm(attackerId) as Actor
			endif
			if attacker && attacker != victim
				sp.Cast(attacker, victim)
			else
				sp.Cast(victim, victim)
			endif
		endif
	endif
	NahIDWinNative.Ack(packed)
EndFunction

; BleedOut: ajoelhado e indefeso. O jogador perde o controlo durante esse tempo.
Function DoBleedOut(Actor victim, int packed)
	NahIDWinNative.Ack(packed)
	float secs = NahIDWinNative.GetBleedSeconds()
	bool isPlayer = (victim == Game.GetPlayer())
	if isPlayer
		Game.DisablePlayerControls(true, true, false, false, false, true, true, false, 0)
	endif
	Debug.SendAnimationEvent(victim, "BleedoutStart")
	Utility.Wait(secs)
	if !victim.IsDead()
		Debug.SendAnimationEvent(victim, "BleedoutStop")
	endif
	if isPlayer
		Game.EnablePlayerControls(true, true, true, true, true, true, true, true, 0)
	endif
	NahIDWinNative.BleedDone(packed)
EndFunction

Spell Function GetSpellForTier(int tier)
	if tier <= 1
		if !sp1
			sp1 = Game.GetFormFromFile(0x00000801, "NahIDWin.esp") as Spell
		endif
		return sp1
	elseif tier == 2
		if !sp2
			sp2 = Game.GetFormFromFile(0x00000802, "NahIDWin.esp") as Spell
		endif
		return sp2
	endif
	if !sp3
		sp3 = Game.GetFormFromFile(0x00000803, "NahIDWin.esp") as Spell
	endif
	return sp3
EndFunction
