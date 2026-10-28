Scriptname NahIDWinBridge extends Quest
{ NAH-I-D-WIN - ponte minima. A DLL faz todas as contas (poise e postura) e envia UM evento de mod
  ("NahIDWin_Stagger") so quando e preciso reagir:
   tipo 1,2,3 = stagger leve / medio / pesado  -> lanca um de tres feiticos de stagger do jogo
   tipo 4     = BleedOut (a postura quebrou)   -> toca a animacao de ajoelhado durante alguns segundos
   tipo 5     = stagger pesado + empurrao (golpe forte, criatura gigante)
   tipo 6     = parry perfeito do jogador      -> faiscas e som (os do Inpa Sekiro, se estiver instalado) e vida devolvida
   tipo 7     = exaustao (estamina a zero)     -> jogador: sem atacar e sem regenerar por uns segundos; NPC: stagger pesado
  O jogador tambem usa eventos de animacao (um por balanco, golpe forte, bloqueio) e uma tecla (furia).
  Nao ha ciclos nem consultas repetidas. }

Spell sp1
Spell sp2
Spell sp3

Event OnInit()
	RegisterForModEvent("NahIDWin_Stagger", "OnNahStagger")
	RegisterForModEvent("NahIDWin_Load", "OnNahLoad")
	RegisterInputs()
	NahIDWinNative.ScriptReady()
EndEvent

; A DLL avisa quando um save e carregado: os registos de animacao perdem-se, entao registamos de novo.
Function OnNahLoad(string eventName, string strArg, float numArg, Form sender)
	RegisterInputs()
EndFunction

Function RegisterInputs()
	Actor pl = Game.GetPlayer()
	if pl
		RegisterForAnimationEvent(pl, "weaponSwing")
		RegisterForAnimationEvent(pl, "weaponLeftSwing")
		RegisterForAnimationEvent(pl, "blockStart")
		RegisterForAnimationEvent(pl, "bashStart")
		RegisterForAnimationEvent(pl, "bashPowerStart")
		RegisterForAnimationEvent(pl, "attackPowerStartInPlace")
		RegisterForAnimationEvent(pl, "attackPowerStartForward")
		RegisterForAnimationEvent(pl, "attackPowerStartBackward")
		RegisterForAnimationEvent(pl, "attackPowerStartLeft")
		RegisterForAnimationEvent(pl, "attackPowerStartRight")
		RegisterForAnimationEvent(pl, "attackPowerStartDualWield")
		ApplyRegen()
	endif
	UnregisterForAllKeys()
	int key = NahIDWinNative.GetRageKey()
	if key > 0
		RegisterForKey(key)
	endif
EndFunction

; Regeneracao de estamina do jogador conforme a armadura (100 = normal).
Function ApplyRegen()
	float r = NahIDWinNative.GetRegenPercent()
	if r > 0.0
		Game.GetPlayer().SetActorValue("StaminaRateMult", r)
	endif
EndFunction

; Cada golpe do jogador gasta estamina (mesmo sem acertar). O custo depende da arma, do tipo de golpe e da armadura.
; Levantar o bloqueio abre a janela do parry.
Event OnAnimationEvent(ObjectReference akSource, string asEventName)
	if asEventName == "blockStart"
		NahIDWinNative.BlockStart()
		return
	endif
	Actor pl = Game.GetPlayer()
	int mode = 0
	if asEventName == "bashStart" || asEventName == "bashPowerStart"
		mode = 2
	elseif asEventName != "weaponSwing" && asEventName != "weaponLeftSwing"
		mode = 1
	endif
	int hand = 1
	if asEventName == "weaponLeftSwing"
		hand = 0
	endif
	float cost = NahIDWinNative.GetSwingCost(mode, pl.GetEquippedItemType(hand))
	if cost > 0.0
		pl.DamageActorValue("Stamina", cost)
		if pl.GetActorValue("Stamina") <= 0.0
			if NahIDWinNative.TryExhaust()
				DoExhaustPlayer()
			endif
		endif
	endif
	ApplyRegen()
EndEvent

; Exaustao do jogador: sem atacar e sem regenerar estamina durante alguns segundos.
Function DoExhaustPlayer()
	Actor pl = Game.GetPlayer()
	float secs = NahIDWinNative.GetExhaustSeconds()
	float savedRate = pl.GetActorValue("StaminaRateMult")
	pl.SetActorValue("StaminaRateMult", 0.0)
	Game.DisablePlayerControls(false, true, false, false, false, false, false, false, 0)
	Debug.Notification("NAH-I-D-WIN: EXAUSTO!")
	Utility.Wait(secs)
	Game.EnablePlayerControls(false, true, false, false, false, false, false, false, 0)
	float r = NahIDWinNative.GetRegenPercent()
	if r > 0.0
		pl.SetActorValue("StaminaRateMult", r)
	else
		pl.SetActorValue("StaminaRateMult", savedRate)
	endif
EndFunction

; Passou o tempo de recarga da furia.
Event OnUpdate()
	Debug.Notification("NAH-I-D-WIN: furia pronta")
EndEvent

; Tecla da furia: imunidade temporaria em troca de vida ou estamina.
Event OnKeyDown(int keyCode)
	if Utility.IsInMenuMode()
		return
	endif
	if keyCode != NahIDWinNative.GetRageKey()
		return
	endif
	float cost = NahIDWinNative.TryRage()
	if cost < 0.0
		Debug.Notification("NAH-I-D-WIN: furia indisponivel (em recarga)")
		return
	endif
	if cost > 0.0
		if NahIDWinNative.GetRageCostType() == 0
			Game.GetPlayer().DamageActorValue("Health", cost)
		else
			Game.GetPlayer().DamageActorValue("Stamina", cost)
		endif
	endif
	Debug.Notification("NAH-I-D-WIN: FURIA!")
	RegisterForSingleUpdate(NahIDWinNative.GetRageCooldown())
EndEvent

; sender = a vitima. numArg = sequencia * 8 + tipo (1..6). strArg = form id do agressor (decimal, pode ser negativo).
Function OnNahStagger(string eventName, string strArg, float numArg, Form sender)
	int packed = numArg as int
	int kind = packed % 8
	Actor victim = sender as Actor
	if victim
		if kind == 4
			DoBleedOut(victim, packed)
			return
		endif
		if kind == 6
			DoParryFX(victim)
			float back = NahIDWinNative.GetParryRefund()
			if back > 0.0
				victim.RestoreActorValue("Health", back)
			endif
			NahIDWinNative.Ack(packed)
			return
		endif
		if kind == 7
			if victim == Game.GetPlayer()
				NahIDWinNative.Ack(packed)
				DoExhaustPlayer()
				return
			endif
			; NPC sem estamina: stagger pesado
		endif
		int tierSpell = kind
		if kind == 5 || kind == 7
			tierSpell = 3
		endif
		Spell sp = GetSpellForTier(tierSpell)
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
			if kind == 5
				float force = NahIDWinNative.GetKnockbackForce()
				if force > 0.0 && attacker && attacker != victim
					victim.PushActorAway(attacker, force)
				endif
			endif
		endif
	endif
	NahIDWinNative.Ack(packed)
EndFunction

; Faiscas e som do parry: usa os do mod Inpa Sekiro Combat se estiver instalado (se nao, nao faz nada).
Function DoParryFX(Actor victim)
	Explosion ex = Game.GetFormFromFile(0x00F39875, "Inpa Sekiro Combat.esp") as Explosion
	if ex
		victim.PlaceAtMe(ex)
	endif
	int left = victim.GetEquippedItemType(0)
	int right = victim.GetEquippedItemType(1)
	Sound snd
	if left == 10
		snd = Game.GetFormFromFile(0x00E74160, "Inpa Sekiro Combat.esp") as Sound
	elseif right == 0
		snd = Game.GetFormFromFile(0x00FE5A86, "Inpa Sekiro Combat.esp") as Sound
	elseif right == 2
		snd = Game.GetFormFromFile(0x00FBD281, "Inpa Sekiro Combat.esp") as Sound
	elseif right == 4 || right == 6
		snd = Game.GetFormFromFile(0x00F48B79, "Inpa Sekiro Combat.esp") as Sound
	else
		snd = Game.GetFormFromFile(0x000012D2, "Inpa Sekiro Combat.esp") as Sound
	endif
	if snd
		snd.Play(victim)
	endif
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
