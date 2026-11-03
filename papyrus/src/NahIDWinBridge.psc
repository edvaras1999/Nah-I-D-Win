Scriptname NahIDWinBridge extends Quest
{ NAH-I-D-WIN - ponte minima. A DLL faz todas as contas (poise e postura) e envia UM evento de mod
  ("NahIDWin_Stagger") so quando e preciso reagir:
   tipo 1,2,3 = stagger leve / medio / pesado  -> lanca um de tres feiticos de stagger do jogo
   tipo 4     = BleedOut (a postura quebrou)   -> toca a animacao de ajoelhado durante alguns segundos
   tipo 5     = stagger pesado + empurrao (golpe forte, criatura gigante)
   tipo 6     = parry perfeito do jogador      -> faiscas e som (do pacote de efeitos)
   tipo 7     = exaustao (estamina a zero)     -> jogador: sem atacar e sem regenerar por uns segundos; NPC: stagger pesado
  O jogador tambem usa eventos de animacao (um por balanco, golpe forte). Parry, furia e dano de vida sao feitos na DLL.
  Nao ha ciclos nem consultas repetidas. }

Spell sp1
Spell sp2
Spell sp3

Event OnInit()
	RegisterForModEvent("NahIDWin_Stagger", "OnNahStagger")
	RegisterForModEvent("NahIDWin_Load", "OnNahLoad")
	RegisterForModEvent("NahIDWin_Combatant", "OnNahCombatant")
	RegisterForModEvent("NahIDWin_Rage", "OnNahRage")
	RegisterForModEvent("NahIDWin_Life", "OnNahLife")
	RegisterForModEvent("NahIDWin_Essential", "OnNahEssential")
	RegisterForModEvent("NahIDWin_BossLife", "OnNahBossLife")
	RegisterForSleep()
	RegisterInputs()
	NahIDWinNative.ScriptReady()
EndEvent

; A DLL avisa quando um save e carregado: os registos de animacao perdem-se, entao registamos de novo.
Function OnNahLoad(string eventName, string strArg, float numArg, Form sender)
	; saves antigos nao tem os registos novos (OnInit so corre em jogos novos): registamos sempre
	RegisterForModEvent("NahIDWin_Stagger", "OnNahStagger")
	RegisterForModEvent("NahIDWin_Combatant", "OnNahCombatant")
	RegisterForModEvent("NahIDWin_Rage", "OnNahRage")
	RegisterForModEvent("NahIDWin_Life", "OnNahLife")
	RegisterForModEvent("NahIDWin_Essential", "OnNahEssential")
	RegisterForModEvent("NahIDWin_BossLife", "OnNahBossLife")
	RegisterForSleep()
	NahIDWinNative.SetLivesUsed(livesUsed)
	RegisterInputs()
	NahIDWinNative.Log("bridge 0.15.3: eventos de mod e de animacao registados")
	; o Inpa reaplica os Game Settings dele ao carregar (bloqueio dos NPCs a 0.60); depois disso repomos o valor escolhido no menu
	Utility.Wait(8.0)
	ApplyNpcBlock()
EndFunction

Function ApplyNpcBlock()
	if NahIDWinNative.IniGetF("General", "Enabled", 1.0) > 0.5
		Game.SetGameSettingFloat("fCombatBlockChanceMin", NahIDWinNative.IniGetF("NpcBlock", "ChanceMin", 0.3))
	endif
EndFunction

Function RegisterInputs()
	Actor pl = Game.GetPlayer()
	if pl
		RegisterForAnimationEvent(pl, "weaponSwing")
		RegisterForAnimationEvent(pl, "weaponLeftSwing")
		RegisterForAnimationEvent(pl, "bashStart")
		RegisterForAnimationEvent(pl, "bashPowerStart")
		RegisterForAnimationEvent(pl, "attackPowerStartInPlace")
		RegisterForAnimationEvent(pl, "attackPowerStartForward")
		RegisterForAnimationEvent(pl, "attackPowerStartBackward")
		RegisterForAnimationEvent(pl, "attackPowerStartLeft")
		RegisterForAnimationEvent(pl, "attackPowerStartRight")
		RegisterForAnimationEvent(pl, "attackPowerStartDualWield")
		RegisterForAnimationEvent(pl, "staggerStart")
		; so para o diagnostico das quedas do jogador (so escrevem no log)
		RegisterForAnimationEvent(pl, "recoilStart")
		RegisterForAnimationEvent(pl, "recoilLargeStart")
		RegisterForAnimationEvent(pl, "bleedOutStart")
		RegisterForAnimationEvent(pl, "KnockDown")
		RegisterForAnimationEvent(pl, "ragdollStart")
		RegisterForAnimationEvent(pl, "recoilStop")
		RegisterForAnimationEvent(pl, "staggerStop")
		RegisterForAnimationEvent(pl, "attackPowerStartForwardLeftHand")
		RegisterForAnimationEvent(pl, "ragdollEnd")
		RegisterForAnimationEvent(pl, "GetUpStart")
		RegisterForAnimationEvent(pl, "GetUpEnd")
		ApplyRegen()
	endif
EndFunction

; Depois de um stagger o jogador fica uns segundos sem poder ser staggerado de novo (para poder bloquear ou esquivar).
; [Stagger] PlayerImmuneAfterSeconds no .ini (0 = desligado). Staggers pesados do nosso mod (poise quebrado) passam sempre.
bool sNoStagger = false

Function OnPlayerStagger()
	float win = NahIDWinNative.IniGetF("Stagger", "PlayerImmuneAfterSeconds", 1.5)
	if win <= 0.0 || sNoStagger
		return
	endif
	sNoStagger = true
	Game.GetPlayer().SetAnimationVariableBool("bNoStagger", true)
	RegisterForSingleUpdate(win)
EndFunction

Event OnUpdate()
	Game.GetPlayer().SetAnimationVariableBool("bNoStagger", false)
	sNoStagger = false
EndEvent

; ---------------- vidas extra do jogador ----------------
int livesUsed = 0
bool sleptInBed = false

; A DLL avisa quando uma vida extra foi gasta (numArg = quantas ja gastas): ouro, aviso e brilho.
; Marca/desmarca "essencial" (como o SkyTweak): o jogo so faz o bleedout verdadeiro se a flag ja la estiver.
Function OnNahEssential(string eventName, string strArg, float numArg, Form sender)
	Actor a = sender as Actor
	if a
		ActorBase ab = a.GetActorBase()
		if ab
			ab.SetEssential(numArg > 0.5)
		endif
	endif
EndFunction

; Boss gastou a vida extra: o jogo fez o bleedout; espera, repoe a vida, tira a flag e volta a lutar.
Function OnNahBossLife(string eventName, string strArg, float numArg, Form sender)
	Actor b = sender as Actor
	if !b
		return
	endif
	float secs = numArg
	if secs < 3.0
		secs = 3.0
	endif
	NahIDWinNative.Log("boss life: bleedout for " + secs + " s")
	Utility.Wait(secs)
	if b.IsDead()
		return
	endif
	NahIDWinNative.BossRecover(b)
	ActorBase ab = b.GetActorBase()
	if ab
		ab.SetEssential(false)
	endif
	b.StartCombat(Game.GetPlayer())
	NahIDWinNative.Log("boss life: restored, back to combat")
EndFunction

Function OnNahLife(string eventName, string strArg, float numArg, Form sender)
	livesUsed = numArg as int
	Actor pl = Game.GetPlayer()
	float pct = NahIDWinNative.IniGetF("Lives", "GoldLossPercent", 25.0)
	Form gold = Game.GetForm(0x0000000F)
	int lost = 0
	if gold && pct > 0.0
		int have = pl.GetItemCount(gold)
		lost = ((have as float) * pct / 100.0) as int
		if lost > 0
			pl.RemoveItem(gold, lost, true, None)
		endif
	endif
	if NahIDWinNative.GetLanguage() == 0
		Debug.Notification("Vida extra usada (" + livesUsed + " gasta(s)). Ouro perdido: " + lost)
	else
		Debug.Notification("Extra life used (" + livesUsed + " spent). Gold lost: " + lost)
	endif
	; Metodo do SkyTweak: o jogo fez o bleedout verdadeiro (jogador essencial). Espera, acaba o combate e repoe a vida.
	if NahIDWinNative.LifeMethod() == 1
		float secs = NahIDWinNative.GetBleedSeconds()
		if secs < 5.0
			secs = 5.0
		endif
		NahIDWinNative.Log("extra life: bleedout (game) for " + secs + " s")
		; Igual ao SkyTweak: acaba o alarme de combate logo no inicio do bleedout, espera, repoe a vida.
		pl.StopCombatAlarm()
		Utility.Wait(secs)
		pl.ResetHealthAndLimbs()
		NahIDWinNative.LifeRecover()
		NahIDWinNative.Log("extra life: health restored, standing up")
	endif
	Spell glow = Game.GetFormFromFile(0x00000841, "NahIDWin.esp") as Spell
	if glow
		glow.Cast(pl, pl)
		Utility.Wait(3.0)
		pl.DispelSpell(glow)
	endif
EndFunction

; Dormir numa cama enche as vidas extra outra vez (esperar nao conta).
Event OnSleepStart(float afSleepStartTime, float afDesiredSleepEndTime)
	Actor pl = Game.GetPlayer()
	sleptInBed = (pl.GetFurnitureReference() != None) && (pl.GetSleepState() >= 2)
EndEvent

Event OnSleepStop(bool abInterrupted)
	if sleptInBed && !abInterrupted && livesUsed > 0
		livesUsed = 0
		NahIDWinNative.SetLivesUsed(0)
		if NahIDWinNative.GetLanguage() == 0
			Debug.Notification("Vidas extra recuperadas.")
		else
			Debug.Notification("Extra lives restored.")
		endif
	endif
	sleptInBed = false
EndEvent

; Furia (jogador e NPC): brilho e/ou simbolo vermelho.
; [Rage] StaggerImmune: 1 = ligado. [Rage] NPCVisual / PlayerVisual: 0 nada, 1 brilho, 2 simbolo, 3 os dois.
Function OnNahRage(string eventName, string strArg, float numArg, Form sender)
	Actor a = sender as Actor
	if !a
		return
	endif
	int mode = NahIDWinNative.IniGetF("Rage", "NPCVisual", 1.0) as int
	if a == Game.GetPlayer()
		mode = NahIDWinNative.IniGetF("Rage", "PlayerVisual", 1.0) as int
	endif
	if strArg == "supreme"
		mode = 3
	endif
	Spell glow = None
	Spell symbol = None
	if mode == 1 || mode == 3
		glow = Game.GetFormFromFile(0x00000841, "NahIDWin.esp") as Spell
	endif
	if mode == 2 || mode == 3
		symbol = Game.GetFormFromFile(0x00000843, "NahIDWin.esp") as Spell
	endif
	string gs = "MISSING"
	string ss = "MISSING"
	if glow
		gs = "ok"
	endif
	if symbol
		ss = "ok"
	endif
	NahIDWinNative.Log("rage visual: mode " + mode + " glow " + gs + " symbol " + ss)
	if numArg > 0.0
		if glow
			glow.Cast(a, a)
		endif
		if symbol
			symbol.Cast(a, a)
		endif
		Utility.Wait(numArg)
	endif
	if glow
		a.DispelSpell(glow)
	endif
	if symbol
		a.DispelSpell(symbol)
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
Event OnAnimationEvent(ObjectReference akSource, string asEventName)
	Actor pl = Game.GetPlayer()
	if asEventName == "recoilStop" || asEventName == "staggerStop" || asEventName == "ragdollEnd" || asEventName == "GetUpStart" || asEventName == "GetUpEnd"
		NahIDWinNative.Log("PLAYER ANIM: " + asEventName)
		return
	endif
	if asEventName == "staggerStart" || asEventName == "recoilStart" || asEventName == "recoilLargeStart" || asEventName == "bleedOutStart" || asEventName == "KnockDown" || asEventName == "ragdollStart"
		NahIDWinNative.Log("PLAYER ANIM: " + asEventName + " (stamina " + pl.GetActorValue("Stamina") + ")")
		if asEventName == "staggerStart"
			OnPlayerStagger()
		endif
		return
	endif
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
	NahIDWinNative.Log("exhaust player start " + secs)
	if NahIDWinNative.GetLanguage() == 1
		Debug.Notification("NAH-I-D-WIN: EXAUSTO!")
	else
		Debug.Notification("NAH-I-D-WIN: EXHAUSTED!")
	endif
	Utility.Wait(secs)
	Game.EnablePlayerControls(false, true, false, false, false, false, false, false, 0)
	float r = NahIDWinNative.GetRegenPercent()
	if r > 0.0
		pl.SetActorValue("StaminaRateMult", r)
	else
		pl.SetActorValue("StaminaRateMult", savedRate)
	endif
EndFunction

; sender = a vitima. numArg = sequencia * 8 + tipo (1..6). strArg = form id do agressor (decimal, pode ser negativo).
Function OnNahStagger(string eventName, string strArg, float numArg, Form sender)
	int packed = numArg as int
	int kind = packed % 8
	Actor victim = sender as Actor
	if victim
		if kind == 4
			Actor aggr = None
			int aid = strArg as int
			if aid != 0
				aggr = Game.GetForm(aid) as Actor
			endif
			DoBleedOut(victim, packed, aggr)
			return
		endif
		if kind == 6
			DoParryFX(victim)
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
		if victim == Game.GetPlayer() && sNoStagger && kind >= 2
			; stagger do nosso mod (poise quebrado etc.) tem de passar mesmo durante a janela de imunidade
			victim.SetAnimationVariableBool("bNoStagger", false)
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
					attacker.PushActorAway(victim, force)
				endif
			endif
		endif
	endif
	NahIDWinNative.Ack(packed)
EndFunction

; Faiscas e som do parry: formas proprias do NahIDWin.esp (os ficheiros .nif/.wav vem do pacote de efeitos).
Function DoParryFX(Actor victim)
	Explosion ex = Game.GetFormFromFile(0x00000810, "NahIDWin.esp") as Explosion
	if ex
		victim.PlaceAtMe(ex)
	endif
	int left = victim.GetEquippedItemType(0)
	int right = victim.GetEquippedItemType(1)
	Sound snd
	if left == 10
		snd = Game.GetFormFromFile(0x00000811, "NahIDWin.esp") as Sound
	elseif right == 0
		snd = Game.GetFormFromFile(0x00000812, "NahIDWin.esp") as Sound
	elseif right == 2
		snd = Game.GetFormFromFile(0x00000813, "NahIDWin.esp") as Sound
	elseif right == 4 || right == 6
		snd = Game.GetFormFromFile(0x00000814, "NahIDWin.esp") as Sound
	else
		snd = Game.GetFormFromFile(0x00000815, "NahIDWin.esp") as Sound
	endif
	if snd
		snd.Play(victim)
	endif
EndFunction

; BleedOut: ajoelhado e indefeso. O jogador perde o controlo durante esse tempo.
Function DoBleedOut(Actor victim, int packed, Actor aggressor)
	NahIDWinNative.Ack(packed)
	float secs = NahIDWinNative.GetBleedSeconds()
	bool isPlayer = (victim == Game.GetPlayer())
	if isPlayer
		Game.DisablePlayerControls(true, true, false, false, false, true, true, false, 0)
	endif
	NahIDWinNative.Log("bleedout start " + secs)
	; NPC com o Inpa fundido: postura quebrada = o "poisebreak" do Inpa (stagger grande, som e janela de execucao com animacao do jogo base).
	; Sem o Inpa (ou no jogador) fica a animacao de ajoelhado de sempre.
	Spell pb = None
	if !isPlayer
		pb = Game.GetFormFromFile(0x0075FAAD, "NahIDWin.esp") as Spell
	endif
	if pb
		NahIDWinNative.OwnAnim(700)
		DoInpaPostureBreak(victim, aggressor, pb)
	else
		; Repete o pedido de BleedoutStart ate o grafo de animacao o aceitar (a DLL diz se aceitou).
		int vid = victim.GetFormID()
		int tries = 0
		bool ok = false
		while tries < 6 && !ok
			NahIDWinNative.OwnAnim(600)
			Debug.SendAnimationEvent(victim, "BleedoutStart")
			Utility.Wait(0.25)
			ok = NahIDWinNative.BleedAccepted(vid)
			tries += 1
		endwhile
		NahIDWinNative.Log("bleedout animation accepted=" + ok + " after " + tries + " tries")
		if !ok
			victim.PushActorAway(victim, 0.0)
		endif
	endif
	Utility.Wait(secs)
	if !victim.IsDead() && !pb
		NahIDWinNative.OwnAnim(1500)
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

; Igual ao que o Inpa faz quando o poise do inimigo quebra, mas disparado pela NOSSA postura e sem mexer na estamina do alvo.
Function DoInpaPostureBreak(Actor victim, Actor aggressor, Spell pb)
	victim.SetAnimationVariableBool("bNoStagger", false)
	victim.SetAnimationVariableFloat("StaggerMagnitude", 10.0)
	if !victim.IsDead() && !victim.IsInKillMove()
		Debug.SendAnimationEvent(victim, "StaggerStart")
	endif
	Actor caster = aggressor
	if !caster
		caster = Game.GetPlayer()
	endif
	if caster == Game.GetPlayer()
		Sound mk = Game.GetFormFromFile(0x00006E76, "NahIDWin.esp") as Sound
		if mk
			mk.Play(caster)
		endif
	endif
	pb.Cast(caster, victim)
	victim.SetAnimationVariableBool("isBlocking", false)
	NahIDWinNative.Log("posture break -> Inpa poisebreak cast on " + victim)
EndFunction

; Substitui o "cloak" do Inpa: so quem entra em combate recebe os feiticos dele (aliado ou inimigo), uma vez por carregamento.
Function OnNahCombatant(string eventName, string strArg, float numArg, Form sender)
	Actor a = sender as Actor
	if !a || a == Game.GetPlayer() || a.IsDead()
		return
	endif
	Spell sp
	if Game.GetPlayer().GetFactionReaction(a) == 2
		sp = Game.GetFormFromFile(0x00908EDF, "NahIDWin.esp") as Spell
	else
		sp = Game.GetFormFromFile(0x000048BF, "NahIDWin.esp") as Spell
	endif
	if sp && !a.HasSpell(sp)
		a.AddSpell(sp, false)
	endif
EndFunction
