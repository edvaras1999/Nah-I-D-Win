Scriptname NahIDWinMCM extends SKI_ConfigBase
{ Menu MCM (SkyUI) do NAH-I-D-WIN. E so uma interface por cima do NahIDWin.ini: cada opcao le e escreve
  o .ini atraves da DLL, que recarrega logo a seguir. Nao ha ciclos. }

; Tabela das opcoes (reconstruida de cada vez que uma pagina abre, por isso as versoes novas nao precisam de reiniciar o menu).
string[] sPage
string[] sLabel
string[] sSec
string[] sKey
string[] sInfo
int[] sType
float[] sMin
float[] sMax
float[] sStep
float[] sDef
int[] sOpt
int count

int Function GetVersion()
	return 5
EndFunction

Event OnVersionUpdate(int a_version)
	SetupPages()
EndEvent

Event OnConfigInit()
	SetupPages()
EndEvent

Function SetupPages()
	ModName = "NAH-I-D-WIN"
	Pages = new string[5]
	Pages[0] = "Poise e Postura"
	Pages[1] = "Estamina"
	Pages[2] = "Parry e Furia"
	Pages[3] = "Stagger e Empurrao"
	Pages[4] = "HUD e Geral"
EndFunction

Function AddSetting(string page, string label, string sec, string key, int type, float mn, float mx, float stp, float df, string info)
	sPage[count] = page
	sLabel[count] = label
	sSec[count] = sec
	sKey[count] = key
	sType[count] = type
	sMin[count] = mn
	sMax[count] = mx
	sStep[count] = stp
	sDef[count] = df
	sInfo[count] = info
	count += 1
EndFunction

Function BuildTable()
	sPage = new string[128]
	sLabel = new string[128]
	sSec = new string[128]
	sKey = new string[128]
	sInfo = new string[128]
	sType = new int[128]
	sMin = new float[128]
	sMax = new float[128]
	sStep = new float[128]
	sDef = new float[128]
	sOpt = new int[128]
	count = 0
	AddSetting("Poise e Postura", "Poise maximo - jogador (escala)", "Poise", "MaxScalePlayer", 0, 0.5, 20.0, 0.1, 4.5, "Tamanho da barra de poise do jogador. Mais alto = aguenta mais golpes.")
	AddSetting("Poise e Postura", "Poise maximo - NPCs (escala)", "Poise", "MaxScaleNPC", 0, 0.5, 30.0, 0.5, 9.0, "Tamanho da barra de poise dos NPCs e criaturas.")
	AddSetting("Poise e Postura", "Dano de poise aos NPCs (x)", "Poise", "NPCTakenMult", 0, 0.5, 10.0, 0.1, 2.5, "Quanto poise e postura os NPCs perdem por golpe.")
	AddSetting("Poise e Postura", "Dano de poise ao jogador (x)", "Poise", "PlayerTakenMult", 0, 0.5, 10.0, 0.1, 1.5, "Quanto poise e postura o jogador perde por golpe.")
	AddSetting("Poise e Postura", "Regeneracao do poise (s)", "Poise", "RegenSeconds", 0, 5.0, 120.0, 5.0, 30.0, "Segundos para o poise encher de zero a cheio.")
	AddSetting("Poise e Postura", "Pausa antes de regenerar (s)", "Poise", "RegenDelaySeconds", 0, 0.0, 10.0, 0.5, 2.0, "Segundos sem levar golpes antes de comecar a regenerar.")
	AddSetting("Poise e Postura", "Poise gasto ao bloquear (x)", "Damage", "BlockedMult", 0, 0.0, 1.0, 0.01, 0.15, "Fracao do dano de poise que um golpe bloqueado causa.")
	AddSetting("Poise e Postura", "Imunidade apos stagger pesado (s)", "Stagger", "ImmuneSeconds", 0, 0.0, 8.0, 0.5, 2.5, "Depois de um stagger pesado, poise e postura nao sobem durante este tempo.")
	AddSetting("Poise e Postura", "Postura ligada", "Posture", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Liga a segunda barra (postura). Quando enche: BleedOut.")
	AddSetting("Poise e Postura", "DIFICULDADE da postura", "Posture", "Difficulty", 0, 0.2, 10.0, 0.1, 1.0, "1.0 normal. 2.0 = a postura aguenta o dobro. 0.5 = quebra com metade.")
	AddSetting("Poise e Postura", "Postura - jogador (escala)", "Posture", "ScalePlayer", 0, 1.0, 20.0, 0.5, 5.0, "Tamanho da barra de postura do jogador.")
	AddSetting("Poise e Postura", "Postura - NPCs (escala)", "Posture", "ScaleNPC", 0, 1.0, 20.0, 0.5, 5.0, "Tamanho da barra de postura dos NPCs.")
	AddSetting("Poise e Postura", "Dano de postura (x)", "Posture", "DamageMult", 0, 0.1, 5.0, 0.1, 1.0, "Quanto cada golpe enche a postura.")
	AddSetting("Poise e Postura", "Postura gasta ao bloquear (x)", "Posture", "BlockedMult", 0, 0.0, 1.0, 0.01, 0.12, "Fracao do dano de postura de um golpe bloqueado.")
	AddSetting("Poise e Postura", "Regeneracao da postura (s)", "Posture", "RegenSeconds", 0, 10.0, 300.0, 5.0, 60.0, "Segundos para a postura baixar de cheia a zero.")
	AddSetting("Poise e Postura", "Bonus com vida baixa", "Posture", "LowHealthBonus", 0, 0.0, 3.0, 0.1, 1.0, "Com a vida baixa a postura enche mais depressa. 0 = desligado.")
	AddSetting("Poise e Postura", "Duracao do BleedOut (s)", "Posture", "BleedOutSeconds", 0, 1.0, 15.0, 0.5, 4.0, "Tempo ajoelhado quando a postura quebra.")
	AddSetting("Poise e Postura", "Imunidade apos BleedOut (s)", "Posture", "BleedOutImmuneSeconds", 0, 0.0, 15.0, 0.5, 5.0, "Tempo sem poder voltar a quebrar depois de levantar.")
	AddSetting("Estamina", "Custo de estamina ligado", "Stamina", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Liga o custo de estamina por golpe e a influencia da armadura.")
	AddSetting("Estamina", "Custo base por golpe - jogador", "Stamina", "SwingCostPlayer", 0, 0.0, 40.0, 1.0, 8.0, "Estamina gasta por cada balanco de arma (mesmo sem acertar).")
	AddSetting("Estamina", "Custo por golpe - NPCs", "Stamina", "HitCostNPC", 0, 0.0, 40.0, 1.0, 0.0, "0 = deixa-se aos mods de estamina. Senao a DLL tira isto por golpe que o NPC acerta.")
	AddSetting("Estamina", "Golpe forte (x)", "Stamina", "PowerAttackMult", 0, 0.5, 4.0, 0.1, 1.5, "Multiplicador do custo em golpe forte.")
	AddSetting("Estamina", "Golpe de escudo (x)", "Stamina", "BashMult", 0, 0.0, 4.0, 0.1, 1.0, "Multiplicador do custo em golpe de escudo.")
	AddSetting("Estamina", "Custo: Punhos (x)", "Stamina", "TypeMult0", 0, 0.0, 4.0, 0.1, 0.6, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Espada 1 mao (x)", "Stamina", "TypeMult1", 0, 0.0, 4.0, 0.1, 1.0, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Adaga (x)", "Stamina", "TypeMult2", 0, 0.0, 4.0, 0.1, 0.6, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Machado 1 mao (x)", "Stamina", "TypeMult3", 0, 0.0, 4.0, 0.1, 1.2, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Maca 1 mao (x)", "Stamina", "TypeMult4", 0, 0.0, 4.0, 0.1, 1.3, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Espada 2 maos (x)", "Stamina", "TypeMult5", 0, 0.0, 4.0, 0.1, 1.6, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Machado/Martelo 2 maos (x)", "Stamina", "TypeMult6", 0, 0.0, 4.0, 0.1, 1.8, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Arco (x)", "Stamina", "TypeMult7", 0, 0.0, 4.0, 0.1, 0.8, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Cajado (x)", "Stamina", "TypeMult8", 0, 0.0, 4.0, 0.1, 0.6, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Feitico (x)", "Stamina", "TypeMult9", 0, 0.0, 4.0, 0.1, 0.0, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Escudo (x)", "Stamina", "TypeMult10", 0, 0.0, 4.0, 0.1, 1.0, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Tocha (x)", "Stamina", "TypeMult11", 0, 0.0, 4.0, 0.1, 0.5, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Custo: Besta (x)", "Stamina", "TypeMult12", 0, 0.0, 4.0, 0.1, 0.8, "Multiplicador do custo de estamina para este tipo de arma.")
	AddSetting("Estamina", "Armadura aumenta o custo", "Stamina", "ArmorCostFactor", 0, 0.0, 2.0, 0.05, 0.25, "Custo x (1 + armadura/100 x isto). Armadura pesada cansa mais.")
	AddSetting("Estamina", "Armadura abranda a regeneracao", "Stamina", "ArmorRegenEnabled", 1, 0.0, 1.0, 1.0, 1.0, "A armadura reduz a regeneracao de estamina do jogador.")
	AddSetting("Estamina", "Intensidade da regeneracao", "Stamina", "ArmorRegenFactor", 0, 0.0, 2.0, 0.05, 0.3, "Regeneracao % = 100 / (1 + armadura/100 x isto).")
	AddSetting("Estamina", "Exaustao ligada", "Exhaustion", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Estamina a zero: nao podes atacar nem regenerar por um tempo.")
	AddSetting("Estamina", "Duracao da exaustao (s)", "Exhaustion", "Seconds", 0, 0.5, 10.0, 0.5, 2.5, "Tempo sem poder atacar e sem regenerar.")
	AddSetting("Estamina", "Recarga da exaustao (s)", "Exhaustion", "CooldownSeconds", 0, 0.0, 30.0, 1.0, 8.0, "Tempo minimo entre duas exaustoes.")
	AddSetting("Estamina", "Exaustao nos NPCs", "Exhaustion", "NPCs", 1, 0.0, 1.0, 1.0, 1.0, "NPCs sem estamina levam stagger pesado.")
	AddSetting("Parry e Furia", "Parry ligado", "Parry", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Bloquear logo depois de levantar a guarda conta como parry.")
	AddSetting("Parry e Furia", "Janela do parry (ms)", "Parry", "WindowMs", 0, 50.0, 600.0, 10.0, 250.0, "Tempo depois de levantar o bloqueio em que o parry funciona.")
	AddSetting("Parry e Furia", "Postura do atacante (fracao)", "Parry", "AttackerPostureFraction", 0, 0.0, 1.0, 0.05, 0.25, "Fracao da postura maxima do atacante que enche no parry.")
	AddSetting("Parry e Furia", "Atacante leva stagger", "Parry", "AttackerStagger", 1, 0.0, 1.0, 1.0, 1.0, "O atacante leva stagger medio no parry.")
	AddSetting("Parry e Furia", "Postura recuperada (fracao)", "Parry", "DefenderPostureRecoverFraction", 0, 0.0, 1.0, 0.05, 0.15, "O que a tua postura recupera no parry.")
	AddSetting("Parry e Furia", "Poise recuperado (fracao)", "Parry", "DefenderPoiseRecoverFraction", 0, 0.0, 1.0, 0.05, 0.25, "O que o teu poise recupera no parry.")
	AddSetting("Parry e Furia", "Parry sem dano", "Parry", "NoDamage", 1, 0.0, 1.0, 1.0, 1.0, "Devolve a vida perdida no golpe aparado.")
	AddSetting("Parry e Furia", "Faiscas e som do parry", "Parry", "Effects", 1, 0.0, 1.0, 1.0, 1.0, "Usa os efeitos do Inpa Sekiro Combat se estiver instalado.")
	AddSetting("Parry e Furia", "Furia ligada", "Rage", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Imunidade temporaria em troca de vida ou estamina.")
	AddSetting("Parry e Furia", "Tecla da furia (codigo)", "Rage", "Key", 0, 1.0, 255.0, 1.0, 34.0, "Codigo DirectInput. 34 = G, 19 = R, 47 = V, 46 = C. Carrega o save para aplicar.")
	AddSetting("Parry e Furia", "Duracao da furia (s)", "Rage", "DurationSeconds", 0, 1.0, 20.0, 0.5, 5.0, "Tempo de imunidade.")
	AddSetting("Parry e Furia", "Recarga da furia (s)", "Rage", "CooldownSeconds", 0, 5.0, 180.0, 5.0, 30.0, "Tempo ate poderes usar outra vez.")
	AddSetting("Parry e Furia", "Custo: 0 vida, 1 estamina", "Rage", "CostType", 0, 0.0, 1.0, 1.0, 0.0, "Com o que pagas a furia.")
	AddSetting("Parry e Furia", "Custo (% do maximo)", "Rage", "CostPercent", 0, 0.0, 60.0, 1.0, 15.0, "Percentagem do maximo que a furia custa. A vida nunca te mata.")
	AddSetting("Parry e Furia", "NPCs usam furia", "Rage", "NPCs", 1, 0.0, 1.0, 1.0, 1.0, "NPCs ativam a furia sozinhos quando perto de quebrar.")
	AddSetting("Parry e Furia", "NPC: postura acima de", "Rage", "NPCPostureTrigger", 0, 0.3, 1.0, 0.05, 0.8, "O NPC ativa quando a postura passa desta fracao.")
	AddSetting("Parry e Furia", "NPC: poise abaixo de", "Rage", "NPCPoiseTrigger", 0, 0.0, 0.6, 0.05, 0.15, "... ou quando o poise desce abaixo desta fracao.")
	AddSetting("Parry e Furia", "NPC: probabilidade (%)", "Rage", "NPCChancePercent", 0, 0.0, 100.0, 5.0, 50.0, "Probabilidade por golpe recebido quando a condicao se cumpre.")
	AddSetting("Parry e Furia", "NPC: recarga (s)", "Rage", "NPCCooldownSeconds", 0, 5.0, 180.0, 5.0, 45.0, "Tempo minimo entre duas furias do mesmo NPC.")
	AddSetting("Parry e Furia", "NPC: custo (%)", "Rage", "NPCCostPercent", 0, 0.0, 50.0, 1.0, 0.0, "0 = gratis para os NPCs.")
	AddSetting("Stagger e Empurrao", "Stagger leve ligado", "Stagger", "LightEnabled", 1, 0.0, 1.0, 1.0, 1.0, "Golpe normal sem bloquear da stagger leve.")
	AddSetting("Stagger e Empurrao", "Stagger leve no jogador", "Stagger", "LightPlayer", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("Stagger e Empurrao", "Stagger leve nos NPCs", "Stagger", "LightNPCs", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("Stagger e Empurrao", "Espaco do stagger leve - NPCs (ms)", "Stagger", "LightCooldownMs", 0, 100.0, 3000.0, 100.0, 700.0, "Tempo minimo entre dois staggers leves.")
	AddSetting("Stagger e Empurrao", "Espaco do stagger leve - jogador (ms)", "Stagger", "LightCooldownPlayerMs", 0, 100.0, 5000.0, 100.0, 1500.0, "Mais espaco = menos tempo travado a levar golpes.")
	AddSetting("Stagger e Empurrao", "Stagger medio em golpe forte", "Stagger", "MediumOnPowerAttack", 1, 0.0, 1.0, 1.0, 1.0, "Golpe forte ou de escudo sem bloquear da stagger medio.")
	AddSetting("Stagger e Empurrao", "Empurrao ligado", "Knockback", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Empurrao no stagger pesado, so em condicoes especificas.")
	AddSetting("Stagger e Empurrao", "Forca do empurrao", "Knockback", "Force", 0, 0.0, 5.0, 0.1, 1.0, "0 = sem empurrao.")
	AddSetting("Stagger e Empurrao", "Empurrar o jogador", "Knockback", "Player", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("Stagger e Empurrao", "Quando golpe forte", "Knockback", "OnPowerAttack", 1, 0.0, 1.0, 1.0, 1.0, "Inclui o carregado e o de corrida.")
	AddSetting("Stagger e Empurrao", "Quando criatura gigante", "Knockback", "OnGiantCreature", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("Stagger e Empurrao", "Massa minima de gigante", "Knockback", "GiantMassMin", 0, 1.0, 10.0, 0.5, 3.0, "Massa da raca a partir da qual conta como gigante.")
	AddSetting("Stagger e Empurrao", "Quando arma pesada (golpe normal)", "Knockback", "OnHeavyWeapon", 1, 0.0, 1.0, 1.0, 0.0, "Cuidado: deixa as armas grandes muito fortes.")
	AddSetting("Stagger e Empurrao", "Peso minimo de arma pesada", "Knockback", "HeavyWeaponMinWeight", 0, 5.0, 40.0, 1.0, 15.0, "")
	AddSetting("HUD e Geral", "Mod ligado", "General", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Liga ou desliga todo o sistema.")
	AddSetting("HUD e Geral", "Poise no jogador", "General", "Player", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("HUD e Geral", "Poise nos NPCs", "General", "NPCs", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("HUD e Geral", "Barra do TrueHUD", "TrueHUD", "Bar", 0, 0.0, 2.0, 1.0, 2.0, "0 = poise, 1 = postura, 2 = risco (a que estiver mais perto de quebrar).")
	AddSetting("HUD e Geral", "Aviso de poise baixo (fracao)", "TrueHUD", "PoiseWarnFraction", 0, 0.0, 0.8, 0.05, 0.3, "A barra pisca quando o poise desce abaixo disto. 0 = desligado.")
	AddSetting("HUD e Geral", "Barra pisca ao quebrar", "TrueHUD", "FlashOnBreak", 1, 0.0, 1.0, 1.0, 1.0, "")
	AddSetting("HUD e Geral", "Segunda barra (EXPERIMENTAL)", "TrueHUD", "SecondBar", 1, 0.0, 1.0, 1.0, 0.0, "Desenha poise e postura no ecra do jogador. Se der problema, desliga.")
	AddSetting("HUD e Geral", "Segunda barra: posicao X", "TrueHUD", "SecondBarX", 0, 0.0, 1280.0, 10.0, 490.0, "")
	AddSetting("HUD e Geral", "Segunda barra: posicao Y", "TrueHUD", "SecondBarY", 0, 0.0, 720.0, 10.0, 640.0, "")
	AddSetting("HUD e Geral", "Segunda barra: largura", "TrueHUD", "SecondBarWidth", 0, 50.0, 600.0, 10.0, 300.0, "")
	AddSetting("HUD e Geral", "Segunda barra: altura", "TrueHUD", "SecondBarHeight", 0, 2.0, 20.0, 1.0, 6.0, "")
	AddSetting("HUD e Geral", "Diagnostico no log", "Diagnostics", "Enabled", 1, 0.0, 1.0, 1.0, 1.0, "Escreve o que acontece em NahIDWin.log.")
EndFunction

string Function FormatFor(int i)
	if sStep[i] >= 1.0
		return "{0}"
	endif
	return "{2}"
EndFunction

Event OnPageReset(string a_page)
	BuildTable()
	SetCursorFillMode(TOP_TO_BOTTOM)
	AddHeaderOption(a_page)
	int i = 0
	while i < count
		if sPage[i] == a_page
			float cur = NahIDWinNative.IniGetF(sSec[i], sKey[i], sDef[i])
			if sType[i] == 1
				sOpt[i] = AddToggleOption(sLabel[i], cur > 0.5)
			else
				sOpt[i] = AddSliderOption(sLabel[i], cur, FormatFor(i))
			endif
		endif
		i += 1
	endWhile
EndEvent

int Function FindOpt(int a_option)
	int i = 0
	while i < count
		if sOpt[i] == a_option && sPage[i] == CurrentPage
			return i
		endif
		i += 1
	endWhile
	return -1
EndFunction

Event OnOptionSelect(int a_option)
	int i = FindOpt(a_option)
	if i < 0
		return
	endif
	if sType[i] == 1
		float cur = NahIDWinNative.IniGetF(sSec[i], sKey[i], sDef[i])
		float nv = 1.0
		if cur > 0.5
			nv = 0.0
		endif
		NahIDWinNative.IniSetF(sSec[i], sKey[i], nv)
		SetToggleOptionValue(a_option, nv > 0.5)
	endif
EndEvent

Event OnOptionSliderOpen(int a_option)
	int i = FindOpt(a_option)
	if i < 0
		return
	endif
	SetSliderDialogStartValue(NahIDWinNative.IniGetF(sSec[i], sKey[i], sDef[i]))
	SetSliderDialogDefaultValue(sDef[i])
	SetSliderDialogRange(sMin[i], sMax[i])
	SetSliderDialogInterval(sStep[i])
EndEvent

Event OnOptionSliderAccept(int a_option, float a_value)
	int i = FindOpt(a_option)
	if i < 0
		return
	endif
	NahIDWinNative.IniSetF(sSec[i], sKey[i], a_value)
	SetSliderOptionValue(a_option, a_value, FormatFor(i))
EndEvent

Event OnOptionDefault(int a_option)
	int i = FindOpt(a_option)
	if i < 0
		return
	endif
	NahIDWinNative.IniSetF(sSec[i], sKey[i], sDef[i])
	if sType[i] == 1
		SetToggleOptionValue(a_option, sDef[i] > 0.5)
	else
		SetSliderOptionValue(a_option, sDef[i], FormatFor(i))
	endif
EndEvent

Event OnOptionHighlight(int a_option)
	int i = FindOpt(a_option)
	if i < 0
		return
	endif
	SetInfoText(sInfo[i])
EndEvent
