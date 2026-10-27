Scriptname PoiseLEBridge extends Quest
{ PoiseLE - minimal bridge. The DLL does all the poise maths and sends ONE mod event
  ("PoiseLE_Stagger") only when someone's poise breaks. This script receives it and makes the
  game play a stagger on the victim, using one of three spells (weak / medium / strong).
  There is no loop and no polling. }

Spell sp1
Spell sp2
Spell sp3

Event OnInit()
	RegisterForModEvent("PoiseLE_Stagger", "OnPoiseStagger")
	PoiseLENative.ScriptReady()
EndEvent

; sender = the victim. numArg = sequence * 4 + tier (tier 1..3). strArg = form id of the attacker (decimal, may be negative).
Event OnPoiseStagger(string eventName, string strArg, float numArg, Form sender)
	int packed = numArg as int
	int tier = packed % 4
	Actor victim = sender as Actor
	if victim
		Spell sp = GetSpellForTier(tier)
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
	PoiseLENative.Ack(packed)
EndEvent

Spell Function GetSpellForTier(int tier)
	if tier <= 1
		if !sp1
			sp1 = Game.GetFormFromFile(0x00000801, "PoiseLE.esp") as Spell
		endif
		return sp1
	elseif tier == 2
		if !sp2
			sp2 = Game.GetFormFromFile(0x00000802, "PoiseLE.esp") as Spell
		endif
		return sp2
	endif
	if !sp3
		sp3 = Game.GetFormFromFile(0x00000803, "PoiseLE.esp") as Spell
	endif
	return sp3
EndFunction
