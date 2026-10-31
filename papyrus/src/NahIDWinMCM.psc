Scriptname NahIDWinMCM extends SKI_ConfigBase
{ Menu UNICO do NAH-I-D-WIN (substitui o menu do Inpa Sekiro Combat; nao herda dele). Estilo do Inpa: cabecalhos laranja, MOD On/Off, sem imagens.
  Tres paginas: "MOD Setting", "Game Setting" e "Advanced".
  - Opcoes do Inpa (parry, bloqueio, dificuldade, resistencias, Game Settings): gravam nos MESMOS globais do Inpa e aplicam os mesmos Game Settings,
    por isso o resto do Inpa continua a funcionar e reaplica tudo ao carregar o jogo (nada precisa de ficar no save).
  - Opcoes nossas (poise, postura, furia, exaustao, estamina por golpe, stagger...): gravam no NahIDWin.ini atraves da DLL.
  - MOD On/Off liga e desliga o Inpa e o NahIDWin em conjunto.
  As propriedades abaixo tem os nomes das propriedades da quest do menu do Inpa (preenchidas pelo NahIDWin.esp). }

; ---------------- propriedades (preenchidas pelo ESP) ----------------
Actor Property PlayerRef Auto
GlobalVariable Property zzzSekiro_MCM_turn Auto
GlobalVariable Property zzzSekiro_parrytimer Auto
GlobalVariable Property zzzSekiro_staminaRecovery Auto
GlobalVariable Property zzzSekiro_parry_ward_opt Auto
GlobalVariable Property zzzSekiro_arrowparry Auto
GlobalVariable Property zzzSekiro_wardparry Auto
GlobalVariable Property zzzSekiro_player_blockdamage0 Auto
GlobalVariable Property zzzSekiro_block_resist_poison_opt Auto
GlobalVariable Property zzzSekiro_postureregen Auto
GlobalVariable Property zzzSekiro_killmove Auto
GlobalVariable Property zzzSekiro_killmove_sneak Auto
GlobalVariable Property zzzSekiro_player_sneak_damage_mult Auto
GlobalVariable Property zzzSekiro_alert Auto
GlobalVariable Property zzzSekiro_npc_arrow_parry Auto
GlobalVariable Property zzzSekiro_npc_magic_parry Auto
GlobalVariable Property zzzSekiro_npc_fist_parry Auto
GlobalVariable Property zzzSekiro_frost_resist Auto
GlobalVariable Property zzzSekiro_npc_magic_stagger Auto
GlobalVariable Property zzzSekiro_penrilrandom Auto
GlobalVariable Property zzzSekiro_dragon_staminaX Auto
GlobalVariable Property zzzSekiro_protect_friend Auto
GlobalVariable Property zzzSekiro_ally_dont_stamina_dmg Auto
GlobalVariable Property zzzSekiro_ally_cant_stagger Auto
GlobalVariable Property zzzSekiro_all_difficulty Auto
GlobalVariable Property zzzSekiro_player_difficulty_taken Auto
GlobalVariable Property zzzSekiro_npc_difficulty Auto
GlobalVariable Property zzzSekiro_npc_difficulty_taken Auto
GlobalVariable Property zzzSekiro_resurrection Auto
GlobalVariable Property zzzSekiro_posture_dmg_mult Auto
GlobalVariable Property zzzSekiro_posture_dmg_mult_player Auto
GlobalVariable Property zzzSekiro_poisesystem Auto
GlobalVariable Property STAMINA_POWERCOST_SLIDER_global Auto
GlobalVariable Property SPRINT_DRAINBASE_SLIDER_global Auto
GlobalVariable Property SPRINT_DRAINMULT_SLIDER_global Auto
GlobalVariable Property COMBAT_STAMINAREGEN_SLIDER_global Auto
GlobalVariable Property STAMINA_REGENDELAY_SLIDER_global Auto
GlobalVariable Property MAX_RESISTANCE_SLIDER_global Auto
GlobalVariable Property ARMOR_MAXRESIST_SLIDER_global Auto
GlobalVariable Property BLOCK_SHIELD_SLIDER_global Auto
GlobalVariable Property BLOCK_WEAPON_SLIDER_global Auto
Spell Property zzzSekiro_player_main_spell Auto
Spell Property zzzSekiro_player_main2_spell Auto
Spell Property zzzSekiro_player_main3_spell Auto
Spell Property zzzSekiro_cloak_spell Auto
Spell Property zzzSekiro_regen_blocking_spell Auto
Spell Property zzzSekiro_ward_spell Auto
Spell Property zzzSekiro_player_parrytimer_spell Auto
Spell Property zzzSekiro_player_parry_spell Auto
Spell Property zzzSekiro_player_parry2_spell Auto
Spell Property zzzSekiro_player_parry3_spell Auto
Spell Property zzzSekiro_zzResurrection_spell Auto

; ---------------- tabela das nossas opcoes (.ini) ----------------
int[] sBlk
int[] sType
string[] sSec
string[] sKey
string[] sLabelPT
string[] sLabelEN
string[] sInfoPT
string[] sInfoEN
float[] sMin
float[] sMax
float[] sStep
float[] sDef
int count

; ---------------- opcoes desenhadas na pagina ----------------
; tipos: 1 interruptor MOD, 2 toggle do .ini, 3 slider do .ini, 4 toggle global, 5 slider global, 6 menu
int[] oKind
int[] oRef
int[] oOpt
int sPageNo = -1
int sFlag = 0

; gid dos globais do Inpa
; 1 parrytimer 2 staminaRecovery 3 parry_ward 4 arrow 5 ward 6 blockdamage0 7 resist_poison 8 alert 9 npc_arrow 10 npc_magic 11 npc_fist
; 12 frost 13 npc_magic_stagger 14 perilous 15 dragon 16 protect_friend 17 ally_stamina 18 ally_stagger 19 killmove 20 killmove_sneak
; 21 sneak_mult 22 powercost 23 sprint_base 24 sprint_mult 25 combat_regen 26 regen_delay 27 max_res 28 armor_res 29 shield 30 weapon
; 31 player_difficulty_on 32 player_taken 33 npc_difficulty_on 34 npc_taken

int Function GetVersion()
	return 20
EndFunction

Event OnVersionUpdate(int a_version)
	SetupPages()
EndEvent

Event OnInit()
	parent.OnInit()
	SetupPages()
EndEvent

Event OnConfigInit()
	SetupPages()
EndEvent

Function SetupPages()
	ModName = "NAH-I-D-WIN"
	Pages = new string[3]
	Pages[0] = "MOD Setting"
	Pages[1] = "Game Setting"
	Pages[2] = "Advanced"
EndFunction

; ---------------- utilitarios ----------------
bool Function IsPT()
	return NahIDWinNative.GetLanguage() == 1
EndFunction

string Function Tr(string en, string pt)
	if IsPT()
		return pt
	endif
	return en
EndFunction

string Function Orange(string t)
	return "<font color=\"#d1820d\">" + t + "</font>"
EndFunction

bool Function ModOn()
	if zzzSekiro_MCM_turn
		return zzzSekiro_MCM_turn.GetValue() > 0.5
	endif
	return NahIDWinNative.IniGetF("General", "Enabled", 1.0) > 0.5
EndFunction

GlobalVariable Function GetG(int gid)
	if gid == 1
		return zzzSekiro_parrytimer
	elseif gid == 2
		return zzzSekiro_staminaRecovery
	elseif gid == 3
		return zzzSekiro_parry_ward_opt
	elseif gid == 4
		return zzzSekiro_arrowparry
	elseif gid == 5
		return zzzSekiro_wardparry
	elseif gid == 6
		return zzzSekiro_player_blockdamage0
	elseif gid == 7
		return zzzSekiro_block_resist_poison_opt
	elseif gid == 8
		return zzzSekiro_alert
	elseif gid == 9
		return zzzSekiro_npc_arrow_parry
	elseif gid == 10
		return zzzSekiro_npc_magic_parry
	elseif gid == 11
		return zzzSekiro_npc_fist_parry
	elseif gid == 12
		return zzzSekiro_frost_resist
	elseif gid == 13
		return zzzSekiro_npc_magic_stagger
	elseif gid == 14
		return zzzSekiro_penrilrandom
	elseif gid == 15
		return zzzSekiro_dragon_staminaX
	elseif gid == 16
		return zzzSekiro_protect_friend
	elseif gid == 17
		return zzzSekiro_ally_dont_stamina_dmg
	elseif gid == 18
		return zzzSekiro_ally_cant_stagger
	elseif gid == 19
		return zzzSekiro_killmove
	elseif gid == 20
		return zzzSekiro_killmove_sneak
	elseif gid == 21
		return zzzSekiro_player_sneak_damage_mult
	elseif gid == 22
		return STAMINA_POWERCOST_SLIDER_global
	elseif gid == 23
		return SPRINT_DRAINBASE_SLIDER_global
	elseif gid == 24
		return SPRINT_DRAINMULT_SLIDER_global
	elseif gid == 25
		return COMBAT_STAMINAREGEN_SLIDER_global
	elseif gid == 26
		return STAMINA_REGENDELAY_SLIDER_global
	elseif gid == 27
		return MAX_RESISTANCE_SLIDER_global
	elseif gid == 28
		return ARMOR_MAXRESIST_SLIDER_global
	elseif gid == 29
		return BLOCK_SHIELD_SLIDER_global
	elseif gid == 30
		return BLOCK_WEAPON_SLIDER_global
	elseif gid == 31
		return zzzSekiro_all_difficulty
	elseif gid == 32
		return zzzSekiro_player_difficulty_taken
	elseif gid == 33
		return zzzSekiro_npc_difficulty
	elseif gid == 34
		return zzzSekiro_npc_difficulty_taken
	endif
	return None
EndFunction

float Function GVal(int gid)
	GlobalVariable g = GetG(gid)
	if g
		return g.GetValue()
	endif
	return 0.0
EndFunction

; Slider: minimo / maximo / passo / valor por omissao / formato (lidos do menu do Inpa)
float Function GMin(int gid)
	if gid == 1
		return 0.1
	elseif gid == 15
		return 1.0
	elseif gid == 21
		return 5.0
	endif
	return 0.0
EndFunction

float Function GMax(int gid)
	if gid == 1
		return 0.2
	elseif gid == 2 || gid == 14
		return 10.0
	elseif gid == 15
		return 10.0
	elseif gid == 21
		return 20.0
	elseif gid == 22
		return 10.0
	elseif gid == 23
		return 50.0
	elseif gid == 24 || gid == 25 || gid == 29 || gid == 30
		return 1.0
	elseif gid == 26
		return 10.0
	elseif gid == 27 || gid == 28
		return 100.0
	endif
	return 1000.0
EndFunction

float Function GStep(int gid)
	if gid == 1 || gid == 24 || gid == 25 || gid == 29 || gid == 30
		return 0.01
	elseif gid == 15 || gid == 22 || gid == 26
		return 0.1
	elseif gid == 21 || gid == 32 || gid == 34
		return 5.0
	endif
	return 1.0
EndFunction

float Function GDef(int gid)
	if gid == 1
		return 0.15
	elseif gid == 2
		return 5.0
	elseif gid == 14
		return 7.0
	elseif gid == 15
		return 2.0
	elseif gid == 21
		return 5.0
	elseif gid == 22
		return 2.0
	elseif gid == 23
		return 7.0
	elseif gid == 24
		return 0.02
	elseif gid == 25
		return 0.35
	elseif gid == 26
		return 5.0
	elseif gid == 27
		return 100.0
	elseif gid == 28
		return 75.0
	elseif gid == 29
		return 0.75
	elseif gid == 30
		return 0.66
	elseif gid == 32
		return 100.0
	elseif gid == 34
		return 35.0
	endif
	return 0.0
EndFunction

string Function GFmt(int gid)
	if gid == 1
		return "{2}sec"
	elseif gid == 2 || gid == 14 || gid == 27 || gid == 28 || gid == 32 || gid == 34
		return "{0}%"
	elseif gid == 15
		return "{1}x"
	elseif gid == 21
		return "{0}x"
	elseif gid == 22 || gid == 26
		return "{1}"
	elseif gid == 23
		return "{0}"
	endif
	return "{2}"
EndFunction

string Function GGmst(int gid)
	if gid == 22
		return "fPowerAttackStaminaPenalty"
	elseif gid == 23
		return "fSprintStaminaDrainMult"
	elseif gid == 24
		return "fSprintStaminaWeightMult"
	elseif gid == 25
		return "fCombatStaminaRegenRateMult"
	elseif gid == 26
		return "fStaminaRegenDelayMax"
	elseif gid == 27
		return "fPlayerMaxResistance"
	elseif gid == 28
		return "fMaxArmorRating"
	elseif gid == 29
		return "fShieldBaseFactor"
	elseif gid == 30
		return "fBlockWeaponBase"
	endif
	return ""
EndFunction

Function Refresh(Spell s)
	if s
		PlayerRef.DispelSpell(s)
		PlayerRef.RemoveSpell(s)
		PlayerRef.AddSpell(s, false)
	endif
EndFunction

Function Strip(Spell s)
	if s
		PlayerRef.DispelSpell(s)
		PlayerRef.RemoveSpell(s)
	endif
EndFunction

Function SetDifficultyPlayer(float pct)
	float f = pct / 100.0
	Game.SetGameSettingFloat("fDiffMultHPToPCVE", f)
	Game.SetGameSettingFloat("fDiffMultHPToPCE", f)
	Game.SetGameSettingFloat("fDiffMultHPToPCN", f)
	Game.SetGameSettingFloat("fDiffMultHPToPCH", f)
	Game.SetGameSettingFloat("fDiffMultHPToPCVH", f)
	Game.SetGameSettingFloat("fDiffMultHPToPCL", f)
EndFunction

Function SetDifficultyNPC(float pct)
	float f = pct / 100.0
	Game.SetGameSettingFloat("fDiffMultHPByPCVE", f)
	Game.SetGameSettingFloat("fDiffMultHPByPCE", f)
	Game.SetGameSettingFloat("fDiffMultHPByPCN", f)
	Game.SetGameSettingFloat("fDiffMultHPByPCH", f)
	Game.SetGameSettingFloat("fDiffMultHPByPCVH", f)
	Game.SetGameSettingFloat("fDiffMultHPByPCL", f)
EndFunction

Function RestoreDifficultyPlayer()
	Game.SetGameSettingFloat("fDiffMultHPToPCVE", 0.5)
	Game.SetGameSettingFloat("fDiffMultHPToPCE", 0.75)
	Game.SetGameSettingFloat("fDiffMultHPToPCN", 1.0)
	Game.SetGameSettingFloat("fDiffMultHPToPCH", 1.5)
	Game.SetGameSettingFloat("fDiffMultHPToPCVH", 2.0)
	Game.SetGameSettingFloat("fDiffMultHPToPCL", 3.0)
EndFunction

Function RestoreDifficultyNPC()
	Game.SetGameSettingFloat("fDiffMultHPByPCVE", 2.0)
	Game.SetGameSettingFloat("fDiffMultHPByPCE", 1.5)
	Game.SetGameSettingFloat("fDiffMultHPByPCN", 1.0)
	Game.SetGameSettingFloat("fDiffMultHPByPCH", 0.75)
	Game.SetGameSettingFloat("fDiffMultHPByPCVH", 0.5)
	Game.SetGameSettingFloat("fDiffMultHPByPCL", 0.25)
EndFunction

; Liga ou desliga o Inpa e o NahIDWin em conjunto.
Function SetModOn(bool a_on)
	if a_on
		zzzSekiro_MCM_turn.SetValue(1.0)
		NahIDWinNative.IniSetF("General", "Enabled", 1.0)
		; sem "regenerar estamina a bloquear" e sem ressurreicao (removidos do mod)
		zzzSekiro_postureregen.SetValue(0.0)
		zzzSekiro_resurrection.SetValue(0.0)
		Strip(zzzSekiro_regen_blocking_spell)
		Strip(zzzSekiro_zzResurrection_spell)
		Strip(zzzSekiro_cloak_spell)
		Refresh(zzzSekiro_player_main_spell)
		Refresh(zzzSekiro_player_main2_spell)
		Refresh(zzzSekiro_player_main3_spell)
		Refresh(zzzSekiro_player_parrytimer_spell)
		Refresh(zzzSekiro_player_parry_spell)
		Refresh(zzzSekiro_player_parry2_spell)
		Refresh(zzzSekiro_player_parry3_spell)
	else
		zzzSekiro_MCM_turn.SetValue(0.0)
		NahIDWinNative.IniSetF("General", "Enabled", 0.0)
		Strip(zzzSekiro_player_main_spell)
		Strip(zzzSekiro_player_main2_spell)
		Strip(zzzSekiro_player_main3_spell)
		Strip(zzzSekiro_cloak_spell)
		Strip(zzzSekiro_regen_blocking_spell)
		Strip(zzzSekiro_ward_spell)
		Strip(zzzSekiro_player_parrytimer_spell)
		Strip(zzzSekiro_player_parry_spell)
		Strip(zzzSekiro_player_parry2_spell)
		Strip(zzzSekiro_player_parry3_spell)
		Strip(zzzSekiro_zzResurrection_spell)
	endif
EndFunction

; efeitos secundarios de mudar um global do Inpa (iguais aos do menu original)
Function GlobalChanged(int gid, float v)
	if gid == 1
		Refresh(zzzSekiro_player_parrytimer_spell)
	elseif gid == 2
		Refresh(zzzSekiro_player_parry_spell)
		Refresh(zzzSekiro_player_parry2_spell)
	elseif gid == 4 || gid == 5
		Refresh(zzzSekiro_player_main_spell)
		Refresh(zzzSekiro_player_parry2_spell)
	elseif gid >= 22 && gid <= 30
		Game.SetGameSettingFloat(GGmst(gid), v)
		Refresh(zzzSekiro_player_main_spell)
	elseif gid == 31
		if v > 0.5
			SetDifficultyPlayer(GVal(32))
		else
			RestoreDifficultyPlayer()
		endif
	elseif gid == 32
		SetDifficultyPlayer(v)
	elseif gid == 33
		if v > 0.5
			SetDifficultyNPC(GVal(34))
		else
			RestoreDifficultyNPC()
		endif
	elseif gid == 34
		SetDifficultyNPC(v)
	endif
EndFunction

; ---------------- tabela das opcoes .ini ----------------
Function AddSetting(int blk, int ty, string sec, string key, string lpt, string lenn, string ipt, string ien, float mn, float mx, float stp, float df)
	sBlk[count] = blk
	sType[count] = ty
	sSec[count] = sec
	sKey[count] = key
	sLabelPT[count] = lpt
	sLabelEN[count] = lenn
	sInfoPT[count] = ipt
	sInfoEN[count] = ien
	sMin[count] = mn
	sMax[count] = mx
	sStep[count] = stp
	sDef[count] = df
	count += 1
EndFunction

; blocos: 1 Poise, 2 Postura, 3 Furia, 4 Exaustao, 5 Estamina (nossa), 6 Bloqueio dos NPCs, 7 Stagger e empurrao,
;         8 Multiplicadores por arma, 9 Armadura, 10 HUD, 11 Sistema, 12 Bloqueio normal
Function BuildTable()
	if count > 0
		return
	endif
	sBlk = new int[128]
	sType = new int[128]
	sSec = new string[128]
	sKey = new string[128]
	sLabelPT = new string[128]
	sLabelEN = new string[128]
	sInfoPT = new string[128]
	sInfoEN = new string[128]
	sMin = new float[128]
	sMax = new float[128]
	sStep = new float[128]
	sDef = new float[128]
	count = 0
	; 0 = slider, 1 = interruptor
	AddSetting(1, 0, "Poise", "MaxScalePlayer", "Poise maximo - jogador (escala)", "Max poise - player (scale)", "Tamanho da barra de poise do jogador. Mais alto = aguenta mais golpes.", "Size of the player's poise bar. Higher = survives more hits.", 0.5, 20.0, 0.1, 4.5)
	AddSetting(1, 0, "Poise", "MaxScaleNPC", "Poise maximo - NPCs (escala)", "Max poise - NPCs (scale)", "Tamanho da barra de poise dos NPCs e criaturas.", "Size of the poise bar of NPCs and creatures.", 0.5, 30.0, 0.5, 9.0)
	AddSetting(1, 0, "Poise", "NPCTakenMult", "Dano de poise aos NPCs (x)", "Poise damage to NPCs (x)", "Quanto poise e postura os NPCs perdem por golpe.", "How much poise and posture NPCs lose per hit.", 0.5, 10.0, 0.1, 2.5)
	AddSetting(1, 0, "Poise", "PlayerTakenMult", "Dano de poise ao jogador (x)", "Poise damage to player (x)", "Quanto poise e postura o jogador perde por golpe.", "How much poise and posture the player loses per hit.", 0.5, 10.0, 0.1, 1.5)
	AddSetting(1, 0, "Poise", "RegenSeconds", "Regeneracao do poise (s)", "Poise regeneration (s)", "Segundos para o poise encher de zero a cheio.", "Seconds for poise to refill from zero to full.", 5.0, 120.0, 5.0, 30.0)
	AddSetting(1, 0, "Poise", "RegenDelaySeconds", "Pausa antes de regenerar (s)", "Delay before regenerating (s)", "Segundos sem levar golpes antes de comecar a regenerar.", "Seconds without being hit before regeneration starts.", 0.0, 10.0, 0.5, 2.0)
	AddSetting(1, 0, "Damage", "BlockedMult", "Poise gasto ao bloquear (x)", "Poise spent when blocking (x)", "Fracao do dano de poise que um golpe bloqueado causa.", "Fraction of poise damage a blocked hit causes.", 0.0, 1.0, 0.01, 0.15)
	AddSetting(1, 0, "Stagger", "ImmuneSeconds", "Imunidade apos stagger pesado (s)", "Immunity after heavy stagger (s)", "Depois de um stagger pesado, poise e postura nao sobem durante este tempo.", "After a heavy stagger, poise and posture do not rise for this long.", 0.0, 8.0, 0.5, 2.5)
	AddSetting(2, 1, "Posture", "Enabled", "Postura ligada", "Posture enabled", "Liga a barra de postura (separada da estamina). Quando enche: BleedOut.", "Enables the posture bar (separate from stamina). When it fills: BleedOut.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 0, "Posture", "Difficulty", "DIFICULDADE da postura", "Posture DIFFICULTY", "1.0 normal. 2.0 = a postura aguenta o dobro. 0.5 = quebra com metade.", "1.0 normal. 2.0 = posture lasts twice as long. 0.5 = breaks with half.", 0.2, 10.0, 0.1, 1.0)
	AddSetting(2, 0, "Posture", "ScalePlayer", "Postura - jogador (escala)", "Posture - player (scale)", "Tamanho da barra de postura do jogador.", "Size of the player's posture bar.", 1.0, 20.0, 0.5, 5.0)
	AddSetting(2, 0, "Posture", "ScaleNPC", "Postura - NPCs (escala)", "Posture - NPCs (scale)", "Tamanho da barra de postura dos NPCs.", "Size of the NPCs' posture bar.", 1.0, 20.0, 0.5, 5.0)
	AddSetting(2, 0, "Posture", "DamageMult", "Dano de postura (x)", "Posture damage (x)", "Quanto cada golpe enche a postura.", "How much each hit fills posture.", 0.1, 5.0, 0.1, 1.0)
	AddSetting(2, 0, "Posture", "BlockedMult", "Postura gasta ao bloquear (x)", "Posture spent when blocking (x)", "Fracao do dano de postura de um golpe bloqueado.", "Fraction of posture damage of a blocked hit.", 0.0, 1.0, 0.01, 0.12)
	AddSetting(2, 0, "Posture", "RegenSeconds", "Regeneracao da postura (s)", "Posture regeneration (s)", "Segundos para a postura baixar de cheia a zero.", "Seconds for posture to drop from full to zero.", 10.0, 300.0, 5.0, 60.0)
	AddSetting(2, 0, "Posture", "LowHealthBonus", "Bonus com vida baixa", "Low health bonus", "Com a vida baixa a postura enche mais depressa. 0 = desligado.", "With low health posture fills faster. 0 = off.", 0.0, 3.0, 0.1, 1.0)
	AddSetting(2, 0, "Posture", "BleedOutSeconds", "Duracao do BleedOut (s)", "BleedOut duration (s)", "Tempo ajoelhado quando a postura quebra.", "Time kneeling when posture breaks.", 1.0, 15.0, 0.5, 4.0)
	AddSetting(2, 0, "Posture", "BleedOutImmuneSeconds", "Imunidade apos BleedOut (s)", "Immunity after BleedOut (s)", "Tempo sem poder voltar a quebrar depois de levantar.", "Time you cannot break again after getting up.", 0.0, 15.0, 0.5, 5.0)
	AddSetting(3, 1, "Rage", "Enabled", "Furia ligada", "Rage enabled", "Imunidade temporaria em troca de vida ou estamina.", "Temporary stagger immunity and damage reduction at a cost.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(3, 0, "Rage", "Key", "Tecla da furia (codigo)", "Rage key (code)", "Codigo DirectInput. 34 = G, 19 = R, 47 = V, 46 = C.", "DirectInput code. 34 = G, 19 = R, 47 = V, 46 = C.", 1.0, 255.0, 1.0, 34.0)
	AddSetting(3, 0, "Rage", "DurationSeconds", "Duracao da furia (s)", "Rage duration (s)", "Tempo de imunidade.", "How long rage lasts.", 1.0, 20.0, 0.5, 5.0)
	AddSetting(3, 0, "Rage", "CooldownSeconds", "Recarga da furia (s)", "Rage cooldown (s)", "Tempo ate poderes usar outra vez.", "Time until you can use it again.", 5.0, 180.0, 5.0, 30.0)
	AddSetting(3, 0, "Rage", "CostType", "Custo: 0 vida, 1 estamina", "Cost: 0 health, 1 stamina", "Com o que pagas a furia.", "What you pay the rage with.", 0.0, 1.0, 1.0, 0.0)
	AddSetting(3, 0, "Rage", "CostPercent", "Custo (% do maximo)", "Cost (% of max)", "Percentagem do maximo que a furia custa. A vida nunca te mata.", "Percent of max that rage costs. Health never kills you.", 0.0, 60.0, 1.0, 15.0)
	AddSetting(3, 0, "Rage", "DamageReductionPercent", "Reducao de dano - jogador (%)", "Player damage reduction (%)", "Reducao do dano de vida enquanto o jogador esta em furia.", "Health damage reduction while the player is raging.", 0.0, 95.0, 5.0, 70.0)
	AddSetting(3, 0, "Rage", "NPCDamageReductionPercent", "Reducao de dano - NPC (%)", "NPC damage reduction (%)", "Reducao do dano de vida enquanto um NPC esta em furia.", "Health damage reduction while an NPC is raging.", 0.0, 95.0, 5.0, 50.0)
	AddSetting(3, 1, "Rage", "ZeroStamina", "Furia sem custo de estamina", "Zero stamina cost in rage", "Durante a furia os golpes nao gastam estamina (so vida).", "During rage your swings cost no stamina (only health).", 0.0, 1.0, 1.0, 1.0)
	AddSetting(3, 1, "Rage", "NPCs", "NPCs usam furia", "NPCs use rage", "NPCs ativam a furia sozinhos quando perto de quebrar.", "NPCs trigger rage on their own when close to breaking.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(3, 0, "Rage", "NPCPostureTrigger", "NPC: postura acima de", "NPC: posture above", "O NPC ativa quando a postura passa desta fracao.", "The NPC triggers when posture passes this fraction.", 0.3, 1.0, 0.05, 0.8)
	AddSetting(3, 0, "Rage", "NPCPoiseTrigger", "NPC: poise abaixo de", "NPC: poise below", "... ou quando o poise desce abaixo desta fracao.", "... or when poise drops below this fraction.", 0.0, 0.6, 0.05, 0.15)
	AddSetting(3, 0, "Rage", "NPCChancePercent", "NPC: probabilidade (%)", "NPC: chance (%)", "Probabilidade por golpe recebido quando a condicao se cumpre.", "Chance per hit received when the condition is met.", 0.0, 100.0, 5.0, 50.0)
	AddSetting(3, 0, "Rage", "NPCCooldownSeconds", "NPC: recarga (s)", "NPC: cooldown (s)", "Tempo minimo entre duas furias do mesmo NPC.", "Minimum time between two rages of the same NPC.", 5.0, 180.0, 5.0, 45.0)
	AddSetting(3, 0, "Rage", "NPCCostPercent", "NPC: custo (%)", "NPC: cost (%)", "0 = gratis para os NPCs.", "0 = free for NPCs.", 0.0, 50.0, 1.0, 0.0)
	AddSetting(4, 1, "Exhaustion", "Enabled", "Exaustao ligada", "Exhaustion enabled", "Estamina a zero: nao podes atacar nem regenerar por um tempo.", "Stamina at zero: you cannot attack or regenerate for a while.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(4, 0, "Exhaustion", "Seconds", "Duracao da exaustao (s)", "Exhaustion duration (s)", "Tempo sem poder atacar e sem regenerar.", "Time without attacking and without regenerating.", 0.5, 10.0, 0.5, 2.5)
	AddSetting(4, 0, "Exhaustion", "CooldownSeconds", "Recarga da exaustao (s)", "Exhaustion cooldown (s)", "Tempo minimo entre duas exaustoes.", "Minimum time between two exhaustions.", 0.0, 30.0, 1.0, 8.0)
	AddSetting(4, 1, "Exhaustion", "NPCs", "Exaustao nos NPCs", "Exhaustion on NPCs", "NPCs sem estamina levam stagger pesado.", "NPCs with no stamina get a heavy stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(5, 1, "Stamina", "Enabled", "Custo de estamina por golpe", "Stamina cost per swing", "Liga o custo de estamina por golpe e a influencia da armadura.", "Enables stamina cost per swing and armor influence.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(5, 0, "Stamina", "SwingCostPlayer", "Custo base por golpe - jogador", "Base cost per swing - player", "Estamina gasta por cada balanco de arma (mesmo sem acertar).", "Stamina spent by each weapon swing (even when missing).", 0.0, 40.0, 1.0, 8.0)
	AddSetting(5, 0, "Stamina", "HitCostNPC", "Custo por golpe - NPCs", "Cost per swing - NPCs", "0 = deixa-se aos mods de estamina. Senao a DLL tira isto por golpe que o NPC acerta.", "0 = left to stamina mods. Otherwise the DLL takes this per hit the NPC lands.", 0.0, 40.0, 1.0, 0.0)
	AddSetting(5, 0, "Stamina", "PowerAttackOwnCost", "Golpe forte: custo extra do mod (x)", "Power attack: extra mod cost (x)", "0 = so o custo do jogo (PowerAttack Cost Mult acima). Acima de 0 o mod cobra tambem este multiplo do custo base.", "0 = only the game cost (PowerAttack Cost Mult above). Above 0 the mod also charges this multiple of the base cost.", 0.0, 4.0, 0.1, 0.0)
	AddSetting(5, 0, "Stamina", "BashMult", "Golpe de escudo (x)", "Shield bash (x)", "Multiplicador do custo em golpe de escudo.", "Cost multiplier for shield bashes.", 0.0, 4.0, 0.1, 1.0)
	AddSetting(6, 0, "NpcBlock", "ChanceMin", "Chance minima de bloqueio dos NPCs", "NPC minimum block chance", "Quanto os NPCs bloqueiam. O Inpa deixa 0.60 (muito). Mais baixo = bloqueiam e fazem parry menos vezes. Nao muda quantos NPCs podem bloquear.", "How often NPCs block. Inpa uses 0.60 (a lot). Lower = they block and parry less often. Does not change how many NPCs can block.", 0.0, 1.0, 0.05, 0.3)
	AddSetting(7, 1, "Stagger", "LightEnabled", "Stagger leve ligado", "Light stagger enabled", "Golpe normal sem bloquear da stagger leve.", "A normal unblocked hit causes a light stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 1, "Stagger", "LightPlayer", "Stagger leve no jogador", "Light stagger on player", "Desligado = o jogador nao e travado por golpes leves.", "Off = the player is not locked by light hits.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 1, "Stagger", "LightNPCs", "Stagger leve nos NPCs", "Light stagger on NPCs", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 0, "Stagger", "LightCooldownMs", "Espaco do stagger leve - NPCs (ms)", "Light stagger spacing - NPCs (ms)", "Tempo minimo entre dois staggers leves.", "Minimum time between two light staggers.", 100.0, 3000.0, 100.0, 700.0)
	AddSetting(7, 0, "Stagger", "LightCooldownPlayerMs", "Espaco do stagger leve - jogador (ms)", "Light stagger spacing - player (ms)", "Mais espaco = menos tempo travado a levar golpes.", "More spacing = less time locked while being hit.", 100.0, 5000.0, 100.0, 1500.0)
	AddSetting(7, 1, "Stagger", "MediumOnPowerAttack", "Stagger medio em golpe forte", "Medium stagger on power attack", "Golpe forte ou de escudo sem bloquear da stagger medio.", "An unblocked power attack or bash causes a medium stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 1, "Knockback", "Enabled", "Empurrao ligado", "Knockback enabled", "Empurrao no stagger pesado, so em condicoes especificas.", "Knockback on heavy stagger, only in specific conditions.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 0, "Knockback", "Force", "Forca do empurrao", "Knockback force", "0 = sem empurrao.", "0 = no knockback.", 0.0, 5.0, 0.1, 1.0)
	AddSetting(7, 1, "Knockback", "Player", "Empurrar o jogador", "Knock back the player", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 1, "Knockback", "OnPowerAttack", "Quando golpe forte", "On power attack", "Inclui o carregado e o de corrida.", "Includes charged and sprinting attacks.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 1, "Knockback", "OnGiantCreature", "Quando criatura gigante", "On giant creature", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(7, 0, "Knockback", "GiantMassMin", "Massa minima de gigante", "Giant minimum mass", "Massa da raca a partir da qual conta como gigante.", "Race mass from which it counts as giant.", 1.0, 10.0, 0.5, 3.0)
	AddSetting(7, 1, "Knockback", "OnHeavyWeapon", "Quando arma pesada (golpe normal)", "On heavy weapon (normal hit)", "Cuidado: deixa as armas grandes muito fortes.", "Careful: makes big weapons very strong.", 0.0, 1.0, 1.0, 0.0)
	AddSetting(7, 0, "Knockback", "HeavyWeaponMinWeight", "Peso minimo de arma pesada", "Heavy weapon minimum weight", "", "", 5.0, 40.0, 1.0, 15.0)
	AddSetting(8, 0, "Stamina", "TypeMult0", "Custo: Punhos (x)", "Cost: Fists (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.6)
	AddSetting(8, 0, "Stamina", "TypeMult1", "Custo: Espada 1 mao (x)", "Cost: 1H Sword (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.0)
	AddSetting(8, 0, "Stamina", "TypeMult2", "Custo: Adaga (x)", "Cost: Dagger (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.6)
	AddSetting(8, 0, "Stamina", "TypeMult3", "Custo: Machado 1 mao (x)", "Cost: 1H Axe (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.2)
	AddSetting(8, 0, "Stamina", "TypeMult4", "Custo: Maca 1 mao (x)", "Cost: 1H Mace (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.3)
	AddSetting(8, 0, "Stamina", "TypeMult5", "Custo: Espada 2 maos (x)", "Cost: 2H Sword (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.6)
	AddSetting(8, 0, "Stamina", "TypeMult6", "Custo: Machado/Martelo 2 maos (x)", "Cost: 2H Axe/Hammer (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.8)
	AddSetting(8, 0, "Stamina", "TypeMult7", "Custo: Arco (x)", "Cost: Bow (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.8)
	AddSetting(8, 0, "Stamina", "TypeMult8", "Custo: Cajado (x)", "Cost: Staff (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.6)
	AddSetting(8, 0, "Stamina", "TypeMult9", "Custo: Feitico (x)", "Cost: Spell (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.0)
	AddSetting(8, 0, "Stamina", "TypeMult10", "Custo: Escudo (x)", "Cost: Shield (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.0)
	AddSetting(8, 0, "Stamina", "TypeMult11", "Custo: Tocha (x)", "Cost: Torch (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.5)
	AddSetting(8, 0, "Stamina", "TypeMult12", "Custo: Besta (x)", "Cost: Crossbow (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.8)
	AddSetting(9, 0, "Stamina", "ArmorCostFactor", "Armadura aumenta o custo", "Armor raises the cost", "Custo x (1 + armadura/100 x isto). Armadura pesada cansa mais.", "Cost x (1 + armor/100 x this). Heavy armor tires faster.", 0.0, 2.0, 0.05, 0.25)
	AddSetting(9, 1, "Stamina", "ArmorRegenEnabled", "Armadura abranda a regeneracao", "Armor slows regeneration", "A armadura reduz a regeneracao de estamina do jogador. Desliga se achas a regeneracao lenta.", "Armor reduces the player's stamina regeneration. Turn off if regeneration feels slow.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(9, 0, "Stamina", "ArmorRegenFactor", "Intensidade da regeneracao", "Regeneration strength", "Regeneracao % = 100 / (1 + armadura/100 x isto).", "Regeneration % = 100 / (1 + armor/100 x this).", 0.0, 2.0, 0.05, 0.3)
	AddSetting(10, 0, "TrueHUD", "Bar", "Barra do TrueHUD", "TrueHUD bar", "0 = poise, 1 = postura (recomendado), 2 = risco (a mais perto de quebrar).", "0 = poise, 1 = posture, 2 = risk (whichever is closest to breaking).", 0.0, 2.0, 1.0, 1.0)
	AddSetting(10, 0, "TrueHUD", "PoiseWarnFraction", "Aviso de poise baixo (fracao)", "Low poise warning (fraction)", "A barra pisca quando o poise desce abaixo disto. 0 = desligado.", "The bar flashes when poise drops below this. 0 = off.", 0.0, 0.8, 0.05, 0.3)
	AddSetting(10, 1, "TrueHUD", "FlashOnBreak", "Barra pisca ao quebrar", "Bar flashes on break", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(11, 1, "General", "Player", "Poise e postura no jogador", "Poise and posture on player", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(11, 1, "General", "NPCs", "Poise e postura nos NPCs", "Poise and posture on NPCs", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(11, 1, "Diagnostics", "Enabled", "Diagnostico no log", "Diagnostics in log", "Escreve o que acontece em NahIDWin.log.", "Writes what happens to NahIDWin.log.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(11, 1, "Diagnostics", "LogPlayerEvents", "Registar eventos que afetam o jogador", "Log events hitting the player", "Ajuda a descobrir a queda a meio dos combos.", "Helps find the fall during combos.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(12, 1, "Block", "DrainEnabled", "Bloqueio gasta estamina do atacante", "Block drains attacker stamina", "Bloqueio normal (sem parry) gasta estamina do atacante. Sem stagger.", "Normal block (no parry) drains the attacker's stamina. No stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(12, 0, "Block", "AttackerStamina", "Estamina do atacante por golpe", "Attacker stamina per hit", "Estamina que o atacante perde por golpe bloqueado.", "Stamina the attacker loses per blocked hit.", 0.0, 60.0, 1.0, 10.0)
	AddSetting(12, 0, "Block", "PowerAttackMult", "Golpe forte bloqueado (x)", "Blocked power attack (x)", "Multiplicador desse custo em golpes fortes.", "Multiplier of that cost for power attacks.", 0.5, 4.0, 0.1, 1.5)
EndFunction

string Function FormatFor(int i)
	if sStep[i] >= 1.0
		return "{0}"
	endif
	return "{2}"
EndFunction

string Function LabelOf(int i)
	if IsPT()
		return sLabelPT[i]
	endif
	return sLabelEN[i]
EndFunction

; ---------------- desenho ----------------
int Function Track(int a_option, int a_kind, int a_ref)
	if a_option >= 0
		int slot = a_option % 128
		oKind[slot] = a_kind
		oRef[slot] = a_ref
	endif
	return a_option
EndFunction

Function Hdr(string en, string pt)
	AddHeaderOption(Orange(Tr(en, pt)), 0)
EndFunction

Function GTog(int gid, string en, string pt)
	Track(AddToggleOption(Tr(en, pt), GVal(gid) > 0.5, sFlag), 4, gid)
EndFunction

Function GSli(int gid, string en, string pt)
	Track(AddSliderOption(Tr(en, pt), GVal(gid), GFmt(gid), sFlag), 5, gid)
EndFunction

string Function KillName(int v)
	if v == 0
		return Tr("Increased Damage X5", "Dano Aumentado X5")
	elseif v == 3
		return Tr("Only Script Animation", "So animacao")
	endif
	return Tr("Only Onekill Damage", "So dano de morte")
EndFunction

; 0 = Dano Aumentado X5, 3 = animacao, 4 = dano de morte (os modos do VioLens ficam de fora)
int Function KillIndex(int v)
	if v == 0
		return 0
	elseif v == 4
		return 2
	endif
	return 1
EndFunction

int Function KillValue(int idx)
	if idx == 0
		return 0
	elseif idx == 2
		return 4
	endif
	return 3
EndFunction

string Function SneakName(int v)
	if v == 0
		return Tr("Off", "Desligado")
	elseif v == 1
		return Tr("Increased Damage", "Dano Aumentado")
	endif
	return Tr("Script Animation", "Animacao")
EndFunction

Function DrawBlock(int blk, string en, string pt)
	Hdr(en, pt)
	int i = 0
	while i < count
		if sBlk[i] == blk
			float cur = NahIDWinNative.IniGetF(sSec[i], sKey[i], sDef[i])
			if sType[i] == 1
				Track(AddToggleOption(LabelOf(i), cur > 0.5, sFlag), 2, i)
			else
				Track(AddSliderOption(LabelOf(i), cur, FormatFor(i), sFlag), 3, i)
			endif
		endif
		i += 1
	endWhile
EndFunction

Event OnPageReset(string a_page)
	if a_page == ""
		sPageNo = -1
		return
	endif
	UnloadCustomContent()
	BuildTable()
	oKind = new int[128]
	oRef = new int[128]
	if ModOn()
		sFlag = OPTION_FLAG_NONE
	else
		sFlag = OPTION_FLAG_DISABLED
	endif
	SetCursorFillMode(TOP_TO_BOTTOM)
	if a_page == Pages[0]
		sPageNo = 0
		DrawPage0()
	elseif a_page == Pages[1]
		sPageNo = 1
		DrawPage1()
	else
		sPageNo = 2
		DrawPage2()
	endif
EndEvent

Function DrawPage0()
	SetCursorPosition(0)
	Track(AddToggleOption("<font color=\"#d9b111\">MOD On/Off</font>", ModOn(), 0), 1, 0)
	int li = AddToggleOption(Tr("Language: Portugues (BR)", "Idioma: Portugues (BR)"), IsPT(), 0)
	Track(li, 7, 0)
	Hdr("Parry Setting", "Parry")
	GSli(1, "Parry & Deflect Timer", "Janela de Parry e Deflexao")
	GSli(2, "Stamina Recovery", "Recuperar Estamina no Parry")
	GTog(3, "Magic Reflect", "Refletir Magia")
	Hdr("Block Setting", "Bloqueio")
	GTog(4, "Block Arrow", "Bloquear Flechas")
	GTog(5, "Block Magic", "Bloquear Magia")
	GTog(6, "Resist Melee Damage", "Resistir Dano Corpo a Corpo")
	GTog(7, "Resist Poison/Disease Damage", "Resistir Veneno/Doenca")
	Hdr("Execution", "Execucao")
	int a = AddMenuOption(Tr("Shinobi Execution", "Execucao Shinobi"), KillName(KillValue(KillIndex(GVal(19) as int))), sFlag)
	Track(a, 6, 19)
	int b = AddMenuOption(Tr("(beta) Sneak Execution", "(beta) Execucao Furtiva"), SneakName(GVal(20) as int), sFlag)
	Track(b, 6, 20)
	GSli(21, "Sneak Execution Damage Mult", "Dano da Execucao Furtiva")
	Hdr("Player Plus", "Jogador Plus")
	GTog(8, "Enemy Alert", "Alerta de Inimigo")
	Hdr("NPC Plus", "NPC Plus")
	GTog(9, "NPC Block Arrow", "NPC Bloqueia Flechas")
	GTog(10, "NPC Block Magic", "NPC Bloqueia Magia")
	GTog(11, "NPC Fist Block", "NPC Bloqueia com Punhos")
	GTog(12, "Enemy Frost Resist", "Inimigos Resistem a Gelo")
	GTog(13, "Enemy Stagger to Magic", "Inimigos Cambaleiam com Magia")
	GSli(14, "Perilous Attacks", "Ataques Perigosos")
	GSli(15, "Dragon Stamina Increase", "Estamina dos Dragoes")
	Hdr("ALLY Plus", "Aliados Plus")
	GTog(16, "Protect Ally From Execution", "Proteger Aliados de Execucao")
	GTog(17, "Reduce Ally's Stamina Damage", "Reduzir Dano de Estamina dos Aliados")
	GTog(18, "Ally Can't Stagger Enemies", "Aliados Nao Fazem Stagger")
	SetCursorPosition(1)
	DrawBlock(1, "Poise", "Poise")
	DrawBlock(2, "Posture", "Postura")
	DrawBlock(3, "Rage", "Furia")
	DrawBlock(4, "Exhaustion", "Exaustao")
EndFunction

Function DrawPage1()
	SetCursorPosition(0)
	Hdr("Stamina Game Setting", "Estamina (Game Setting)")
	GSli(22, "PowerAttack Cost Mult", "Custo do Golpe Forte (x)")
	GSli(23, "Sprint Stamina Drain Base", "Gasto Base da Corrida")
	GSli(24, "Sprint Stamina Drain Mult", "Gasto da Corrida por Peso")
	GSli(25, "Combat Stamina Regen", "Regeneracao de Estamina em Combate")
	GSli(26, "Stamina Regen Delay", "Pausa da Regeneracao de Estamina")
	DrawBlock(5, "Stamina per Swing", "Estamina por Golpe")
	Hdr("Resist Game Setting", "Resistencias (Game Setting)")
	GSli(27, "Maximum Resistance", "Resistencia Maxima")
	GSli(28, "Maximum Armor Resistance", "Resistencia Maxima da Armadura")
	GSli(29, "Shield Block Base", "Base do Bloqueio com Escudo")
	GSli(30, "Weapon Block Base", "Base do Bloqueio com Arma")
	SetCursorPosition(1)
	Hdr("Difficulty Game Setting", "Dificuldade (Game Setting)")
	GTog(31, "Player Difficulty Change", "Mudar Dificuldade do Jogador")
	GSli(32, "Player taken damage mult", "Dano que o Jogador Recebe")
	GTog(33, "NPC Difficulty Change", "Mudar Dificuldade dos NPCs")
	GSli(34, "NPC taken damage mult", "Dano que os NPCs Recebem")
	DrawBlock(6, "NPC Block Frequency", "Frequencia de Bloqueio dos NPCs")
	DrawBlock(7, "Stagger and Knockback", "Stagger e Empurrao")
EndFunction

Function DrawPage2()
	SetCursorPosition(0)
	DrawBlock(8, "Stamina cost per weapon type", "Custo de estamina por tipo de arma")
	DrawBlock(9, "Armor", "Armadura")
	SetCursorPosition(1)
	DrawBlock(12, "Normal block (ours)", "Bloqueio normal (nosso)")
	DrawBlock(10, "HUD (TrueHUD bar)", "HUD (barra do TrueHUD)")
	DrawBlock(11, "System", "Sistema")
EndFunction

; ---------------- eventos ----------------
Event OnOptionSelect(int a_option)
	int slot = a_option % 128
	int k = oKind[slot]
	int r = oRef[slot]
	if k == 1
		bool turnOn = !ModOn()
		SetToggleOptionValue(a_option, turnOn)
		SetModOn(turnOn)
		ForcePageReset()
	elseif k == 7
		float nv = 1.0
		if IsPT()
			nv = 0.0
		endif
		NahIDWinNative.IniSetF("Menu", "Language", nv)
		ForcePageReset()
	elseif k == 2
		float cur = NahIDWinNative.IniGetF(sSec[r], sKey[r], sDef[r])
		float nv2 = 1.0
		if cur > 0.5
			nv2 = 0.0
		endif
		NahIDWinNative.IniSetF(sSec[r], sKey[r], nv2)
		SetToggleOptionValue(a_option, nv2 > 0.5)
	elseif k == 4
		float g = 1.0
		if GVal(r) > 0.5
			g = 0.0
		endif
		GetG(r).SetValue(g)
		SetToggleOptionValue(a_option, g > 0.5)
		GlobalChanged(r, g)
	endif
EndEvent

Event OnOptionDefault(int a_option)
	int slot = a_option % 128
	int k = oKind[slot]
	int r = oRef[slot]
	if k == 2
		NahIDWinNative.IniSetF(sSec[r], sKey[r], sDef[r])
		SetToggleOptionValue(a_option, sDef[r] > 0.5)
	elseif k == 3
		NahIDWinNative.IniSetF(sSec[r], sKey[r], sDef[r])
		SetSliderOptionValue(a_option, sDef[r], FormatFor(r))
	elseif k == 5
		GetG(r).SetValue(GDef(r))
		SetSliderOptionValue(a_option, GDef(r), GFmt(r))
		GlobalChanged(r, GDef(r))
	endif
EndEvent

Event OnOptionSliderOpen(int a_option)
	int slot = a_option % 128
	int k = oKind[slot]
	int r = oRef[slot]
	if k == 3
		SetSliderDialogStartValue(NahIDWinNative.IniGetF(sSec[r], sKey[r], sDef[r]))
		SetSliderDialogDefaultValue(sDef[r])
		SetSliderDialogRange(sMin[r], sMax[r])
		SetSliderDialogInterval(sStep[r])
	elseif k == 5
		SetSliderDialogStartValue(GVal(r))
		SetSliderDialogDefaultValue(GDef(r))
		SetSliderDialogRange(GMin(r), GMax(r))
		SetSliderDialogInterval(GStep(r))
	endif
EndEvent

Event OnOptionSliderAccept(int a_option, float a_value)
	int slot = a_option % 128
	int k = oKind[slot]
	int r = oRef[slot]
	if k == 3
		NahIDWinNative.IniSetF(sSec[r], sKey[r], a_value)
		SetSliderOptionValue(a_option, a_value, FormatFor(r))
	elseif k == 5
		GetG(r).SetValue(a_value)
		SetSliderOptionValue(a_option, a_value, GFmt(r))
		GlobalChanged(r, a_value)
	endif
EndEvent

Event OnOptionMenuOpen(int a_option)
	int slot = a_option % 128
	int r = oRef[slot]
	if oKind[slot] != 6
		return
	endif
	if r == 19
		string[] opts = new string[3]
		opts[0] = KillName(0)
		opts[1] = KillName(3)
		opts[2] = KillName(4)
		SetMenuDialogOptions(opts)
		SetMenuDialogStartIndex(KillIndex(GVal(19) as int))
		SetMenuDialogDefaultIndex(1)
	elseif r == 20
		string[] opts2 = new string[3]
		opts2[0] = SneakName(0)
		opts2[1] = SneakName(1)
		opts2[2] = SneakName(2)
		SetMenuDialogOptions(opts2)
		SetMenuDialogStartIndex(GVal(20) as int)
		SetMenuDialogDefaultIndex(0)
	endif
EndEvent

Event OnOptionMenuAccept(int a_option, int a_index)
	int slot = a_option % 128
	int r = oRef[slot]
	if oKind[slot] != 6 || a_index < 0
		return
	endif
	if r == 19
		int v = KillValue(a_index)
		GetG(19).SetValue(v as float)
		SetMenuOptionValue(a_option, KillName(v))
	elseif r == 20
		GetG(20).SetValue(a_index as float)
		SetMenuOptionValue(a_option, SneakName(a_index))
	endif
EndEvent

string Function GInfo(int gid)
	if gid == 1
		return Tr("Deflect timer when you are blocking.", "Janela de parry/deflexao quando comecas a bloquear.")
	elseif gid == 2
		return Tr("Stamina recovered when you parry or deflect.", "Estamina que recuperas ao fazer parry ou deflexao.")
	elseif gid == 3
		return Tr("Reflect magic back at the caster.", "Reflete a magia de volta para quem a lancou.")
	elseif gid == 4
		return Tr("Set whether the player can block arrows.", "Define se o jogador pode bloquear flechas.")
	elseif gid == 5
		return Tr("Set whether the player can block and deflect spells.", "Define se o jogador pode bloquear e defletir magias.")
	elseif gid == 6
		return Tr("No melee damage while blocking, like Sekiro. Level-difference damage cannot be fully blocked.", "Sem dano corpo a corpo ao bloquear, como no Sekiro.")
	elseif gid == 7
		return Tr("Resist poison and disease while blocking.", "Resiste a veneno e doenca ao bloquear.")
	elseif gid == 8
		return Tr("Enemy alert sound when the player is in combat.", "Som de alerta de inimigo quando estas em combate.")
	elseif gid == 9
		return Tr("Set whether enemies can block and deflect arrows.", "Define se os inimigos bloqueiam flechas.")
	elseif gid == 10
		return Tr("Set whether enemies can block magic (with enchanted weapon or shield).", "Define se os inimigos bloqueiam magia (com arma ou escudo encantados).")
	elseif gid == 11
		return Tr("Set whether enemies can block with fists.", "Define se os inimigos bloqueiam com os punhos.")
	elseif gid == 12
		return Tr("Gives enemies 50% frost resistance, like Nord.", "Da 50% de resistencia a gelo aos inimigos, como os Nord.")
	elseif gid == 13
		return Tr("Enemies get staggered by magic like the player.", "Os inimigos cambaleiam com magia como o jogador.")
	elseif gid == 14
		return Tr("Random chance of enemy Perilous Attacks. 0 = off.", "Probabilidade de ataques perigosos dos inimigos. 0 = desligado.")
	elseif gid == 15
		return Tr("Multiplies the stamina of dragons.", "Multiplica a estamina dos dragoes.")
	elseif gid == 16
		return Tr("Protect your allies from execution.", "Protege os teus aliados de execucao.")
	elseif gid == 17
		return Tr("Allies take half stamina damage.", "Os aliados levam metade do dano de estamina.")
	elseif gid == 18
		return Tr("Allies cannot stagger enemies.", "Os aliados nao fazem stagger aos inimigos.")
	elseif gid == 19
		return Tr("Execution when an enemy is in posture break. The VioLens modes are not available.", "Execucao quando o inimigo esta com a postura quebrada. Os modos do VioLens nao existem.")
	elseif gid == 20
		return Tr("Backstab execution while sneaking behind an enemy.", "Execucao pelas costas enquanto furtivo.")
	elseif gid == 21
		return Tr("Damage multiplier of the sneak execution.", "Multiplicador de dano da execucao furtiva.")
	elseif gid == 22
		return Tr("Game setting (vanilla 2): stamina cost multiplier of power attacks, not bashes.", "Game Setting (vanilla 2): multiplicador do custo de estamina dos golpes fortes, nao dos de escudo.")
	elseif gid == 23
		return Tr("Game setting: base stamina drain of sprinting.", "Game Setting: gasto base de estamina ao correr.")
	elseif gid == 24
		return Tr("Game setting: sprint stamina drain per armor weight.", "Game Setting: gasto de estamina da corrida por peso da armadura.")
	elseif gid == 25
		return Tr("Game setting: stamina regeneration rate while in combat (Inpa uses 0.35, vanilla 0.35). Raise it if stamina feels slow to recover.", "Game Setting: velocidade da regeneracao de estamina em combate. Sobe se a estamina demorar a recuperar.")
	elseif gid == 26
		return Tr("Game setting: seconds before stamina starts to regenerate after being spent (vanilla 5). Lower = recovers sooner.", "Game Setting: segundos ate a estamina comecar a regenerar depois de gasta (vanilla 5). Mais baixo = recupera mais cedo.")
	elseif gid == 27
		return Tr("Game setting: maximum possible damage reduction from resistances, blocking, etc.", "Game Setting: reducao maxima de dano por resistencias, bloqueio, etc.")
	elseif gid == 28
		return Tr("Game setting: maximum damage reduction given by armor rating.", "Game Setting: reducao maxima de dano dada pela armadura.")
	elseif gid == 29
		return Tr("Game setting: base of shield blocking.", "Game Setting: base do bloqueio com escudo.")
	elseif gid == 30
		return Tr("Game setting: base of weapon blocking.", "Game Setting: base do bloqueio com arma.")
	elseif gid == 31
		return Tr("Override the vanilla difficulty damage taken by the player.", "Substitui a dificuldade do jogo para o dano que o jogador recebe.")
	elseif gid == 32
		return Tr("How much damage the player takes.", "Quanto dano o jogador recebe.")
	elseif gid == 33
		return Tr("Override the vanilla difficulty damage taken by enemies.", "Substitui a dificuldade do jogo para o dano que os inimigos recebem.")
	elseif gid == 34
		return Tr("How much damage enemies take.", "Quanto dano os inimigos recebem.")
	endif
	return ""
EndFunction

Event OnOptionHighlight(int a_option)
	int slot = a_option % 128
	int k = oKind[slot]
	int r = oRef[slot]
	if k == 1
		SetInfoText(Tr("Turns Inpa Sekiro and NahIDWin on/off together. Do it out of combat.", "Liga/desliga o Inpa Sekiro e o NahIDWin em conjunto. Faz isto fora de combate."))
	elseif k == 7
		SetInfoText(Tr("Switch between English and Portugues (BR).", "Alterna entre English e Portugues (BR)."))
	elseif k == 2 || k == 3
		SetInfoText(Tr(sInfoEN[r], sInfoPT[r]))
	elseif k == 4 || k == 5 || k == 6
		SetInfoText(GInfo(r))
	endif
EndEvent
