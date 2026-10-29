Scriptname NahIDWinMCM extends SKI_ConfigBase
{ Menu MCM (SkyUI) do NAH-I-D-WIN v0.6.1: 3 abas, duas colunas, English / Portugues (BR).
  E so uma interface por cima do NahIDWin.ini: cada opcao le e escreve o .ini atraves da DLL. Nao ha ciclos. }

int[] sPage
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
int[] sOpt
int count

int Function GetVersion()
	return 7
EndFunction

Event OnVersionUpdate(int a_version)
	SetupPages()
EndEvent

Event OnConfigInit()
	SetupPages()
EndEvent

Event OnConfigOpen()
	SetupPages()
EndEvent

Function SetupPages()
	ModName = "NAH-I-D-WIN"
	Pages = new string[3]
	Pages[0] = "Poise and Posture"
	Pages[1] = "Parry, Rage, Health"
	Pages[2] = "Stamina, HUD, System"
EndFunction

bool Function IsPT()
	return NahIDWinNative.GetLanguage() == 1
EndFunction

Function AddSetting(int pg, int ty, string sec, string key, string lpt, string lenn, string ipt, string ien, float mn, float mx, float stp, float df)
	sPage[count] = pg
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

Function BuildTable()
	sPage = new int[128]
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
	sOpt = new int[128]
	count = 0
	AddSetting(0, 2, "", "", "Poise", "Poise", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(0, 0, "Poise", "MaxScalePlayer", "Poise maximo - jogador (escala)", "Max poise - player (scale)", "Tamanho da barra de poise do jogador. Mais alto = aguenta mais golpes.", "Size of the player's poise bar. Higher = survives more hits.", 0.5, 20.0, 0.1, 4.5)
	AddSetting(0, 0, "Poise", "MaxScaleNPC", "Poise maximo - NPCs (escala)", "Max poise - NPCs (scale)", "Tamanho da barra de poise dos NPCs e criaturas.", "Size of the poise bar of NPCs and creatures.", 0.5, 30.0, 0.5, 9.0)
	AddSetting(0, 0, "Poise", "NPCTakenMult", "Dano de poise aos NPCs (x)", "Poise damage to NPCs (x)", "Quanto poise e postura os NPCs perdem por golpe.", "How much poise and posture NPCs lose per hit.", 0.5, 10.0, 0.1, 2.5)
	AddSetting(0, 0, "Poise", "PlayerTakenMult", "Dano de poise ao jogador (x)", "Poise damage to player (x)", "Quanto poise e postura o jogador perde por golpe.", "How much poise and posture the player loses per hit.", 0.5, 10.0, 0.1, 1.5)
	AddSetting(0, 0, "Poise", "RegenSeconds", "Regeneracao do poise (s)", "Poise regeneration (s)", "Segundos para o poise encher de zero a cheio.", "Seconds for poise to refill from zero to full.", 5.0, 120.0, 5.0, 30.0)
	AddSetting(0, 0, "Poise", "RegenDelaySeconds", "Pausa antes de regenerar (s)", "Delay before regenerating (s)", "Segundos sem levar golpes antes de comecar a regenerar.", "Seconds without being hit before regeneration starts.", 0.0, 10.0, 0.5, 2.0)
	AddSetting(0, 0, "Damage", "BlockedMult", "Poise gasto ao bloquear (x)", "Poise spent when blocking (x)", "Fracao do dano de poise que um golpe bloqueado causa.", "Fraction of poise damage a blocked hit causes.", 0.0, 1.0, 0.01, 0.15)
	AddSetting(0, 0, "Stagger", "ImmuneSeconds", "Imunidade apos stagger pesado (s)", "Immunity after heavy stagger (s)", "Depois de um stagger pesado, poise e postura nao sobem durante este tempo.", "After a heavy stagger, poise and posture do not rise for this long.", 0.0, 8.0, 0.5, 2.5)
	AddSetting(0, 2, "", "", "Postura", "Posture", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(0, 1, "Posture", "Enabled", "Postura ligada", "Posture enabled", "Liga a segunda barra (postura). Quando enche: BleedOut.", "Enables the posture bar. When it fills: BleedOut.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 0, "Posture", "Difficulty", "DIFICULDADE da postura", "Posture DIFFICULTY", "1.0 normal. 2.0 = a postura aguenta o dobro. 0.5 = quebra com metade.", "1.0 normal. 2.0 = posture lasts twice as long. 0.5 = breaks with half.", 0.2, 10.0, 0.1, 1.0)
	AddSetting(0, 0, "Posture", "ScalePlayer", "Postura - jogador (escala)", "Posture - player (scale)", "Tamanho da barra de postura do jogador.", "Size of the player's posture bar.", 1.0, 20.0, 0.5, 5.0)
	AddSetting(0, 0, "Posture", "ScaleNPC", "Postura - NPCs (escala)", "Posture - NPCs (scale)", "Tamanho da barra de postura dos NPCs.", "Size of the NPCs' posture bar.", 1.0, 20.0, 0.5, 5.0)
	AddSetting(0, 0, "Posture", "DamageMult", "Dano de postura (x)", "Posture damage (x)", "Quanto cada golpe enche a postura.", "How much each hit fills posture.", 0.1, 5.0, 0.1, 1.0)
	AddSetting(0, 0, "Posture", "BlockedMult", "Postura gasta ao bloquear (x)", "Posture spent when blocking (x)", "Fracao do dano de postura de um golpe bloqueado.", "Fraction of posture damage of a blocked hit.", 0.0, 1.0, 0.01, 0.12)
	AddSetting(0, 0, "Posture", "RegenSeconds", "Regeneracao da postura (s)", "Posture regeneration (s)", "Segundos para a postura baixar de cheia a zero.", "Seconds for posture to drop from full to zero.", 10.0, 300.0, 5.0, 60.0)
	AddSetting(0, 0, "Posture", "LowHealthBonus", "Bonus com vida baixa", "Low health bonus", "Com a vida baixa a postura enche mais depressa. 0 = desligado.", "With low health posture fills faster. 0 = off.", 0.0, 3.0, 0.1, 1.0)
	AddSetting(0, 0, "Posture", "BleedOutSeconds", "Duracao do BleedOut (s)", "BleedOut duration (s)", "Tempo ajoelhado quando a postura quebra.", "Time kneeling when posture breaks.", 1.0, 15.0, 0.5, 4.0)
	AddSetting(0, 0, "Posture", "BleedOutImmuneSeconds", "Imunidade apos BleedOut (s)", "Immunity after BleedOut (s)", "Tempo sem poder voltar a quebrar depois de levantar.", "Time you cannot break again after getting up.", 0.0, 15.0, 0.5, 5.0)
	AddSetting(0, 2, "", "", "Stagger e empurrao", "Stagger and knockback", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(0, 1, "Stagger", "LightEnabled", "Stagger leve ligado", "Light stagger enabled", "Golpe normal sem bloquear da stagger leve.", "A normal unblocked hit causes a light stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 1, "Stagger", "LightPlayer", "Stagger leve no jogador", "Light stagger on player", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 1, "Stagger", "LightNPCs", "Stagger leve nos NPCs", "Light stagger on NPCs", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 0, "Stagger", "LightCooldownMs", "Espaco do stagger leve - NPCs (ms)", "Light stagger spacing - NPCs (ms)", "Tempo minimo entre dois staggers leves.", "Minimum time between two light staggers.", 100.0, 3000.0, 100.0, 700.0)
	AddSetting(0, 0, "Stagger", "LightCooldownPlayerMs", "Espaco do stagger leve - jogador (ms)", "Light stagger spacing - player (ms)", "Mais espaco = menos tempo travado a levar golpes.", "More spacing = less time locked while being hit.", 100.0, 5000.0, 100.0, 1500.0)
	AddSetting(0, 1, "Stagger", "MediumOnPowerAttack", "Stagger medio em golpe forte", "Medium stagger on power attack", "Golpe forte ou de escudo sem bloquear da stagger medio.", "An unblocked power attack or bash causes a medium stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 1, "Knockback", "Enabled", "Empurrao ligado", "Knockback enabled", "Empurrao no stagger pesado, so em condicoes especificas.", "Knockback on heavy stagger, only in specific conditions.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 0, "Knockback", "Force", "Forca do empurrao", "Knockback force", "0 = sem empurrao.", "0 = no knockback.", 0.0, 5.0, 0.1, 1.0)
	AddSetting(0, 1, "Knockback", "Player", "Empurrar o jogador", "Knock back the player", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 1, "Knockback", "OnPowerAttack", "Quando golpe forte", "On power attack", "Inclui o carregado e o de corrida.", "Includes charged and sprinting attacks.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 1, "Knockback", "OnGiantCreature", "Quando criatura gigante", "On giant creature", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(0, 0, "Knockback", "GiantMassMin", "Massa minima de gigante", "Giant minimum mass", "Massa da raca a partir da qual conta como gigante.", "Race mass from which it counts as giant.", 1.0, 10.0, 0.5, 3.0)
	AddSetting(0, 1, "Knockback", "OnHeavyWeapon", "Quando arma pesada (golpe normal)", "On heavy weapon (normal hit)", "Cuidado: deixa as armas grandes muito fortes.", "Careful: makes big weapons very strong.", 0.0, 1.0, 1.0, 0.0)
	AddSetting(0, 0, "Knockback", "HeavyWeaponMinWeight", "Peso minimo de arma pesada", "Heavy weapon minimum weight", "", "", 5.0, 40.0, 1.0, 15.0)
	AddSetting(1, 2, "", "", "Parry", "Parry", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(1, 1, "Parry", "Enabled", "Parry ligado", "Parry enabled", "Bloquear logo depois de levantar a guarda conta como parry.", "Blocking right after raising the guard counts as a parry.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Parry", "WindowMs", "Janela do parry (ms)", "Parry window (ms)", "Tempo depois de levantar o bloqueio em que o parry funciona. Ajusta ao teu gosto.", "Time after raising the block in which the parry works.", 100.0, 800.0, 10.0, 300.0)
	AddSetting(1, 0, "Parry", "CooldownMs", "Recarga do parry (ms)", "Parry cooldown (ms)", "Tempo minimo entre duas janelas de parry (evita carregar sem parar).", "Minimum time between two parry windows (stops block spamming).", 100.0, 2000.0, 50.0, 600.0)
	AddSetting(1, 1, "Parry", "NoDamage", "Parry sem dano", "Parry takes no damage", "O golpe aparado causa dano de vida zero (feito na DLL).", "A parried hit causes zero health damage (done in the DLL).", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Parry", "AttackerPostureFraction", "Postura do atacante (fracao)", "Attacker posture (fraction)", "Fracao da postura maxima do atacante que enche no parry.", "Fraction of the attacker's max posture filled on parry.", 0.0, 1.0, 0.05, 0.25)
	AddSetting(1, 1, "Parry", "AttackerStagger", "Atacante leva stagger", "Attacker staggers", "O atacante leva stagger medio no parry.", "The attacker gets a medium stagger on parry.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Parry", "DefenderPostureRecoverFraction", "Postura recuperada (fracao)", "Posture recovered (fraction)", "O que a tua postura recupera no parry.", "What your posture recovers on parry.", 0.0, 1.0, 0.05, 0.15)
	AddSetting(1, 0, "Parry", "DefenderPoiseRecoverFraction", "Poise recuperado (fracao)", "Poise recovered (fraction)", "O que o teu poise recupera no parry.", "What your poise recovers on parry.", 0.0, 1.0, 0.05, 0.25)
	AddSetting(1, 1, "Parry", "Effects", "Faiscas e som do parry", "Parry sparks and sound", "Usa os efeitos do Inpa Sekiro Combat se estiver instalado.", "Uses Inpa Sekiro Combat effects if installed.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 2, "", "", "Bloqueio normal", "Normal block", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(1, 1, "Block", "DrainEnabled", "Bloqueio gasta estamina do atacante", "Normal block drains attacker stamina", "Bloqueio normal (sem parry) gasta estamina do atacante. Sem stagger.", "A normal block (no parry) costs the attacker stamina. No stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Block", "AttackerStamina", "Estamina do atacante por golpe", "Attacker stamina per blocked hit", "Estamina que o atacante perde por golpe bloqueado.", "Stamina the attacker loses per blocked hit.", 0.0, 60.0, 1.0, 10.0)
	AddSetting(1, 0, "Block", "PowerAttackMult", "Golpe forte bloqueado (x)", "Power attack blocked (x)", "Multiplicador desse custo em golpes fortes.", "Multiplier of that cost for power attacks.", 0.5, 4.0, 0.1, 1.5)
	AddSetting(1, 2, "", "", "Furia", "Rage", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(1, 1, "Rage", "Enabled", "Furia ligada", "Rage enabled", "Imunidade temporaria em troca de vida ou estamina.", "Temporary stagger immunity and damage reduction at a cost.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Rage", "Key", "Tecla da furia (codigo)", "Rage key (code)", "Codigo DirectInput. 34 = G, 19 = R, 47 = V, 46 = C.", "DirectInput code. 34 = G, 19 = R, 47 = V, 46 = C.", 1.0, 255.0, 1.0, 34.0)
	AddSetting(1, 0, "Rage", "DurationSeconds", "Duracao da furia (s)", "Rage duration (s)", "Tempo de imunidade.", "How long rage lasts.", 1.0, 20.0, 0.5, 5.0)
	AddSetting(1, 0, "Rage", "CooldownSeconds", "Recarga da furia (s)", "Rage cooldown (s)", "Tempo ate poderes usar outra vez.", "Time until you can use it again.", 5.0, 180.0, 5.0, 30.0)
	AddSetting(1, 0, "Rage", "CostType", "Custo: 0 vida, 1 estamina", "Cost: 0 health, 1 stamina", "Com o que pagas a furia.", "What you pay the rage with.", 0.0, 1.0, 1.0, 0.0)
	AddSetting(1, 0, "Rage", "CostPercent", "Custo (% do maximo)", "Cost (% of max)", "Percentagem do maximo que a furia custa. A vida nunca te mata.", "Percent of max that rage costs. Health never kills you.", 0.0, 60.0, 1.0, 15.0)
	AddSetting(1, 0, "Rage", "DamageReductionPercent", "Reducao de dano - jogador (%)", "Player damage reduction (%)", "Reducao do dano de vida enquanto o jogador esta em furia.", "Health damage reduction while the player is raging.", 0.0, 95.0, 5.0, 70.0)
	AddSetting(1, 0, "Rage", "NPCDamageReductionPercent", "Reducao de dano - NPC (%)", "NPC damage reduction (%)", "Reducao do dano de vida enquanto um NPC esta em furia.", "Health damage reduction while an NPC is raging.", 0.0, 95.0, 5.0, 50.0)
	AddSetting(1, 1, "Rage", "ZeroStamina", "Furia sem custo de estamina", "Zero stamina cost in rage", "Durante a furia os golpes nao gastam estamina (so vida).", "During rage your swings cost no stamina (only health).", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 1, "Rage", "NPCs", "NPCs usam furia", "NPCs use rage", "NPCs ativam a furia sozinhos quando perto de quebrar.", "NPCs trigger rage on their own when close to breaking.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Rage", "NPCPostureTrigger", "NPC: postura acima de", "NPC: posture above", "O NPC ativa quando a postura passa desta fracao.", "The NPC triggers when posture passes this fraction.", 0.3, 1.0, 0.05, 0.8)
	AddSetting(1, 0, "Rage", "NPCPoiseTrigger", "NPC: poise abaixo de", "NPC: poise below", "... ou quando o poise desce abaixo desta fracao.", "... or when poise drops below this fraction.", 0.0, 0.6, 0.05, 0.15)
	AddSetting(1, 0, "Rage", "NPCChancePercent", "NPC: probabilidade (%)", "NPC: chance (%)", "Probabilidade por golpe recebido quando a condicao se cumpre.", "Chance per hit received when the condition is met.", 0.0, 100.0, 5.0, 50.0)
	AddSetting(1, 0, "Rage", "NPCCooldownSeconds", "NPC: recarga (s)", "NPC: cooldown (s)", "Tempo minimo entre duas furias do mesmo NPC.", "Minimum time between two rages of the same NPC.", 5.0, 180.0, 5.0, 45.0)
	AddSetting(1, 0, "Rage", "NPCCostPercent", "NPC: custo (%)", "NPC: cost (%)", "0 = gratis para os NPCs.", "0 = free for NPCs.", 0.0, 50.0, 1.0, 0.0)
	AddSetting(1, 2, "", "", "Dificuldade propria", "Own difficulty", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(1, 1, "Health", "Enabled", "Dificuldade propria ligada", "Own difficulty enabled", "Dano de vida independente da dificuldade do jogo (GMST).", "Health damage independent of the game difficulty (GMST).", 0.0, 1.0, 1.0, 1.0)
	AddSetting(1, 0, "Health", "TakenMult", "Dano recebido pelo jogador (x)", "Damage taken by player (x)", "1.0 = normal. Multiplica o dano de vida que o jogador leva.", "1.0 = normal. Multiplies health damage the player receives.", 0.1, 5.0, 0.1, 1.0)
	AddSetting(1, 0, "Health", "DealtMult", "Dano causado pelo jogador (x)", "Damage dealt by player (x)", "1.0 = normal. Multiplica o dano de vida que o jogador causa.", "1.0 = normal. Multiplies health damage the player deals.", 0.1, 5.0, 0.1, 1.0)
	AddSetting(1, 0, "Health", "BleedOutDamageMult", "Dano extra em BleedOut (x)", "Extra damage vs BleedOut (x)", "1.0 = sem extra. 2.0 = dobro de dano num alvo em BleedOut.", "1.0 = none. 2.0 = double damage on a target in BleedOut.", 1.0, 5.0, 0.1, 1.0)
	AddSetting(2, 2, "", "", "Estamina", "Stamina", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(2, 1, "Stamina", "Enabled", "Custo de estamina ligado", "Stamina cost enabled", "Liga o custo de estamina por golpe e a influencia da armadura.", "Enables stamina cost per swing and armor influence.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 0, "Stamina", "SwingCostPlayer", "Custo base por golpe - jogador", "Base cost per swing - player", "Estamina gasta por cada balanco de arma (mesmo sem acertar).", "Stamina spent by each weapon swing (even when missing).", 0.0, 40.0, 1.0, 8.0)
	AddSetting(2, 0, "Stamina", "HitCostNPC", "Custo por golpe - NPCs", "Cost per swing - NPCs", "0 = deixa-se aos mods de estamina. Senao a DLL tira isto por golpe que o NPC acerta.", "0 = left to stamina mods. Otherwise the DLL takes this per hit the NPC lands.", 0.0, 40.0, 1.0, 0.0)
	AddSetting(2, 0, "Stamina", "PowerAttackMult", "Golpe forte (x)", "Power attack (x)", "Multiplicador do custo em golpe forte.", "Cost multiplier for power attacks.", 0.5, 4.0, 0.1, 1.5)
	AddSetting(2, 0, "Stamina", "BashMult", "Golpe de escudo (x)", "Shield bash (x)", "Multiplicador do custo em golpe de escudo.", "Cost multiplier for shield bashes.", 0.0, 4.0, 0.1, 1.0)
	AddSetting(2, 0, "Stamina", "TypeMult0", "Custo: Punhos (x)", "Cost: Fists (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.6)
	AddSetting(2, 0, "Stamina", "TypeMult1", "Custo: Espada 1 mao (x)", "Cost: 1H Sword (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.0)
	AddSetting(2, 0, "Stamina", "TypeMult2", "Custo: Adaga (x)", "Cost: Dagger (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.6)
	AddSetting(2, 0, "Stamina", "TypeMult3", "Custo: Machado 1 mao (x)", "Cost: 1H Axe (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.2)
	AddSetting(2, 0, "Stamina", "TypeMult4", "Custo: Maca 1 mao (x)", "Cost: 1H Mace (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.3)
	AddSetting(2, 0, "Stamina", "TypeMult5", "Custo: Espada 2 maos (x)", "Cost: 2H Sword (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.6)
	AddSetting(2, 0, "Stamina", "TypeMult6", "Custo: Machado/Martelo 2 maos (x)", "Cost: 2H Axe/Hammer (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.8)
	AddSetting(2, 0, "Stamina", "TypeMult7", "Custo: Arco (x)", "Cost: Bow (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.8)
	AddSetting(2, 0, "Stamina", "TypeMult8", "Custo: Cajado (x)", "Cost: Staff (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.6)
	AddSetting(2, 0, "Stamina", "TypeMult9", "Custo: Feitico (x)", "Cost: Spell (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.0)
	AddSetting(2, 0, "Stamina", "TypeMult10", "Custo: Escudo (x)", "Cost: Shield (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 1.0)
	AddSetting(2, 0, "Stamina", "TypeMult11", "Custo: Tocha (x)", "Cost: Torch (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.5)
	AddSetting(2, 0, "Stamina", "TypeMult12", "Custo: Besta (x)", "Cost: Crossbow (x)", "Multiplicador do custo de estamina para este tipo de arma.", "Stamina cost multiplier for this weapon type.", 0.0, 4.0, 0.1, 0.8)
	AddSetting(2, 0, "Stamina", "ArmorCostFactor", "Armadura aumenta o custo", "Armor raises the cost", "Custo x (1 + armadura/100 x isto). Armadura pesada cansa mais.", "Cost x (1 + armor/100 x this). Heavy armor tires faster.", 0.0, 2.0, 0.05, 0.25)
	AddSetting(2, 1, "Stamina", "ArmorRegenEnabled", "Armadura abranda a regeneracao", "Armor slows regeneration", "A armadura reduz a regeneracao de estamina do jogador.", "Armor reduces the player's stamina regeneration.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 0, "Stamina", "ArmorRegenFactor", "Intensidade da regeneracao", "Regeneration strength", "Regeneracao % = 100 / (1 + armadura/100 x isto).", "Regeneration % = 100 / (1 + armor/100 x this).", 0.0, 2.0, 0.05, 0.3)
	AddSetting(2, 2, "", "", "Exaustao", "Exhaustion", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(2, 1, "Exhaustion", "Enabled", "Exaustao ligada", "Exhaustion enabled", "Estamina a zero: nao podes atacar nem regenerar por um tempo.", "Stamina at zero: you cannot attack or regenerate for a while.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 0, "Exhaustion", "Seconds", "Duracao da exaustao (s)", "Exhaustion duration (s)", "Tempo sem poder atacar e sem regenerar.", "Time without attacking and without regenerating.", 0.5, 10.0, 0.5, 2.5)
	AddSetting(2, 0, "Exhaustion", "CooldownSeconds", "Recarga da exaustao (s)", "Exhaustion cooldown (s)", "Tempo minimo entre duas exaustoes.", "Minimum time between two exhaustions.", 0.0, 30.0, 1.0, 8.0)
	AddSetting(2, 1, "Exhaustion", "NPCs", "Exaustao nos NPCs", "Exhaustion on NPCs", "NPCs sem estamina levam stagger pesado.", "NPCs with no stamina get a heavy stagger.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 2, "", "", "Barras (HUD)", "Bars (HUD)", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(2, 0, "TrueHUD", "Bar", "Barra do TrueHUD", "TrueHUD bar", "0 = poise, 1 = postura (recomendado), 2 = risco (a mais perto de quebrar).", "0 = poise, 1 = posture, 2 = risk (whichever is closest to breaking).", 0.0, 2.0, 1.0, 1.0)
	AddSetting(2, 0, "TrueHUD", "PoiseWarnFraction", "Aviso de poise baixo (fracao)", "Low poise warning (fraction)", "A barra pisca quando o poise desce abaixo disto. 0 = desligado.", "The bar flashes when poise drops below this. 0 = off.", 0.0, 0.8, 0.05, 0.3)
	AddSetting(2, 1, "TrueHUD", "FlashOnBreak", "Barra pisca ao quebrar", "Bar flashes on break", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "TrueHUD", "SecondBar", "Barras proprias (Poise + Furia)", "Own bars (Poise + Rage)", "Desenha linhas de poise e furia por baixo da estamina do jogador.", "Draws poise and rage lines under the player's stamina.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "TrueHUD", "BarPoise", "Mostrar linha de poise", "Show poise line", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "TrueHUD", "BarRage", "Mostrar linha de furia", "Show rage line", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 0, "TrueHUD", "SecondBarX", "Barras: posicao X", "Bars: X position", "", "", 0.0, 1280.0, 10.0, 490.0)
	AddSetting(2, 0, "TrueHUD", "SecondBarY", "Barras: posicao Y", "Bars: Y position", "", "", 0.0, 720.0, 10.0, 640.0)
	AddSetting(2, 0, "TrueHUD", "SecondBarWidth", "Barras: largura", "Bars: width", "", "", 50.0, 600.0, 10.0, 300.0)
	AddSetting(2, 0, "TrueHUD", "SecondBarHeight", "Barras: altura", "Bars: height", "", "", 2.0, 20.0, 1.0, 6.0)
	AddSetting(2, 1, "TrueHUD", "EnemyBars", "Barras nos inimigos (EXPERIMENTAL)", "Bars under enemies (EXPERIMENTAL)", "As mesmas linhas por baixo da cabeca dos inimigos. Desliga se der problemas.", "Same lines under enemies' heads. Turn off if there are problems.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 0, "TrueHUD", "EnemyBarWidth", "Barras inimigos: largura", "Enemy bars: width", "", "", 40.0, 300.0, 5.0, 110.0)
	AddSetting(2, 0, "TrueHUD", "EnemyBarHeight", "Barras inimigos: altura", "Enemy bars: height", "", "", 2.0, 16.0, 1.0, 4.0)
	AddSetting(2, 0, "TrueHUD", "EnemyBarOffsetY", "Barras inimigos: deslocamento vertical", "Enemy bars: vertical offset", "Pixeis para baixo a partir da cabeca. Negativo = para cima.", "Pixels down from the head. Negative = up.", -100.0, 100.0, 2.0, 14.0)
	AddSetting(2, 0, "TrueHUD", "EnemyBarMax", "Barras inimigos: maximo", "Enemy bars: max count", "Quantas barras de inimigos no maximo.", "How many enemy bars at most.", 1.0, 16.0, 1.0, 8.0)
	AddSetting(2, 2, "", "", "Sistema", "System", "", "", 0.0, 0.0, 0.0, 0.0)
	AddSetting(2, 1, "Camera", "DisableShake", "Remover abalo de camara (EXPERIMENTAL)", "Remove camera shake (EXPERIMENTAL)", "Tenta tirar a oscilacao do ecra no stagger. Pode afetar outros abalos.", "Tries to remove the screen shake on stagger. Also affects other shakes.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "General", "Enabled", "Mod ligado", "Mod enabled", "Liga ou desliga todo o sistema.", "Turns the whole system on or off.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "General", "Player", "Poise no jogador", "Poise on player", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "General", "NPCs", "Poise nos NPCs", "Poise on NPCs", "", "", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "Diagnostics", "Enabled", "Diagnostico no log", "Diagnostics in log", "Escreve o que acontece em NahIDWin.log.", "Writes what happens to NahIDWin.log.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "Diagnostics", "LogPlayerEvents", "Registar eventos que afetam o jogador", "Log events hitting the player", "Ajuda a descobrir a queda a meio dos combos.", "Helps find the fall during combos.", 0.0, 1.0, 1.0, 1.0)
	AddSetting(2, 1, "Menu", "Language", "Idioma Portugues (BR)", "Portugues (BR) language", "Ligado = Portugues. Desligado = English.", "Turn on for Portuguese. Off = English.", 0.0, 1.0, 1.0, 0.0)
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

int Function PageIndex(string a_page)
	if a_page == Pages[0]
		return 0
	elseif a_page == Pages[1]
		return 1
	elseif a_page == Pages[2]
		return 2
	endif
	return -1
EndFunction

Event OnPageReset(string a_page)
	int pg = PageIndex(a_page)
	if pg < 0
		return
	endif
	BuildTable()
	SetCursorFillMode(LEFT_TO_RIGHT)
	int col = 0
	int i = 0
	while i < count
		if sPage[i] == pg
			if sType[i] == 2
				if col == 1
					AddEmptyOption()
					col = 0
				endif
				sOpt[i] = AddHeaderOption(LabelOf(i))
				AddEmptyOption()
			else
				float cur = NahIDWinNative.IniGetF(sSec[i], sKey[i], sDef[i])
				if sType[i] == 1
					sOpt[i] = AddToggleOption(LabelOf(i), cur > 0.5)
				else
					sOpt[i] = AddSliderOption(LabelOf(i), cur, FormatFor(i))
				endif
				col = 1 - col
			endif
		endif
		i += 1
	endWhile
EndEvent

int Function FindOpt(int a_option)
	int pg = PageIndex(CurrentPage)
	int i = 0
	while i < count
		if sOpt[i] == a_option && sPage[i] == pg && sType[i] != 2
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
		if sKey[i] == "Language"
			ForcePageReset()
		endif
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
		if sKey[i] == "Language"
			ForcePageReset()
		endif
	else
		SetSliderOptionValue(a_option, sDef[i], FormatFor(i))
	endif
EndEvent

Event OnOptionHighlight(int a_option)
	int i = FindOpt(a_option)
	if i < 0
		return
	endif
	if IsPT()
		SetInfoText(sInfoPT[i])
	else
		SetInfoText(sInfoEN[i])
	endif
EndEvent
