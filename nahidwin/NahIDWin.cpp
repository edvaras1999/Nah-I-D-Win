// NAH-I-D-WIN (NahIDWin) - Poise + Postura para Skyrim LE 1.9.32.0 + SKSE 1.7.3.
//
// Cada ator tem DUAS barras, ambas separadas da stamina:
//   POISE   (rapida)  - cada golpe tira poise. Sem quebrar: stagger leve (golpe normal) ou medio (golpe
//                       forte). Quebrou: stagger pesado e o poise volta a encher.
//   POSTURA (lenta, estilo Sekiro) - cada golpe enche a postura; ela baixa sozinha com o tempo.
//                       Quando enche por completo: BLEEDOUT (ajoelhado, indefeso durante alguns segundos).
// Golpes bloqueados enchem as duas barras so um pouco e nao dao stagger.
// Nao ha nenhum script em ciclo.
//
// Como se provoca o stagger (StaggerMode no .ini):
//   0 = (por omissao) o plugin envia UM evento de mod ("NahIDWin_Stagger") por stagger; um script
//       Papyrus minimo (NahIDWinBridge) recebe-o e lanca um feitico de stagger do proprio jogo.
//   1 = (experimental, diagnostico) o plugin tenta tocar a animacao de stagger sozinho, sem Papyrus.
//       Pode falhar ou fechar o jogo: serve para descobrirmos o caminho mais leve. Use um save de teste.
//
// O diagnostico (secao [Diagnostics]) regista tudo no log: golpes, contas do poise, eventos enviados,
// confirmacoes do script (e o atraso), e despeja os primeiros bytes das funcoes do jogo que precisamos
// de identificar para a versao sem Papyrus.
//
// v0.7.0 (base: projeto corrigido pelo utilizador): parry = janela de imunidade total decidida so pelo tempo (sem esperar pela marca de "bloqueado").
//   - Dano de vida (hook em Actor::HandleHealthDamage, assinatura verificada): parry = dano zero, furia = reducao,
//     BleedOut = bonus de dano. Dificuldade propria por GMST (fDiffMult*), sem contas de devolucao de vida.
//   - Parry: tecla de bloqueio lida na DLL (receptor de input), janela configuravel.
//   - Furia: tecla G, imunidade a stagger, reducao de dano, sem custo de estamina durante a furia.
//   - Barras: TrueHUD = Postura; linhas proprias de Poise e Furia no jogador e (experimental) por baixo dos inimigos.
// O Papyrus fica so com o stagger (feiticos), BleedOut, FX do parry e exaustao.
//
// Licenca GPL-3.0. A base de compilacao e os enderecos vem do FreeClimb LE / SoftTarget LE (GPL-3.0).

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameRTTI.h"
#include "skse/GameForms.h"
#include "skse/GameData.h"
#include "skse/GameObjects.h"
#include "skse/GameReferences.h"
#include "skse/GameEvents.h"
#include "skse/GameMenus.h"
#include "skse/GameInput.h"
#include "skse/GameSettings.h"
#include "skse/NiTypes.h"
#include "skse/NiObjects.h"
#include "skse/ScaleformMovie.h"
#include "skse/ScaleformCallbacks.h"
#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"
#include "Log.h"
#include "Engine.h"

#include <shlobj.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <set>
#include <functional>

#define PLUGIN_NAME			"NahIDWin"
#define PLUGIN_VERSION_STR	"0.18.1 (alvo Soft Target + raio no ecra; bosses por nivel; magia continua; deflexao; ataque perigoso; execucao; bosses; fogo; bleedout fiavel; vidas extra com bleedout; bosses com 2 vidas; barras novas; Supremo Rage com aura; menu retratil; empurrao corrigido; escudo de stagger;  Poise + Postura + estamina; Inpa Sekiro fundido, menu unico, poisebreak do Inpa na nossa postura)"
#define PLUGIN_LOG_FILE		"\\My Games\\Skyrim\\SKSE\\NahIDWin.log"

static PluginHandle				g_pluginHandle	= kPluginHandle_Invalid;
static SKSEMessagingInterface *	g_messaging		= NULL;
static SKSEPapyrusInterface *	g_papyrus		= NULL;

// O SKSE declara estas funcoes mas so as define em ficheiros que nao compilamos.
// Definimos aqui com os enderecos da LE 1.9.32.0 (confirmados no FreeClimb LE / TrueHUD LE).
const _LookupREFRByHandle LookupREFRByHandle = (_LookupREFRByHandle)0x004A9180;
const _CreateRefHandleByREFR CreateRefHandleByREFR = (_CreateRefHandleByREFR)0x0065CC00;
const _LookupFormByID LookupFormByID = (_LookupFormByID)0x00451A30;
// Evento de golpe do jogo (endereco do SKSE 1.7.3, GameEvents.cpp).
EventDispatcher<TESHitEvent> * g_hitEventDispatcher = (EventDispatcher<TESHitEvent> *)0x012E4F60;

// ---------------------------------------------------------------------------------------------
// Estrutura do evento de mod do SKSE (copia do SKSEModCallbackEvent do SKSE 1.7.3).
struct PoiseModEvent
{
	BSFixedString	eventName;
	BSFixedString	strArg;
	float			numArg;
	TESForm *		sender;

	PoiseModEvent(const char * a_eventName, const char * a_strArg, float a_numArg, TESForm * a_sender) :
		eventName(a_eventName), strArg(a_strArg), numArg(a_numArg), sender(a_sender) {}
};

// ---------------------------------------------------------------------------------------------
// Interface (parcial) da API do TrueHUD LE, versao 1. So as funcoes ate RegisterSpecialResourceFunctions,
// na MESMA ordem do TrueHUDAPI.h (a ordem e o que importa). Nunca derivamos desta classe.
namespace THUD
{
	typedef UInt32 ActorHandle;
	typedef UInt32 ActorValueId;
	enum APIResult : unsigned char { kOK = 0, kNotOwner, kMustKeep, kAlreadyGiven, kAlreadyTaken, kWidgetFailedToLoad, kBadThread };
	typedef std::function<float(Actor *)> SpecialResourceCallback;

	class IVTrueHUD1
	{
	public:
		virtual unsigned long GetTrueHUDThreadId() const noexcept = 0;
		virtual APIResult RequestTargetControl(PluginHandle a_handle) noexcept = 0;
		virtual APIResult RequestSpecialResourceBarsControl(PluginHandle a_handle) noexcept = 0;
		virtual APIResult SetTarget(PluginHandle a_handle, ActorHandle a_actor) noexcept = 0;
		virtual APIResult SetSoftTarget(PluginHandle a_handle, ActorHandle a_actor) noexcept = 0;
		virtual void AddActorInfoBar(ActorHandle a_actor) noexcept = 0;
		virtual void RemoveActorInfoBar(ActorHandle a_actor, unsigned char a_mode) noexcept = 0;
		virtual void AddBoss(ActorHandle a_actor) noexcept = 0;
		virtual void RemoveBoss(ActorHandle a_actor, unsigned char a_mode) noexcept = 0;
		virtual void FlashActorValue(ActorHandle a_actor, ActorValueId a_av, bool a_long) noexcept = 0;
		virtual APIResult FlashActorSpecialBar(PluginHandle a_handle, ActorHandle a_actor, bool a_long) noexcept = 0;
		virtual APIResult RegisterSpecialResourceFunctions(PluginHandle a_handle, SpecialResourceCallback && a_cur, SpecialResourceCallback && a_max, bool a_specialMode, bool a_displayForPlayer) noexcept = 0;
	};
}

namespace
{
	// ------------------------------------------------------------------ afinacoes (.ini)
	struct Config
	{
		int		enabled;			// 1 = ligado
		int		applyPlayer;		// 1 = o jogador tem poise
		int		applyNPCs;			// 1 = os outros atores tem poise
		int		staggerMode;		// 0 = mini-script Papyrus, 1 = nativo (experimental)
		int		nativeSlot;			// modo 1: posicao da funcao NotifyAnimationGraph na tabela do IAnimationGraphManagerHolder
		char	nativeEvent[48];	// modo 1: nome do evento de animacao

		// poise maximo = massa da raca * massMult + armadura * armorFrac (minimo minPoise)
		float	massMult;
		float	armorFrac;
		float	minPoise;
		int		armorAV;			// valor de ator da armadura (39 = DamageResist)

		// dano de poise por golpe
		float	weightMult;			// peso da arma * isto
		float	attackerMassMult;	// massa de quem bate * isto
		float	typeBase[10];		// base por tipo de arma (0 = punhos ... 9 = besta)
		float	unarmedMult;		// dano fisico da raca * isto (punhos e criaturas)
		float	creatureBase;		// base para ataques sem arma (criaturas)
		float	magicDamage;		// dano fixo de feiticos
		float	staggerStatMult;	// o valor "stagger" da arma multiplica o dano por (1 + stagger * isto)
		float	powerMult;			// ataque forte
		float	bashMult;			// golpe de escudo
		float	blockedMult;		// golpe bloqueado
		float	sneakMult;			// ataque furtivo
		float	rangedMult;			// projeteis (flechas)
		float	playerTakenMult;	// o jogador leva isto vezes mais dano de poise

		// regeneracao e quebra
		float	regenSeconds;		// tempo para encher de 0 a cheio
		float	regenDelay;			// segundos sem levar golpe antes de comecar a regenerar
		float	resetFraction;		// poise depois do stagger (1.0 = cheio)
		float	immuneSeconds;		// depois do stagger, o poise nao desce durante este tempo

		// poise: ajustes de dificuldade
		float	poiseMaxScalePlayer;	// o poise maximo do jogador e multiplicado por isto
		float	poiseMaxScaleNpc;		// idem para os outros
		float	npcTakenMult;			// os outros levam isto vezes mais dano de poise e de postura

		// stagger leve / medio
		int		lightEnabled;
		int		lightPlayer;
		int		lightNpc;
		int		lightCooldownMs;	// tempo minimo entre dois staggers leves no mesmo ator
		int		lightCooldownPlayerMs;	// idem, para o jogador
		int		mediumOnPower;		// golpe forte (power attack) sem quebrar da stagger medio

		// postura
		int		postureEnabled;
		float	postureBase;			// postura maxima = base + vidaMax*frac + armadura*frac
		float	postureHealthFrac;
		float	postureArmorFrac;
		float	postureDifficulty;		// 1.0 normal; maior = a postura quebra com mais dificuldade
		float	postureDamageMult;		// dano de postura = dano de poise (sem bloqueio) * isto
		float	postureBlockedMult;		// golpe bloqueado: dano de postura * isto
		float	postureRegenSeconds;	// tempo para baixar de cheia a zero
		float	postureRegenDelay;
		float	postureResetFraction;	// postura depois do BleedOut (0 = vazia)
		float	postureLowHealthBonus;	// com vida baixa a postura enche mais depressa (1.0 = ate 2x com a vida a 0)
		float	postureScalePlayer;		// a postura maxima do jogador * isto
		float	postureScaleNpc;
		float	bleedSeconds;			// duracao do BleedOut
		float	bleedExtraImmune;		// segundos de imunidade depois de levantar

		// estamina: custo de bater (separado do poise)
		int		stamEnabled;
		float	swingCostPlayer;		// por balanco de arma do jogador (feito pelo script ponte, evento de animacao)
		float	hitCostNpc;				// por golpe de NPC/criatura (feito na DLL)
		float	typeMult[13];			// custo por tipo de arma (codigos do Papyrus: 0 punhos, 1 espada, 2 adaga, 3 machado, 4 maca, 5 espada 2m, 6 machado/martelo 2m, 7 arco, 8 cajado, 9 feitico, 10 escudo, 11 tocha, 12 besta)
		float	powerCostMult;			// golpe forte
		float	bashCostMult;			// golpe de escudo
		float	armorCostFactor;		// custo * (1 + armadura/100 * isto)
		int		armorRegenEnabled;		// armadura abranda a regeneracao de estamina do jogador
		float	armorRegenFactor;		// regeneracao % = 100 / (1 + armadura/100 * isto)

		// exaustao: estamina a zero
		int		exhaustEnabled;
		float	exhaustSeconds;			// tempo sem poder atacar nem regenerar
		float	exhaustCooldown;		// minimo entre duas exaustoes
		int		exhaustNpc;				// NPCs tambem (levam stagger pesado)

		// empurrao no stagger pesado (so em condicoes especificas)
		int		kbEnabled;
		float	kbForce;				// 0 = desligado
		int		kbPlayer;				// tambem empurrar o jogador
		int		kbOnPower;				// golpe forte (inclui o de corrida e o carregado)
		int		kbOnGiant;				// atacante de raca grande (massa >= kbGiantMass)
		float	kbGiantMass;
		float	kbVictimMassMax;		// vitima de massa >= isto nao e empurrada (gigantes, dragoes: o ragdoll deles bloqueia)
		int		kbOnHeavyWeapon;		// arma de duas maos ou pesada
		float	kbHeavyWeight;

		// avisos e golpes proprios
		// parry proprio (janela depois de levantar o bloqueio)
		int		inpaMode;				// 1 = o parry/bloqueio/dificuldade sao do Inpa Sekiro (fundido no NahIDWin.esp): a DLL so le o estado dele
		int		logPlayerDamage;		// diagnostico: regista cada dano de vida que o jogador leva (para achar "dano a duplicar")
		float	powerOwnCost;			// custo proprio do golpe forte (0 = so o do jogo, fPowerAttackStaminaPenalty)
		int		parryEnabled;
		int		parryWindowMs;
		float	parryAttackerPosture;	// fracao da postura maxima do atacante que enche
		int		parryAttackerStagger;	// 1 = o atacante leva stagger medio
		float	parryDefPostureRecover;	// fracao da postura maxima que o defensor recupera
		float	parryDefPoiseRecover;	// fracao do poise maximo que o defensor recupera
		int		parryFx;				// faiscas e som (usa os do Inpa Sekiro se instalado)
		int		parryNoDamage;			// 1 = o golpe aparado nao causa dano de vida (intercetado na funcao de dano do motor)
		int		parryRequireHeld;		// 1 = a janela acaba quando largas o bloqueio
		float	parryDefStaminaRecover;	// fracao da estamina maxima que o defensor recupera
		float	parryAttackerStamina;	// estamina que o atacante perde
		int		parryFrontAngle;		// graus de cada lado em que o golpe conta
		int		blockDrain;				// 1 = bloqueio normal gasta estamina do atacante
		float	blockDrainBase;
		float	blockDrainPower;
		int		parryCooldownMs;		// minimo entre duas janelas de parry (evita carregar sem parar)

		// furia: imunidade temporaria em troca de vida ou estamina
		int		rageEnabled;
		int		rageKey;				// tecla (codigo DirectInput) do jogador. 34 = G
		float	rageSeconds;
		float	rageCooldown;
		int		rageCostType;			// 0 = vida, 1 = estamina
		float	rageCostPercent;		// percentagem do maximo
		float	rageDamageReduction;	// % do dano de vida que o jogador deixa de levar durante a furia
		float	rageNpcDamageReduction;	// idem para NPCs
		float	rageDamageBonus;		// % de dano a mais que o jogador causa em furia
		float	rageNpcDamageBonus;		// idem NPCs
		float	rageLifeCost;			// % da vida maxima (custo em vida)
		int		rageZeroStamina;		// a estamina do jogador nao desce durante a furia
		int		rageStaggerShield;		// 1 = sem stagger durante a furia
		// vidas extra do jogador
		int		livesEnabled, livesLevel2, livesLevel3;
		float	livesHealthPct, livesGoldPct, livesInvuln;
		// bosses (2 vidas): unico + nivel minimo
		int		execEnabled, execBigKill;			// execucao garantida (evento para o Papyrus); gigantes/dragoes: matar
		float	bossPoiseMult, bossPostureMult, bigPoiseMult, bigPostureMult;
		char	supAnim[48];						// evento de animacao ao ativar o Supremo (vazio = nenhum)
		int		livesEssential;	// 1 = metodo do SkyTweak: jogador essencial enquanto ha vidas (bleedout verdadeiro do jogo)
		int		bossEnabled, bossLevelMargin;
		float	bigRageMult;
		int		magicDotMs, deflectEnabled;
		int		perilEnabled, perilChance, perilWindowMs;
		float	perilCooldown, perilDmgMult, perilParryPosture, perilRange;
		float	magicDotMult;
		float	bossHealthPct, bossInvuln;
		// Supremo Rage (aura)
		int		supEnabled, supKey;
		float	supSeconds, supCooldown, supCostPerSec, supRadius, supDps;
		int		iconsAlways;
		int		rageNpc;				// NPCs ativam sozinhos
		float	rageNpcPosture;			// ativa quando a postura passa desta fracao
		float	rageNpcPoise;			// ... ou o poise desce abaixo desta fracao
		int		rageNpcChance;			// probabilidade (%) por golpe recebido quando as condicoes se cumprem
		float	rageNpcCooldown;
		float	rageNpcCostPercent;		// 0 = gratis para NPCs

		int		bar2Enabled;			// EXPERIMENTAL: segunda barra desenhada por codigo (so para o jogador)
		float	bar2X, bar2Y, bar2W, bar2H;
		int		barPoise;				// mostrar a linha do poise
		int		barRage;				// mostrar a linha da furia
		int		enemyBars;				// EXPERIMENTAL: as mesmas linhas por baixo dos inimigos
		float	enemyBarW, enemyBarH, enemyBarOffsetY;
		int		enemyBarMax;			// quantas barras de inimigos no maximo
		int		language;				// 0 = ingles, 1 = portugues (texto do MCM e avisos)

		// vida (estilo Sekiro): o dano de vida nao depende da dificuldade do jogo
		int		healthEnabled;
		float	healthTaken;			// o jogador leva isto vezes o dano base (em todas as dificuldades)
		float	healthDealt;			// o jogador causa isto vezes o dano base
		float	bleedDamageMult;		// dano extra que um alvo em BleedOut leva (1.0 = igual)

		float	poiseWarnFraction;		// a barra pisca quando o poise desce abaixo disto (0 = desligado)
		int		ignoreOwnSpells;		// 1 = nao contar os feiticos de stagger deste mod como dano
		// TrueHUD
		int		hudEnabled;
		int		hudPlayer;			// mostrar a barra tambem para o jogador
		int		hudFlash;			// piscar a barra quando algo quebra
		int		hudBar;				// 0 = barra do poise, 1 = barra da postura, 2 = a mais perto de quebrar
		// HUD proprio (NahIDWinHUD.swf)
		int		wEnabled;			// 1 = barras proprias de postura e furia
		int		wPlayer;			// barras do jogador
		int		wPlayerAlways;		// 1 = sempre visiveis; 0 = so em combate/furia
		int		wTarget;			// barras do alvo
		int		wTargetRadius, wTargetStickyMs, wTargetMaxDist;	// raio (px) e tempo que a barra fica apos o alvo sair
		int		wIcon;				// 1 = icone da furia ativa
		int		wAnchor;			// 1 = alvo preso a barra de vida do jogo; 0 = posicao fixa
		int		wStyle;				// 0 plana, 1 Sekiro (enche dos dois lados)
		int		wPosH, wPosV, wTPosH, wTPosV;	// 0/1/2 = esquerda/centro/direita, cima/centro/baixo
		float	wShiftX, wShiftY, wTShiftX, wTShiftY;
		float	wScreenW, wScreenH;
		float	wPX, wPY, wPW, wPH;	// posicao/tamanho das barras do jogador
		float	wTOffX, wTOffY, wTW, wTH;	// barras do alvo (relativas ao nome do alvo)
		float	wFX, wFY;			// posicao se a barra de vida do jogo nao existir

		// diagnostico
		int		diag;				// 1 = diagnostico ligado
		int		logHitsMax;			// quantos golpes escrever no log por sessao (0 = nenhum)
		int		summarySec;			// de quantos em quantos segundos escrever o resumo (0 = nunca)
		int		dumpVtable;			// 1 = despejar a tabela de funcoes de animacao do jogador
		int		logPlayerEvents;	// 1 = registar tudo o que faz o jogador cair ou cambalear
	};
	Config g_cfg;

	char g_iniPath[MAX_PATH] = "";
	FILETIME g_iniTime = { 0, 0 };
	DWORD g_lastReloadCheck = 0;

	void ResolveIniPath()
	{
		HMODULE hm = NULL;
		if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							   (LPCSTR)&ResolveIniPath, &hm) || !hm)
		{
			strcpy_s(g_iniPath, sizeof(g_iniPath), "Data\\SKSE\\Plugins\\NahIDWin.ini");
			return;
		}
		GetModuleFileNameA(hm, g_iniPath, MAX_PATH);
		char * dot = strrchr(g_iniPath, '.');
		if(dot) strcpy_s(dot, (size_t)(g_iniPath + MAX_PATH - dot), ".ini");
		else strcat_s(g_iniPath, sizeof(g_iniPath), ".ini");
	}

	float IniF(const char * section, const char * key, float def)
	{
		char buf[64];
		GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), g_iniPath);
		return buf[0] ? (float)atof(buf) : def;
	}

	int IniI(const char * section, const char * key, int def)
	{
		char buf[64];
		GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), g_iniPath);
		return buf[0] ? atoi(buf) : def;
	}

	void ApplyHealthGMST();

	void LoadConfig()
	{
		Config & c = g_cfg;
		c.enabled		= IniI("General", "Enabled", 1);
		c.applyPlayer	= IniI("General", "Player", 1);
		c.applyNPCs		= IniI("General", "NPCs", 1);

		c.staggerMode	= IniI("Stagger", "StaggerMode", 0);
		c.nativeSlot	= IniI("Stagger", "NativeSlot", 1);
		GetPrivateProfileStringA("Stagger", "NativeEvent", "staggerStart", c.nativeEvent, sizeof(c.nativeEvent), g_iniPath);
		c.resetFraction	= IniF("Stagger", "ResetFraction", 1.0f);
		c.immuneSeconds	= IniF("Stagger", "ImmuneSeconds", 2.5f);
		c.lightEnabled	= IniI("Stagger", "LightEnabled", 1);
		c.lightPlayer	= IniI("Stagger", "LightPlayer", 1);
		c.lightNpc		= IniI("Stagger", "LightNPCs", 1);
		c.lightCooldownMs = IniI("Stagger", "LightCooldownMs", 700);
		c.lightCooldownPlayerMs = IniI("Stagger", "LightCooldownPlayerMs", 1500);
		c.mediumOnPower	= IniI("Stagger", "MediumOnPowerAttack", 1);

		c.massMult		= IniF("Poise", "MassMult", 20.0f);
		c.armorFrac		= IniF("Poise", "ArmorFraction", 0.5f);
		c.minPoise		= IniF("Poise", "MinPoise", 10.0f);
		c.armorAV		= IniI("Poise", "ArmorActorValue", 39);
		c.regenSeconds	= IniF("Poise", "RegenSeconds", 30.0f);
		c.regenDelay	= IniF("Poise", "RegenDelaySeconds", 2.0f);
		c.playerTakenMult = IniF("Poise", "PlayerTakenMult", 1.5f);
		c.poiseMaxScalePlayer = IniF("Poise", "MaxScalePlayer", 4.5f);
		c.poiseMaxScaleNpc = IniF("Poise", "MaxScaleNPC", 9.0f);
		c.npcTakenMult	= IniF("Poise", "NPCTakenMult", 2.5f);

		c.postureEnabled	= IniI("Posture", "Enabled", 1);
		c.postureBase		= IniF("Posture", "Base", 100.0f);
		c.postureHealthFrac	= IniF("Posture", "HealthFraction", 0.3f);
		c.postureArmorFrac	= IniF("Posture", "ArmorFraction", 0.3f);
		c.postureDifficulty	= IniF("Posture", "Difficulty", 1.0f);
		c.postureDamageMult	= IniF("Posture", "DamageMult", 1.0f);
		c.postureBlockedMult = IniF("Posture", "BlockedMult", 0.12f);
		c.postureRegenSeconds = IniF("Posture", "RegenSeconds", 60.0f);
		c.postureRegenDelay	= IniF("Posture", "RegenDelaySeconds", 3.0f);
		c.postureResetFraction = IniF("Posture", "ResetFraction", 0.0f);
		c.postureLowHealthBonus = IniF("Posture", "LowHealthBonus", 1.0f);
		c.postureScalePlayer = IniF("Posture", "ScalePlayer", 5.0f);
		c.postureScaleNpc	= IniF("Posture", "ScaleNPC", 5.0f);
		c.bleedSeconds		= IniF("Posture", "BleedOutSeconds", 4.0f);
		c.bleedExtraImmune	= IniF("Posture", "BleedOutImmuneSeconds", 5.0f);

		c.stamEnabled		= IniI("Stamina", "Enabled", 1);
		c.swingCostPlayer	= IniF("Stamina", "SwingCostPlayer", 10.0f);
		c.hitCostNpc		= IniF("Stamina", "HitCostNPC", 10.0f);
		static const float typeMultDefault[13] = { 0.6f, 1.0f, 0.6f, 1.2f, 1.3f, 1.6f, 1.8f, 0.8f, 0.6f, 0.0f, 1.0f, 0.5f, 0.8f };
		for(int i = 0; i < 13; i++)
		{
			char key[24];
			_snprintf_s(key, sizeof(key), _TRUNCATE, "TypeMult%d", i);
			c.typeMult[i] = IniF("Stamina", key, typeMultDefault[i]);
		}
		c.powerCostMult		= IniF("Stamina", "PowerAttackMult", 1.5f);
		c.bashCostMult		= IniF("Stamina", "BashMult", 1.0f);
		c.armorCostFactor	= IniF("Stamina", "ArmorCostFactor", 0.25f);
		c.armorRegenEnabled	= IniI("Stamina", "ArmorRegenEnabled", 1);
		c.armorRegenFactor	= IniF("Stamina", "ArmorRegenFactor", 0.3f);

		c.exhaustEnabled	= IniI("Exhaustion", "Enabled", 1);
		c.exhaustSeconds	= IniF("Exhaustion", "Seconds", 2.5f);
		c.exhaustCooldown	= IniF("Exhaustion", "CooldownSeconds", 8.0f);
		c.exhaustNpc		= IniI("Exhaustion", "NPCs", 1);

		c.kbEnabled			= IniI("Knockback", "Enabled", 1);
		c.kbForce			= IniF("Knockback", "Force", 1.0f);
		c.kbPlayer			= IniI("Knockback", "Player", 1);
		c.kbOnPower			= IniI("Knockback", "OnPowerAttack", 1);
		c.kbOnGiant			= IniI("Knockback", "OnGiantCreature", 1);
		c.kbGiantMass		= IniF("Knockback", "GiantMassMin", 3.0f);
		c.kbVictimMassMax	= IniF("Knockback", "VictimMassMax", 3.0f);
		c.kbOnHeavyWeapon	= IniI("Knockback", "OnHeavyWeapon", 0);
		c.kbHeavyWeight		= IniF("Knockback", "HeavyWeaponMinWeight", 15.0f);

		c.poiseWarnFraction	= IniF("TrueHUD", "PoiseWarnFraction", 0.3f);

		c.parryEnabled		= IniI("Parry", "Enabled", 1);
		c.inpaMode			= IniI("Inpa", "Mode", 0);
		c.logPlayerDamage	= IniI("Inpa", "LogPlayerDamage", 1);
		c.powerOwnCost		= IniF("Stamina", "PowerAttackOwnCost", 0.0f);
		c.parryWindowMs		= IniI("Parry", "WindowMs", 250);
		c.parryAttackerPosture = IniF("Parry", "AttackerPostureFraction", 0.10f);
		c.parryAttackerStagger = IniI("Parry", "AttackerStagger", 1);
		c.parryDefPostureRecover = IniF("Parry", "DefenderPostureRecoverFraction", 0.15f);
		c.parryDefPoiseRecover = IniF("Parry", "DefenderPoiseRecoverFraction", 0.25f);
		c.parryFx			= IniI("Parry", "Effects", 1);
		c.parryNoDamage		= IniI("Parry", "NoDamage", 1);
		if(c.inpaMode) { c.parryFx = 0; c.parryNoDamage = 0; }
		c.parryCooldownMs	= IniI("Parry", "CooldownMs", 200);
		c.parryRequireHeld	= IniI("Parry", "RequireHeld", 1);
		c.parryDefStaminaRecover = IniF("Parry", "DefenderStaminaRecoverFraction", 0.25f);
		c.parryAttackerStamina = IniF("Parry", "AttackerStaminaDrain", 20.0f);
		c.parryFrontAngle	= IniI("Parry", "FrontAngleDegrees", 100);
		c.blockDrain		= IniI("Block", "DrainEnabled", 1);
		c.blockDrainBase	= IniF("Block", "AttackerStamina", 10.0f);
		c.blockDrainPower	= IniF("Block", "PowerAttackMult", 1.5f);

		c.rageEnabled		= IniI("Rage", "Enabled", 1);
		c.rageKey			= IniI("Rage", "Key", 34);
		c.rageSeconds		= IniF("Rage", "DurationSeconds", 5.0f);
		c.rageCooldown		= IniF("Rage", "CooldownSeconds", 30.0f);
		c.rageCostType		= IniI("Rage", "CostType", 0);
		c.rageCostPercent	= IniF("Rage", "CostPercent", 15.0f);
		c.rageDamageReduction = IniF("Rage", "ExtraReduction", 40.0f);
		c.rageNpcDamageReduction = IniF("Rage", "NPCExtraReduction", 30.0f);
		c.rageDamageBonus = IniF("Rage", "DamageBonus", 20.0f);
		c.rageNpcDamageBonus = IniF("Rage", "NPCDamageBonus", 10.0f);
		c.rageLifeCost = IniF("Rage", "LifeCostPercent", 20.0f);
		c.rageStaggerShield = IniI("Rage", "StaggerImmune", 1);
		c.livesEnabled	= IniI("Lives", "Enabled", 1);
		c.livesLevel2	= IniI("Lives", "Level2", 30);
		c.livesLevel3	= IniI("Lives", "Level3", 60);
		c.livesHealthPct = IniF("Lives", "HealthPercent", 50.0f);
		c.livesGoldPct	= IniF("Lives", "GoldLossPercent", 25.0f);
		c.livesInvuln	= IniF("Lives", "InvulnSeconds", 3.0f);
		c.livesEssential = IniI("Lives", "EssentialBleedout", 1);
		c.execEnabled	= IniI("Execution", "Enabled", 1);
		c.execBigKill	= IniI("Execution", "KillBigCreatures", 1);
		c.bossPoiseMult	= IniF("Bosses", "PoiseMult", 1.5f);
		c.bossPostureMult = IniF("Bosses", "PostureMult", 1.5f);
		c.bigPoiseMult	= IniF("BigCreatures", "PoiseMult", 1.5f);
		c.bigPostureMult = IniF("BigCreatures", "PostureMult", 1.5f);
		GetPrivateProfileStringA("Supreme", "ActivationAnim", "attackPowerStartForward", c.supAnim, sizeof(c.supAnim), g_iniPath);
		c.bossEnabled	= IniI("Bosses", "Enabled", 1);
		c.bossLevelMargin	= IniI("Bosses", "LevelMargin", 7);
		c.bigRageMult	= IniF("BigCreatures", "RageDurationMult", 1.5f);
		c.magicDotMs	= IniI("Magic", "DotIntervalMs", 1000);
		c.magicDotMult	= IniF("Magic", "DotPoiseMult", 0.5f);
		c.deflectEnabled = IniI("Magic", "DeflectEnabled", 1);
		c.perilEnabled	= IniI("Perilous", "Enabled", 1);
		c.perilChance	= IniI("Perilous", "ChancePercent", 25);
		c.perilWindowMs	= IniI("Perilous", "WindowMs", 1300);
		c.perilCooldown	= IniF("Perilous", "CooldownSeconds", 12.0f);
		c.perilDmgMult	= IniF("Perilous", "DamageMult", 1.5f);
		c.perilParryPosture = IniF("Perilous", "ParryPosture", 0.5f);
		c.perilRange	= IniF("Perilous", "Range", 700.0f);
		c.bossHealthPct	= IniF("Bosses", "HealthPercent", 100.0f);
		c.bossInvuln	= IniF("Bosses", "InvulnSeconds", 3.0f);
		c.supEnabled	= IniI("Supreme", "Enabled", 1);
		c.supKey		= IniI("Supreme", "Key", 35);
		c.supSeconds	= IniF("Supreme", "DurationSeconds", 10.0f);
		c.supCooldown	= IniF("Supreme", "CooldownSeconds", 500.0f);
		c.supCostPerSec	= IniF("Supreme", "CostPercentPerSecond", 2.0f);
		c.supRadius		= IniF("Supreme", "Radius", 300.0f);
		c.supDps		= IniF("Supreme", "DamagePerSecond", 8.0f);
		c.iconsAlways	= IniI("HUD", "IconsAlways", 1);
		c.rageZeroStamina	= IniI("Rage", "ZeroStamina", 1);
		c.rageNpc			= IniI("Rage", "NPCs", 1);
		c.rageNpcPosture	= IniF("Rage", "NPCPostureTrigger", 0.8f);
		c.rageNpcPoise		= IniF("Rage", "NPCPoiseTrigger", 0.15f);
		c.rageNpcChance		= IniI("Rage", "NPCChancePercent", 50);
		c.rageNpcCooldown	= IniF("Rage", "NPCCooldownSeconds", 45.0f);
		c.rageNpcCostPercent = IniF("Rage", "NPCCostPercent", 0.0f);
		c.ignoreOwnSpells	= IniI("Diagnostics", "IgnoreOwnSpells", 1);

		c.weightMult		= IniF("Damage", "WeightMult", 0.5f);
		c.attackerMassMult	= IniF("Damage", "AttackerMassMult", 1.0f);
		static const float typeDefault[10] = { 3.0f, 5.0f, 3.0f, 6.0f, 7.0f, 10.0f, 12.0f, 4.0f, 3.0f, 7.0f };
		for(int i = 0; i < 10; i++)
		{
			char key[24];
			_snprintf_s(key, sizeof(key), _TRUNCATE, "TypeBase%d", i);
			c.typeBase[i] = IniF("Damage", key, typeDefault[i]);
		}
		c.unarmedMult		= IniF("Damage", "UnarmedMult", 1.0f);
		c.creatureBase		= IniF("Damage", "CreatureBase", 6.0f);
		c.magicDamage		= IniF("Damage", "MagicDamage", 4.0f);
		c.staggerStatMult	= IniF("Damage", "WeaponStaggerMult", 1.0f);
		c.powerMult			= IniF("Damage", "PowerAttackMult", 2.0f);
		c.bashMult			= IniF("Damage", "BashMult", 1.5f);
		c.blockedMult		= IniF("Damage", "BlockedMult", 0.15f);
		c.sneakMult			= IniF("Damage", "SneakMult", 1.0f);
		c.rangedMult		= IniF("Damage", "RangedMult", 0.8f);

		c.hudEnabled	= IniI("TrueHUD", "Enabled", 1);
		c.hudPlayer		= IniI("TrueHUD", "ShowForPlayer", 1);
		c.hudFlash		= IniI("TrueHUD", "FlashOnBreak", 1);
		c.hudBar		= IniI("TrueHUD", "Bar", 0);
		c.bar2Enabled	= IniI("TrueHUD", "SecondBar", 1);
		c.barPoise		= IniI("TrueHUD", "BarPoise", 1);
		c.barRage		= IniI("TrueHUD", "BarRage", 1);
		c.enemyBars		= IniI("TrueHUD", "EnemyBars", 1);
		c.enemyBarW		= IniF("TrueHUD", "EnemyBarWidth", 110.0f);
		c.enemyBarH		= IniF("TrueHUD", "EnemyBarHeight", 4.0f);
		c.enemyBarOffsetY = IniF("TrueHUD", "EnemyBarOffsetY", 14.0f);
		c.enemyBarMax	= IniI("TrueHUD", "EnemyBarMax", 8);
		c.language		= IniI("Menu", "Language", 0);
		c.healthEnabled	= IniI("Health", "Enabled", 1);
		if(c.inpaMode) c.healthEnabled = 0;		// dificuldade/dano: do Inpa (GMSTs dele)
		c.healthTaken	= IniF("Health", "TakenMult", 1.0f);
		c.healthDealt	= IniF("Health", "DealtMult", 1.0f);
		c.bleedDamageMult = IniF("Health", "BleedOutDamageMult", 1.0f);
		c.wEnabled		= IniI("HUD", "Enabled", 1);
		c.wPlayer		= IniI("HUD", "Player", 1);
		c.wPlayerAlways	= IniI("HUD", "PlayerAlways", 0);
		c.wTarget		= IniI("HUD", "Target", 1);
		c.wTargetRadius	= IniI("HUD", "TargetRadius", 150);
		c.wTargetStickyMs = IniI("HUD", "TargetStickyMs", 3000);
		c.wTargetMaxDist = IniI("HUD", "TargetMaxDistance", 2000);
		c.wIcon			= IniI("HUD", "RageIcon", 1);
		c.wAnchor		= IniI("HUD", "TargetAnchor", 0);
		c.wStyle		= IniI("HUD", "BarStyle", 1);
		c.wPosH			= IniI("HUD", "PlayerPosH", 0);
		c.wPosV			= IniI("HUD", "PlayerPosV", 2);
		c.wShiftX		= IniF("HUD", "PlayerShiftX", 120.0f);
		c.wShiftY		= IniF("HUD", "PlayerShiftY", -117.0f);
		c.wTPosH		= IniI("HUD", "TargetPosH", 1);
		c.wTPosV		= IniI("HUD", "TargetPosV", 0);
		c.wTShiftX		= IniF("HUD", "TargetShiftX", 0.0f);
		c.wTShiftY		= IniF("HUD", "TargetShiftY", 55.0f);
		c.wScreenW		= IniF("HUD", "ScreenWidth", 1280.0f);
		c.wScreenH		= IniF("HUD", "ScreenHeight", 720.0f);
		c.wPX = 0; c.wPY = 0;
		c.wPW			= IniF("HUD", "PlayerWidth", 200.0f);
		c.wPH			= IniF("HUD", "BarHeight", 5.0f);
		c.wTOffX		= IniF("HUD", "TargetOffsetX", 0.0f);
		c.wTOffY		= IniF("HUD", "TargetOffsetY", 22.0f);
		c.wTW			= IniF("HUD", "TargetWidth", 0.0f);
		c.wTH			= IniF("HUD", "TargetBarHeight", 5.0f);
		c.wFX			= IniF("HUD", "TargetFallbackX", 540.0f);
		c.wFY			= IniF("HUD", "TargetFallbackY", 60.0f);
		c.bar2X			= IniF("TrueHUD", "SecondBarX", 490.0f);
		c.bar2Y			= IniF("TrueHUD", "SecondBarY", 640.0f);
		c.bar2W			= IniF("TrueHUD", "SecondBarWidth", 300.0f);
		c.bar2H			= IniF("TrueHUD", "SecondBarHeight", 6.0f);

		c.diag			= IniI("Diagnostics", "Enabled", 1);
		c.logHitsMax	= IniI("Diagnostics", "LogHitsMax", 400);
		c.summarySec	= IniI("Diagnostics", "SummarySeconds", 30);
		c.dumpVtable	= IniI("Diagnostics", "DumpVtable", 0);
		c.logPlayerEvents = IniI("Diagnostics", "LogPlayerEvents", 1);

		WIN32_FILE_ATTRIBUTE_DATA fad;
		if(GetFileAttributesExA(g_iniPath, GetFileExInfoStandard, &fad)) g_iniTime = fad.ftLastWriteTime;

		_MESSAGE("config: enabled=%d player=%d npcs=%d staggerMode=%d (slot %d, '%s') posture=%d (difficulty %.2f) hud=%d bar=%d diag=%d  [%s]",
			c.enabled, c.applyPlayer, c.applyNPCs, c.staggerMode, c.nativeSlot, c.nativeEvent, c.postureEnabled, c.postureDifficulty, c.hudEnabled, c.hudBar, c.diag, g_iniPath);
		ApplyHealthGMST();
	}

	void MaybeReloadConfig(DWORD now)
	{
		if(now - g_lastReloadCheck < 2000) return;
		g_lastReloadCheck = now;
		WIN32_FILE_ATTRIBUTE_DATA fad;
		if(!GetFileAttributesExA(g_iniPath, GetFileExInfoStandard, &fad)) return;
		if(fad.ftLastWriteTime.dwLowDateTime != g_iniTime.dwLowDateTime || fad.ftLastWriteTime.dwHighDateTime != g_iniTime.dwHighDateTime)
		{
			_MESSAGE("ini changed on disk - reloading");
			LoadConfig();
		}
	}

	// ------------------------------------------------------------------ estado do poise
	struct PoiseState
	{
		float	cur;			// poise atual (desce com os golpes)
		float	max;
		DWORD	lastHit;
		DWORD	lastTick;
		DWORD	immuneUntil;	// depois de quebrar o poise
		float	pCur;			// postura atual (SOBE com os golpes; ao chegar a pMax: BleedOut)
		float	pMax;
		DWORD	pLastHit;
		DWORD	pTick;
		DWORD	bleedUntil;		// em BleedOut ate este momento (e imune ate bleedImmuneUntil)
		DWORD	bleedImmuneUntil;
		DWORD	lightUntil;		// proximo stagger leve permitido a partir daqui
		DWORD	rageUntil;		// furia (imune) ate este momento
		DWORD	rageReadyAt;	// proxima furia permitida a partir daqui
		DWORD	exhaustReadyAt;	// proxima exaustao permitida a partir daqui
		DWORD	invulnUntil;	// sem dano de vida ate aqui (vida extra / bleedout de boss)
		int		bossUsed;		// boss: a vida extra ja foi gasta
		DWORD	lastExec;		// ultima execucao pedida
	};
	typedef std::map<UInt32, PoiseState> StateMap;

	struct LockHolder { LockHolder() { InitializeCriticalSection(&cs); } CRITICAL_SECTION cs; };
	LockHolder	g_lockHolder;
	StateMap	g_states;

	// contadores e estatisticas (diagnostico)
	volatile DWORD	g_powerUntil = 0;		// um golpe forte esta a comecar (o balanco seguinte nao paga duas vezes)


	volatile LONG	g_parries = 0, g_rages = 0;
	volatile DWORD	g_parryUntil = 0;		// janela do parry: o jogador carregou no bloqueio ha pouco
	volatile DWORD	g_parryReadyAt = 0;		// ate quando a janela esta em recarga
	volatile LONG	g_parryHitDone = 0;		// o parry desta janela ja foi confirmado
	struct PerilSlot { int prev; DWORD ready; DWORD until; };
	std::map<UInt32, PerilSlot> g_peril;
	DWORD g_perilTick = 0;
	std::map<UInt32, DWORD> g_hostile;		// inimigos do jogador: id -> ultimo golpe trocado com o jogador
	volatile DWORD	g_execTick = 0;			// ultima execucao pedida (vigia da vida do jogador)
	struct FleeSlot { int combat; DWORD lastLog; };
	std::map<UInt32, FleeSlot> g_flee;
	DWORD g_fleeTick = 0;
	volatile LONG	g_deflectOn = 0;		// parry numa magia: enquanto segurar o bloqueio, a magia e devolvida
	volatile DWORD	g_deflectStart = 0;
	volatile UInt32	g_parryAtkId = 0;		// anti-duplicacao: um ataque conta um parry so
	volatile DWORD	g_parryAtkTick = 0;
	struct MagicAtk { UInt32 id; DWORD tick; };
	MagicAtk		g_magicAtk[4];			// atacantes de magia recentes do jogador
	struct DotEntry { UInt32 key; DWORD tick; };
	DotEntry		g_dot[16];				// limitador de dano continuo (por alvo+origem+feitico)
	DWORD			g_reflectTick[4];
	UInt32			g_reflectId[4];
	volatile LONG	g_blockHeld = 0;		// impede rearmar o parry enquanto o botao continua premido
	volatile LONG	g_blockInputLogCount = 0;
	volatile LONG	g_hdCalls = 0, g_hdParryZero = 0, g_hdRage = 0, g_hdBleed = 0, g_hdLogged = 0, g_hdPlayerLogged = 0;
	volatile LONG	g_postureBreaks = 0, g_lightSent = 0, g_mediumSent = 0;
	volatile LONG	g_hits = 0, g_applied = 0, g_breaks = 0, g_sent = 0, g_acks = 0, g_faults = 0, g_scriptReady = 0;
	volatile LONG	g_seq = 0;
	DWORD			g_latSum = 0, g_latMax = 0, g_latCount = 0;	// protegidos pelo lock
	DWORD			g_sentTick[64];
	LONG			g_sentSeq[64];
	DWORD			g_lastSummary = 0;
	volatile LONG	g_hitLogged = 0;
	bool			g_vtableDumped = false;

	void Lock()		{ EnterCriticalSection(&g_lockHolder.cs); }
	void Unlock()	{ LeaveCriticalSection(&g_lockHolder.cs); }

	// Chamar com o lock tomado.
	void Regen(PoiseState & st, DWORD now)
	{
		if(g_cfg.regenSeconds > 0.0f && st.cur < st.max)
		{
			DWORD from = st.lastHit + (DWORD)(g_cfg.regenDelay * 1000.0f);
			if(from < st.lastTick) from = st.lastTick;
			if(now > from)
			{
				st.cur += (st.max / g_cfg.regenSeconds) * ((float)(now - from) / 1000.0f);
				if(st.cur > st.max) st.cur = st.max;
			}
		}
		st.lastTick = now;
	}

	// Chamar com o lock tomado. A postura baixa sozinha depois de uma pausa.
	void RegenPosture(PoiseState & st, DWORD now)
	{
		if(g_cfg.postureRegenSeconds > 0.0f && st.pCur > 0.0f && st.pMax > 0.0f && now >= st.bleedUntil)
		{
			DWORD from = st.pLastHit + (DWORD)(g_cfg.postureRegenDelay * 1000.0f);
			if(from < st.pTick) from = st.pTick;
			if(now > from)
			{
				st.pCur -= (st.pMax / g_cfg.postureRegenSeconds) * ((float)(now - from) / 1000.0f);
				if(st.pCur < 0.0f) st.pCur = 0.0f;
			}
		}
		st.pTick = now;
	}

	// Chamar com o lock tomado.
	PoiseState & FindOrCreate(UInt32 id, float maxPoise, float maxPosture, DWORD now)
	{
		StateMap::iterator it = g_states.find(id);
		if(it == g_states.end())
		{
			if(g_states.size() > 1500)
			{
				for(StateMap::iterator p = g_states.begin(); p != g_states.end(); )
				{
					if(now - p->second.lastTick > 180000) g_states.erase(p++);
					else ++p;
				}
			}
			PoiseState s;
			s.cur = maxPoise; s.max = maxPoise; s.lastHit = 0; s.lastTick = now; s.immuneUntil = 0;
			s.pCur = 0.0f; s.pMax = maxPosture; s.pLastHit = 0; s.pTick = now; s.bleedUntil = 0; s.bleedImmuneUntil = 0; s.lightUntil = 0; s.rageUntil = 0; s.rageReadyAt = 0; s.exhaustReadyAt = 0; s.invulnUntil = 0; s.bossUsed = 0; s.lastExec = 0;
			it = g_states.insert(std::make_pair(id, s)).first;
		}
		else
		{
			it->second.max = maxPoise;
			if(it->second.cur > maxPoise) it->second.cur = maxPoise;
			it->second.pMax = maxPosture;
			if(it->second.pCur > maxPosture) it->second.pCur = maxPosture;
		}
		return it->second;
	}

	// ------------------------------------------------------------------ leitura do jogo (protegida)
	struct ActorStats
	{
		UInt32	id;
		float	mass;
		float	armor;
		float	unarmed;
		float	hp;
		float	hpMax;
		bool	alive;
		bool	isPlayer;
		bool	boss;
	};

	// Boss = NPC com nivel >= nivel do jogador + margem (e que nao e companheiro do jogador).
	bool IsBossActor(Actor * a)
	{
		__try
		{
			if(!g_cfg.bossEnabled || !a || a->formID == 0x14) return false;
			if(a->flags1 & Actor::kFlags_IsPlayerTeammate) return false;
			TESForm * b = a->baseForm;
			if(!b || b->formType != kFormType_NPC) return false;
			Actor * pl = (Actor *)Engine::Player();
			if(!pl) return false;
			// boss = inimigo com N niveis acima do jogador (margem configuravel)
			return (int)CALL_MEMBER_FN(a, GetLevel)() >= (int)CALL_MEMBER_FN(pl, GetLevel)() + g_cfg.bossLevelMargin;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	// Sem objetos com destrutor: pode usar __try.
	bool ReadActorStats(Actor * a, ActorStats * out)
	{
		__try
		{
			out->id = a->formID;
			out->isPlayer = (a->formID == 0x14);
			float mass = 1.0f, unarmed = 0.0f;
			TESRace * race = a->race;
			if(race && race->formType == kFormType_Race)
			{
				mass = race->data.mass;
				unarmed = race->data.unarmedDamage;
			}
			out->mass = (mass > 0.0f && mass < 10000.0f) ? mass : 1.0f;
			out->unarmed = (unarmed > 0.0f && unarmed < 10000.0f) ? unarmed : 0.0f;
			out->armor = a->actorValueOwner.GetCurrent((UInt32)g_cfg.armorAV);
			if(!(out->armor >= 0.0f && out->armor < 100000.0f)) out->armor = 0.0f;
			out->hp = a->actorValueOwner.GetCurrent(24);		// 24 = Health
			out->hpMax = a->actorValueOwner.GetMaximum(24);
			if(!(out->hpMax > 0.0f && out->hpMax < 100000.0f)) out->hpMax = 100.0f;
			if(!(out->hp >= 0.0f && out->hp <= out->hpMax * 4.0f)) out->hp = out->hpMax;
			out->alive = (Engine::LifeState(a) == 0);
			out->boss = IsBossActor(a);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	float ComputeMax(const ActorStats & s)
	{
		float m = s.mass * g_cfg.massMult + s.armor * g_cfg.armorFrac;
		if(m < g_cfg.minPoise) m = g_cfg.minPoise;
		m *= s.isPlayer ? g_cfg.poiseMaxScalePlayer : g_cfg.poiseMaxScaleNpc;
		if(!s.isPlayer && s.boss) m *= g_cfg.bossPoiseMult;
		if(!s.isPlayer && s.mass >= g_cfg.kbGiantMass) m *= g_cfg.bigPoiseMult;
		return (m < 5.0f) ? 5.0f : m;
	}

	// Postura maxima: base + vida maxima (cresce com o nivel) + armadura, vezes a dificuldade.
	float ComputeMaxPosture(const ActorStats & s)
	{
		float m = g_cfg.postureBase + s.hpMax * g_cfg.postureHealthFrac + s.armor * g_cfg.postureArmorFrac;
		m *= s.isPlayer ? g_cfg.postureScalePlayer : g_cfg.postureScaleNpc;
		m *= (g_cfg.postureDifficulty > 0.05f) ? g_cfg.postureDifficulty : 0.05f;	// dificuldade maior = postura maior
		if(!s.isPlayer && s.boss) m *= g_cfg.bossPostureMult;
		if(!s.isPlayer && s.mass >= g_cfg.kbGiantMass) m *= g_cfg.bigPostureMult;
		return (m < 10.0f) ? 10.0f : m;
	}

	struct HitInfo
	{
		TESObjectREFR *	victimRef;
		UInt32			attackerId;
		ActorStats		victim;
		ActorStats		attacker;
		bool			attackerIsActor;
		Actor *			attackerActor;
		bool			ownSpell;
		UInt32			flags;
		UInt32			sourceId;
		UInt32			projId;
		int				sourceKind;		// 0 = nenhuma, 1 = arma, 2 = feitico/outra coisa
		float			weaponWeight;
		float			weaponStagger;
		int				weaponType;
	};

	bool ReadHit(TESHitEvent * evn, HitInfo * h)
	{
		__try
		{
			memset(h, 0, sizeof(HitInfo));
			TESObjectREFR * target = evn->target;
			TESObjectREFR * caster = evn->caster;
			if(!target || target->formType != kFormType_Character) return false;
			h->victimRef = target;
			if(!ReadActorStats((Actor *)target, &h->victim)) return false;

			if(caster)
			{
				h->attackerId = caster->formID;
				if(caster->formType == kFormType_Character)
				{
					h->attackerIsActor = ReadActorStats((Actor *)caster, &h->attacker);
					h->attackerActor = (Actor *)caster;
				}
			}
			h->flags = evn->flags;
			h->sourceId = evn->sourceFormID;
			h->projId = evn->projectileFormID;
			if(h->sourceId)
			{
				TESForm * form = LookupFormByID(h->sourceId);
				if(form)
				{
					if(form->formType == kFormType_Weapon)
					{
						TESObjectWEAP * w = (TESObjectWEAP *)form;
						h->sourceKind = 1;
						h->weaponWeight = w->weight.weight;
						h->weaponStagger = w->stagger();
						h->weaponType = (int)w->type();
					}
					else
					{
						h->sourceKind = 2;
						if(g_cfg.ignoreOwnSpells && form->formType == kFormType_Spell)
						{
							TESFullName * fn = DYNAMIC_CAST(form, TESForm, TESFullName);
							if(fn && fn->name.data && strncmp(fn->name.data, "NahIDWin", 8) == 0) h->ownSpell = true;
						}
					}
				}
			}
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Le a funcao na posicao "slot" da tabela do sub-objeto holder (0 se falhar).
	void * ReadVtblSlot(void * holder, int slot)
	{
		__try
		{
			void ** vtbl = *(void ***)holder;
			if(!vtbl) return NULL;
			return vtbl[slot];
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return NULL;
		}
	}

	bool ReadBytes(void * p, unsigned char * out, int n)
	{
		__try
		{
			memcpy(out, p, (size_t)n);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool SafeCreateHandle(TESObjectREFR * ref, UInt32 * out)
	{
		__try
		{
			*out = 0;
			CreateRefHandleByREFR(out, ref);
			return (*out != 0);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool SafeSend(EventDispatcher<PoiseModEvent> * d, PoiseModEvent * evn)
	{
		__try
		{
			d->SendEvent(evn);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// ---- escudo de stagger (furia): filtra os pedidos de reacao a golpe enviados ao grafo de animacao
	volatile LONG	g_shieldBlocked = 0;
	volatile LONG	g_livesUsed = 0;		// vidas extra gastas desde o ultimo sono
	volatile LONG	g_lifePending = 0;
	DWORD			g_invulnUntil = 0;
	LONG			g_essSent = -1;
	DWORD			g_essLast = 0;
	std::set<UInt32> g_bossMarked;
	struct RecentAtk { UInt32 id; DWORD tick; };
	RecentAtk		g_recentAtk[4];
	DWORD			g_supUntil = 0, g_supReadyAt = 0, g_supLastCost = 0, g_supLastHit = 0;
	volatile LONG	g_ownNotify = 0;		// 1 = chamada vinda do proprio mod (nunca filtrar)
	void EnsureNotifyHook(TESObjectREFR * ref);
	bool CallNotify(void * holder, void * fn, const BSFixedString * name, bool * result)
	{
		__try
		{
			typedef bool (__thiscall * NotifyFn)(void *, const BSFixedString *);
			InterlockedExchange(&g_ownNotify, 1);
			*result = ((NotifyFn)fn)(holder, name);
			InterlockedExchange(&g_ownNotify, 0);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			InterlockedExchange(&g_ownNotify, 0);
			return false;
		}
	}

	// Estamina (valor de ator 26): escreve e verifica. Sem objetos com destrutor.
	bool AddStamina(Actor * a, float delta, float * oldv, float * newv)
	{
		__try
		{
			const float cur = a->actorValueOwner.GetCurrent(26);
			float nv = cur + delta;
			if(nv < 0.0f) nv = 0.0f;
			a->actorValueOwner.SetCurrent(26, nv);
			*oldv = cur;
			*newv = a->actorValueOwner.GetCurrent(26);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Restaura (delta > 0) ou ataca (delta < 0) um valor de ator pelo caminho do proprio jogo
	// (ActorValueOwner posicao 6, modificador 2 = dano), o mesmo que o Papyrus DamageActorValue / RestoreActorValue.
	void RestoreAV(Actor * a, UInt32 av, float delta)
	{
		__try
		{
			UInt32 bits;
			memcpy(&bits, &delta, sizeof(bits));
			a->actorValueOwner.Unk_06(2, av, bits);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

	bool ReadAV(Actor * a, UInt32 av, float * cur, float * mx)
	{
		__try
		{
			*cur = a->actorValueOwner.GetCurrent(av);
			*mx = a->actorValueOwner.GetMaximum(av);
			return (*mx > 0.0f && *mx < 100000.0f && *cur >= -100000.0f);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool			g_stamBroken = false;
	bool			g_dataLoaded = false;
	volatile LONG	g_stamHits = 0;

	// ------------------------------------------------------------------ TrueHUD
	THUD::IVTrueHUD1 *	g_hud = NULL;
	bool				g_hudTried = false;

	float HudGetMax(Actor * a)
	{
		ActorStats s;
		if(!a || !ReadActorStats(a, &s)) return 1.0f;
		if(g_cfg.hudBar == 2) return 100.0f;
		return (g_cfg.hudBar == 1) ? ComputeMaxPosture(s) : ComputeMax(s);
	}

	// ---- barras extra: poise + furia, desenhadas por codigo no HUD do jogo (jogador e, em teste, inimigos).
	// A barra especial do TrueHUD mostra a postura; estas duas linhas ficam por baixo.
	struct BarRect { float x, y, w, h; int color; int alpha; };
	struct BarInfo
	{
		float	poise;			// fracao que resta (0..1)
		int		rageState;		// 0 pronta, 1 em recarga, 2 ativa
		float	rageFrac;		// ativa: o que resta; em recarga: o que ja recarregou
		bool	warn;			// poise perto de quebrar
		float	posture;		// fracao da postura (0..1)
	};
	struct EnemyBar
	{
		float	wx, wy, wz;		// posicao da cabeca no mundo
		BarInfo	info;
		DWORD	tick;
	};
	typedef std::map<UInt32, EnemyBar> EnemyBarMap;
	EnemyBarMap		g_enemyBars;			// protegido pelo lock principal
	BarInfo			g_playerBar = { 1.0f, 0, 1.0f, false, 0.0f };
	DWORD			g_playerBarTick = 0;
	DWORD			g_barsActiveUntil = 0;
	bool			g_barsDrawn = false;
	bool			g_playerBarValid = false;
	DWORD			g_rageFlashUntil = 0;
	int				g_prevPlayerRageState = -1;
	GFxMovieView *	g_bar2View = NULL;
	DWORD			g_bar2Tick = 0;
	bool			g_bar2Dead = false;

	GFxMovieView * GetHudView()
	{
		MenuManager * mm = MenuManager::GetSingleton();
		if(!mm) return NULL;
		BSFixedString name("HUD Menu");
		IMenu * menu = mm->GetMenu(&name);
		return menu ? menu->view : NULL;
	}

	// Sem objetos com destrutor: pode usar __try. Desenha os retangulos (fundo e valor) de uma vez.
	bool InvokeDrawRects(GFxMovieView * v, bool init, const BarRect * r, int n)
	{
		__try
		{
			if(init) v->Invoke("_root.createEmptyMovieClip", NULL, "%s%d", "nahBar", 9500);
			v->Invoke("_root.nahBar.clear", NULL, "");
			for(int i = 0; i < n; i++)
			{
				const BarRect & b = r[i];
				if(!(b.w > 0.0f) || !(b.h > 0.0f)) continue;
				v->Invoke("_root.nahBar.beginFill", NULL, "%d%d", b.color, b.alpha);
				v->Invoke("_root.nahBar.moveTo", NULL, "%f%f", b.x, b.y);
				v->Invoke("_root.nahBar.lineTo", NULL, "%f%f", b.x + b.w, b.y);
				v->Invoke("_root.nahBar.lineTo", NULL, "%f%f", b.x + b.w, b.y + b.h);
				v->Invoke("_root.nahBar.lineTo", NULL, "%f%f", b.x, b.y + b.h);
				v->Invoke("_root.nahBar.endFill", NULL, "");
			}
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	bool GetVisibleRect(GFxMovieView * v, float * l, float * t, float * r, float * b)
	{
		__try
		{
			GRectF rc = v->GetVisibleFrameRect();
			*l = rc.left; *t = rc.top; *r = rc.right; *b = rc.bottom;
			return (*r > *l) && (*b > *t);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Posicao da cabeca de um ator (no mundo). Mesmo caminho que o TrueHUD LE usa (gestor de processo -> no da cabeca).
	bool ReadHeadPos(Actor * a, float * x, float * y, float * z)
	{
		__try
		{
			float px = a->pos.x, py = a->pos.y, pz = a->pos.z + 110.0f;
			void * pm = a->processManager;
			if(pm)
			{
				void * middle = *(void **)((UInt8 *)pm + 4);
				if(middle)
				{
					NiAVObject * head = *(NiAVObject **)((UInt8 *)middle + 0xB8);
					if(head)
					{
						px = head->m_worldTransform.pos.x;
						py = head->m_worldTransform.pos.y;
						pz = head->m_worldTransform.pos.z;
					}
				}
			}
			if(!(px > -1.0e8f && px < 1.0e8f && py > -1.0e8f && py < 1.0e8f && pz > -1.0e8f && pz < 1.0e8f)) return false;
			*x = px; *y = py; *z = pz;
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Mundo -> ecra (0..1). Funcao e matrizes da LE 1.9.32.0, as mesmas que o TrueHUD LE usa.
	bool ProjectPoint(float wx, float wy, float wz, float * sx, float * sy)
	{
		__try
		{
			typedef bool (__cdecl * ProjFn)(const float *, const NiRect<float> *, const NiPoint3 *, float *, float *, float *, float);
			NiPoint3 p;
			p.x = wx; p.y = wy; p.z = wz;
			float x = 0.0f, y = 0.0f, d = 0.0f;
			const bool r = ((ProjFn)0x00AB84C0)((const float *)0x01B3EA10, (const NiRect<float> *)0x01B3EA74, &p, &x, &y, &d, 1e-5f);
			if(!r || !(x > -2.0f && x < 3.0f && y > -2.0f && y < 3.0f)) return false;
			*sx = x; *sy = y;
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Chamar com o lock tomado.
	void RageDisplay(const PoiseState & st, DWORD now, bool npc, int * state, float * frac)
	{
		float f = 1.0f;
		if(now < st.rageUntil)
		{
			const float total = g_cfg.rageSeconds * 1000.0f;
			*state = 2;
			f = (total > 1.0f) ? (float)(st.rageUntil - now) / total : 0.0f;
		}
		else if(now < st.rageReadyAt)
		{
			const float total = (npc ? g_cfg.rageNpcCooldown : g_cfg.rageCooldown) * 1000.0f;
			*state = 1;
			f = (total > 1.0f) ? 1.0f - (float)(st.rageReadyAt - now) / total : 1.0f;
		}
		else *state = 0;
		if(f < 0.0f) f = 0.0f;
		if(f > 1.0f) f = 1.0f;
		*frac = f;
	}

	int AppendBarRects(BarRect * out, int n, int cap, float x, float y, float w, float h, const BarInfo & b, bool showRageRow, DWORD now)
	{
		const float gap = 3.0f;
		float yy = y;
		if(g_cfg.barPoise && n + 2 <= cap)
		{
			BarRect bg = { x, yy, w, h, 0x000000, 60 };
			out[n++] = bg;
			BarRect fl = { x, yy, w * b.poise, h, b.warn ? 0xFF6040 : 0x4FA3FF, 90 };
			out[n++] = fl;
			yy += h + gap;
		}
		if(g_cfg.barRage && g_cfg.rageEnabled && showRageRow && n + 2 <= cap)
		{
			BarRect bg = { x, yy, w, h, 0x000000, 60 };
			out[n++] = bg;
			int color, alpha;
			float frac = b.rageFrac;
			if(b.rageState == 2) { color = 0xFFC83C; alpha = 95; }
			else if(b.rageState == 1) { color = 0x7A6A48; alpha = 70; }
			else
			{
				// pronta: dourada; pisca durante um instante quando acaba de ficar pronta
				frac = 1.0f;
				if(now < g_rageFlashUntil) { color = 0xFFFFFF; alpha = ((now / 150) & 1) ? 100 : 45; }
				else { color = 0xE8B84A; alpha = 80; }
			}
			BarRect fl = { x, yy, w * frac, h, color, alpha };
			out[n++] = fl;
		}
		return n;
	}

	int AppendEnemyRects(GFxMovieView * v, BarRect * out, int n, int cap, DWORD now)
	{
		float l, t, r, b;
		if(!GetVisibleRect(v, &l, &t, &r, &b)) return n;
		int shown = 0;
		Lock();
		for(EnemyBarMap::iterator it = g_enemyBars.begin(); it != g_enemyBars.end(); )
		{
			if(now - it->second.tick > 500) { g_enemyBars.erase(it++); continue; }
			if(shown < g_cfg.enemyBarMax && now - it->second.tick <= 250)
			{
				float sx, sy;
				if(ProjectPoint(it->second.wx, it->second.wy, it->second.wz, &sx, &sy))
				{
					const float hx = l + (r - l) * sx;
					const float hy = t + (b - t) * (1.0f - sy);
					const int before = n;
					n = AppendBarRects(out, n, cap, hx - g_cfg.enemyBarW * 0.5f, hy + g_cfg.enemyBarOffsetY, g_cfg.enemyBarW, g_cfg.enemyBarH,
						it->second.info, it->second.info.rageState == 2, now);
					if(n > before) shown++;
				}
			}
			++it;
		}
		Unlock();
		return n;
	}

	void FlushBars(DWORD now)
	{
		if(g_bar2Dead) return;
		if(!g_cfg.bar2Enabled) { if(!g_barsDrawn) return; }
		if(now - g_bar2Tick < 10) return;
		g_bar2Tick = now;
		GFxMovieView * v = GetHudView();
		if(!v) return;
		const bool init = (v != g_bar2View);
		g_bar2View = v;
		if(!init && !g_barsDrawn && !g_playerBarValid && g_enemyBars.empty()) return;
		BarRect rects[160];
		int n = 0;
		if(g_cfg.bar2Enabled)
		{
			// como o TrueHUD: os inimigos so aparecem enquanto o TrueHUD lhes mostra a barra; o jogador, em combate ou quando algo mudou
			int enemies = 0;
			if(g_cfg.enemyBars)
			{
				n = AppendEnemyRects(v, rects, n, 160, now);
				enemies = n;
			}
			const bool fresh = g_playerBarValid && (now - g_playerBarTick) < 400;
			if(fresh && (enemies > 0 || now < g_barsActiveUntil))
				n = AppendBarRects(rects, n, 160, g_cfg.bar2X, g_cfg.bar2Y, g_cfg.bar2W, g_cfg.bar2H, g_playerBar, true, now);
		}
		if(n == 0 && !g_barsDrawn && !init) return;
		if(!InvokeDrawRects(v, init, rects, n))
		{
			g_bar2Dead = true;
			_MESSAGE("WARNING: the extra bars crashed while drawing (caught) - turned off for this session");
		}
		else
		{
			g_barsDrawn = (n > 0);
			if(init) _MESSAGE("bars: HUD drawing started (poise + rage lines)");
		}
	}

	void RecordEnemyBar(Actor * a, UInt32 id, const BarInfo & info, DWORD now)
	{
		float wx, wy, wz;
		if(!ReadHeadPos(a, &wx, &wy, &wz)) return;
		Lock();
		if(g_enemyBars.size() > 64) g_enemyBars.clear();
		EnemyBar & e = g_enemyBars[id];
		e.wx = wx; e.wy = wy; e.wz = wz; e.info = info; e.tick = now;
		Unlock();
	}

	// Estamina cheia (usado durante a furia do jogador).
	void FillStamina(Actor * a)
	{
		float c = 0.0f, m = 0.0f;
		if(ReadAV(a, 26, &c, &m) && c < m - 0.5f) RestoreAV(a, 26, m - c);
	}

	// Poise: a barra mostra o que resta. Postura: a barra mostra o que ja encheu (cheia = BleedOut).
	// O TrueHUD chama isto para cada ator com barra (e todos os quadros para o jogador).
	float HudGetCur(Actor * a)
	{
		ActorStats s;
		if(!a || !ReadActorStats(a, &s)) return 0.0f;
		const float mx = ComputeMax(s), mp = ComputeMaxPosture(s);
		if((s.isPlayer && !g_cfg.applyPlayer) || (!s.isPlayer && !g_cfg.applyNPCs)) return (g_cfg.hudBar == 0) ? mx : 0.0f;
		const DWORD now = GetTickCount();
		BarInfo info;
		float cur;
		bool raging = false;
		Lock();
		{
			PoiseState & st = FindOrCreate(s.id, mx, mp, now);
			Regen(st, now);
			RegenPosture(st, now);
			float poiseFrac = (st.max > 0.0f) ? (st.cur / st.max) : 1.0f;
			if(poiseFrac < 0.0f) poiseFrac = 0.0f;
			if(poiseFrac > 1.0f) poiseFrac = 1.0f;
			if(g_cfg.hudBar == 2)
			{
				// "risco": o que estiver mais perto de quebrar (0 = tranquilo, 100 = vai quebrar)
				const float rp = 1.0f - poiseFrac;
				const float rs = (st.pMax > 0.0f) ? (st.pCur / st.pMax) : 0.0f;
				cur = 100.0f * (rp > rs ? rp : rs);
			}
			else cur = (g_cfg.hudBar == 1) ? st.pCur : st.cur;
			info.poise = poiseFrac;
			info.posture = (st.pMax > 0.0f) ? (st.pCur / st.pMax) : 0.0f;
			info.warn = (g_cfg.poiseWarnFraction > 0.0f && poiseFrac < g_cfg.poiseWarnFraction);
			RageDisplay(st, now, !s.isPlayer, &info.rageState, &info.rageFrac);
			raging = (now < st.rageUntil);
		}
		Unlock();
		if(s.isPlayer)
		{
			if(info.rageState == 0 && g_prevPlayerRageState == 1) g_rageFlashUntil = now + 1500;	// a furia acabou de ficar pronta
			g_prevPlayerRageState = info.rageState;
			g_playerBar = info;
			g_playerBarValid = true;
			g_playerBarTick = now;
			if(info.poise < 0.995f || info.posture > 0.005f || info.rageState != 0) g_barsActiveUntil = now + 4000;
			if(raging && g_cfg.rageEnabled && g_cfg.rageZeroStamina) FillStamina(a);
		}
		else if(g_cfg.bar2Enabled && g_cfg.enemyBars && s.alive)
		{
			RecordEnemyBar(a, s.id, info, now);
		}
		FlushBars(now);
		return cur;
	}

	void TryHud()
	{
		if(g_hud || g_hudTried || !g_cfg.hudEnabled) return;
		HMODULE mod = GetModuleHandleA("TrueHUD.dll");
		if(!mod) { _MESSAGE("TrueHUD: TrueHUD.dll is not loaded - no poise bars (the poise still works)"); return; }
		g_hudTried = true;
		typedef void * (__cdecl * RequestFn)(unsigned char);
		RequestFn req = (RequestFn)GetProcAddress(mod, "RequestPluginAPI");
		if(!req) { _MESSAGE("TrueHUD: RequestPluginAPI not found"); return; }
		THUD::IVTrueHUD1 * api = (THUD::IVTrueHUD1 *)req(0);	// 0 = interface V1
		if(!api) { _MESSAGE("TrueHUD: the API request returned nothing"); return; }

		THUD::APIResult r1 = api->RequestSpecialResourceBarsControl(g_pluginHandle);
		_MESSAGE("TrueHUD: RequestSpecialResourceBarsControl -> %d (0 ok, 3 already given, 4 taken by another mod)", (int)r1);
		if(r1 != THUD::kOK && r1 != THUD::kAlreadyGiven) return;

		THUD::APIResult r2 = api->RegisterSpecialResourceFunctions(g_pluginHandle,
			THUD::SpecialResourceCallback(&HudGetCur), THUD::SpecialResourceCallback(&HudGetMax), true, g_cfg.hudPlayer != 0);
		_MESSAGE("TrueHUD: RegisterSpecialResourceFunctions -> %d", (int)r2);
		if(r2 == THUD::kOK || r2 == THUD::kAlreadyGiven)
		{
			g_hud = api;
			_MESSAGE("TrueHUD: poise bars registered (turn on 'special bars' in the TrueHUD menu if you do not see them)");
		}
	}

	void HudFlash(TESObjectREFR * ref)
	{
		if(!g_hud || !g_cfg.hudFlash) return;
		UInt32 handle = 0;
		if(!SafeCreateHandle(ref, &handle)) return;
		g_hud->FlashActorSpecialBar(g_pluginHandle, handle, true);
	}

	// ------------------------------------------------------------------ stagger
	void NativeStagger(TESObjectREFR * ref)
	{
		BSFixedString name(g_cfg.nativeEvent);
		void * holder = (UInt8 *)ref + 0x20;	// sub-objeto IAnimationGraphManagerHolder do TESObjectREFR
		void * fn = ReadVtblSlot(holder, g_cfg.nativeSlot);
		if(!fn) { _MESSAGE("native stagger: could not read slot %d", g_cfg.nativeSlot); return; }
		_MESSAGE("native stagger: calling slot %d (function %08X) on %08X with '%s'", g_cfg.nativeSlot, (unsigned)reinterpret_cast<UInt32>(fn), ref->formID, g_cfg.nativeEvent);
		bool result = false;
		const bool ok = CallNotify(holder, fn, &name, &result);
		_MESSAGE("native stagger: %s (returned %d)", ok ? "call finished" : "CRASHED inside the call (caught)", result ? 1 : 0);
	}

	// tier: 1 = stagger leve, 2 = medio, 3 = pesado (poise quebrou), 4 = BleedOut (postura quebrou)
	void SendStaggerEvent(TESObjectREFR * victim, UInt32 attackerId, int tier)
	{
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) { _MESSAGE("stagger: the SKSE mod event dispatcher is not available"); return; }

		if(tier == 4) EnsureNotifyHook(victim);
		const LONG seq = InterlockedIncrement(&g_seq) & 0x1FFFFF;
		char arg[24];
		_snprintf_s(arg, sizeof(arg), _TRUNCATE, "%d", (int)attackerId);
		const float packed = (float)(seq * 8 + tier);

		Lock();
		g_sentTick[seq & 63] = GetTickCount();
		g_sentSeq[seq & 63] = seq;
		Unlock();

		PoiseModEvent evn("NahIDWin_Stagger", arg, packed, (TESForm *)victim);
		if(SafeSend(d, &evn)) InterlockedIncrement(&g_sent);
		else { InterlockedIncrement(&g_faults); _MESSAGE("stagger: sending the mod event FAILED (caught)"); }
	}

	// Combatentes que ja receberam os feitiços do Inpa (substitui o "cloak" dele e a distribuicao geral por SPID):
	// so quem entra em combate leva os scripts. O script ponte da os feitiços ao receber o evento.
	std::set<UInt32> g_combatants;

	void NoteCombatant(TESObjectREFR * ref, UInt32 id)
	{
		if(!ref || id == 0 || id == 0x14 || !g_cfg.inpaMode) return;
		bool isNew = false;
		Lock();
		{
			if(g_combatants.size() > 4000) g_combatants.clear();
			isNew = g_combatants.insert(id).second;
		}
		Unlock();
		if(!isNew) return;
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_Combatant", "", 0.0f, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	// Furia de um NPC: avisa a ponte (sender = o NPC, numArg = duracao em segundos) para mostrar o brilho/simbolo.
	void SendLifeEvent(TESObjectREFR * ref, int used)
	{
		if(!ref) return;
		EnsureNotifyHook(ref);
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_Life", "", (float)used, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	// Execucao: o Papyrus lanca o feitico de execucao do Inpa (ou mata, em gigantes/dragoes). Nao depende do OnHit do Inpa.
	void SendExecuteEvent(TESObjectREFR * ref, const char * tag)
	{
		if(!ref) return;
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		g_execTick = GetTickCount();
		_MESSAGE("EXECUTION requested: victim=%08X %s", (UInt32)ref->formID, tag);
		PoiseModEvent evn("NahIDWin_Execute", tag, 0.0f, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	// Deflect: o Papyrus relanca o feitico no conjurador (sender = conjurador, tag = formID do feitico em decimal).
	void SendReflectEvent(TESObjectREFR * ref, UInt32 spellId)
	{
		if(!ref || !spellId) return;
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		char tag[24];
		_snprintf(tag, sizeof(tag), "%u", (unsigned)spellId);
		tag[23] = 0;
		PoiseModEvent evn("NahIDWin_Reflect", tag, 0.0f, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	void SendRageEvent(TESObjectREFR * ref, float seconds, const char * tag = "")
	{
		if(!ref) return;
		EnsureNotifyHook(ref);
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_Rage", tag, seconds, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	// Em cada load/novo jogo o estado antigo (poise, postura, furia...) deixa de valer.
	void ClearStates(const char * why)
	{
		Lock();
		const size_t n = g_states.size();
		g_states.clear();
		g_combatants.clear();
		Unlock();
		g_parryUntil = 0;
		g_parryReadyAt = 0;
		g_parryHitDone = 0;
		g_blockHeld = 0;
		g_peril.clear(); g_hostile.clear(); g_flee.clear(); g_deflectOn = 0; g_parryAtkId = 0; memset(g_magicAtk, 0, sizeof(g_magicAtk)); memset(g_dot, 0, sizeof(g_dot));
		g_playerBarValid = false;
		g_prevPlayerRageState = -1;
		g_powerUntil = 0;
		g_livesUsed = 0; g_invulnUntil = 0; g_supUntil = 0; g_supReadyAt = 0;
		g_essSent = -1; g_essLast = 0; g_bossMarked.clear(); memset(g_recentAtk, 0, sizeof(g_recentAtk));
		_MESSAGE("%s: cleared %d tracked actors", why, (int)n);
	}

	// Avisa o script ponte de que um save foi carregado (para registar de novo os eventos de animacao do jogador).
	void SendLoadEvent()
	{
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_Load", "", 0.0f, NULL);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	// ------------------------------------------------------------------ diagnostico
	void WriteSummary(DWORD now, bool force)
	{
		if(!g_cfg.diag || g_cfg.summarySec <= 0) return;
		if(!force && now - g_lastSummary < (DWORD)g_cfg.summarySec * 1000) return;
		g_lastSummary = now;
		Lock();
		const DWORD latCount = g_latCount, latSum = g_latSum, latMax = g_latMax;
		const size_t tracked = g_states.size();
		Unlock();
		_MESSAGE("SUMMARY: hits=%d applied=%d poiseBreaks=%d postureBreaks=%d light=%d medium=%d parries=%d rages=%d | shield blocked=%d | events sent=%d acked=%d pending=%d | script started=%d | latency avg=%d ms max=%d ms (n=%d) | tracked=%d faults=%d | truehud=%s",
			(int)g_hits, (int)g_applied, (int)g_breaks, (int)g_postureBreaks, (int)g_lightSent, (int)g_mediumSent, (int)g_parries, (int)g_rages, (int)g_shieldBlocked, (int)g_sent, (int)g_acks, (int)(g_sent - g_acks), (int)g_scriptReady,
			latCount ? (int)(latSum / latCount) : 0, (int)latMax, (int)latCount, (int)tracked, (int)g_faults, g_hud ? "on" : "off");
		if(g_sent > 0 && g_acks == 0)
			_MESSAGE("WARNING: stagger events were sent but the Papyrus script never answered. The script is not running or the mod event was not registered (is NahIDWin.esp enabled? is the .pex installed?)");
		else if(g_sent - g_acks > 3)
			_MESSAGE("WARNING: %d stagger events were not answered by the script (Papyrus lag or lost events).", (int)(g_sent - g_acks));
	}

	void DumpVtable()
	{
		if(g_vtableDumped || !g_cfg.diag || !g_cfg.dumpVtable) return;
		PlayerCharacter * p = Engine::Player();
		if(!p) return;
		g_vtableDumped = true;
		void * holder = (UInt8 *)p + 0x20;
		_MESSAGE("=== VTABLE DUMP: IAnimationGraphManagerHolder of the player (holder %08X, slots 0..19, first 40 bytes of each function) ===", (unsigned)reinterpret_cast<UInt32>(holder));
		for(int slot = 0; slot < 20; slot++)
		{
			void * fn = ReadVtblSlot(holder, slot);
			unsigned char bytes[40];
			char line[128];
			line[0] = 0;
			if(fn && ReadBytes(fn, bytes, 40))
			{
				for(int i = 0; i < 40; i++)
				{
					char two[4];
					_snprintf_s(two, sizeof(two), _TRUNCATE, "%02X", bytes[i]);
					strcat_s(line, sizeof(line), two);
				}
			}
			_MESSAGE("vtbl[%02d] = %08X : %s", slot, (unsigned)reinterpret_cast<UInt32>(fn), line[0] ? line : "(unreadable)");
		}
		_MESSAGE("=== END VTABLE DUMP ===");
	}

	// ------------------------------------------------------------------ o golpe
	float PoiseDamage(const HitInfo & h)
	{
		float dmg;
		if(h.sourceKind == 1)
		{
			int t = h.weaponType;
			if(t < 0 || t > 9) t = 1;
			dmg = h.weaponWeight * g_cfg.weightMult + g_cfg.typeBase[t];
			if(t == 0) dmg += h.attacker.unarmed * g_cfg.unarmedMult;	// punhos
			if(h.weaponStagger > 0.0f) dmg *= (1.0f + h.weaponStagger * g_cfg.staggerStatMult);
		}
		else if(h.sourceKind == 2)
		{
			dmg = g_cfg.magicDamage;
		}
		else
		{
			dmg = g_cfg.creatureBase + h.attacker.unarmed * g_cfg.unarmedMult;	// criaturas e ataques sem arma
		}
		if(h.attackerIsActor && h.sourceKind != 2) dmg += h.attacker.mass * g_cfg.attackerMassMult;

		if(h.projId && h.sourceKind != 2)			dmg *= g_cfg.rangedMult;
		if(h.flags & TESHitEvent::kFlag_PowerAttack)	dmg *= g_cfg.powerMult;
		if(h.flags & TESHitEvent::kFlag_Bash)			dmg *= g_cfg.bashMult;
		if(h.flags & TESHitEvent::kFlag_SneakAttack)	dmg *= g_cfg.sneakMult;
		dmg *= h.victim.isPlayer ? g_cfg.playerTakenMult : g_cfg.npcTakenMult;
		if(!(dmg >= 0.0f && dmg < 100000.0f)) dmg = 0.0f;
		return dmg;
	}

	// Parry: o atacante (NPC) ganha postura e, se nao estiver imune, leva stagger (ou BleedOut se a postura quebrar).
	// Devolve o tipo de evento a enviar ao atacante (0 = nenhum).
	void NoteMagicAttacker(UInt32 id, DWORD now)
	{
		int slot = 0;
		for(int i = 0; i < 4; i++)
		{
			if(g_magicAtk[i].id == id) { slot = i; break; }
			if(g_magicAtk[i].tick < g_magicAtk[slot].tick) slot = i;
		}
		g_magicAtk[slot].id = id; g_magicAtk[slot].tick = now;
	}
	// Deflect: depois de um parry numa magia, enquanto o jogador segura o bloqueio e tem estamina.
	bool DeflectActive(DWORD now)
	{
		if(!g_cfg.deflectEnabled || !g_deflectOn) return false;
		if(g_blockHeld == 0 || now - g_deflectStart > 20000) { g_deflectOn = 0; return false; }
		PlayerCharacter * p = Engine::Player();
		float c = 1.0f, m = 1.0f;
		if(!p || Engine::LifeState((Actor *)p) != 0) { g_deflectOn = 0; return false; }
		if(ReadAV((Actor *)p, 26, &c, &m) && c <= 0.5f) { g_deflectOn = 0; return false; }
		return true;
	}
	bool DeflectBlocks(UInt32 attackerId, DWORD now)
	{
		if(!DeflectActive(now)) return false;
		for(int i = 0; i < 4; i++) if(g_magicAtk[i].id == attackerId && now - g_magicAtk[i].tick < 1500) return true;
		return false;
	}
	// Limitador do dano continuo (Chamas, raios...): devolve true se este golpe de magia deve ser ignorado; *sustained = fluxo continuo.
	bool MagicThrottle(UInt32 victim, UInt32 attacker, UInt32 source, DWORD now, bool * sustained)
	{
		*sustained = false;
		if(g_cfg.magicDotMs <= 0) return false;
		const UInt32 key = victim * 31u ^ attacker * 17u ^ source * 7u ^ 0x9e3779b9u;
		int slot = 0;
		for(int i = 0; i < 16; i++)
		{
			if(g_dot[i].key == key)
			{
				const DWORD dt = now - g_dot[i].tick;
				if(dt < (DWORD)g_cfg.magicDotMs) return true;
				if(dt < 3000) *sustained = true;
				g_dot[i].tick = now;
				return false;
			}
			if(g_dot[i].tick < g_dot[slot].tick) slot = i;
		}
		g_dot[slot].key = key; g_dot[slot].tick = now;
		return false;
	}

	// A janela do parry: so o tempo decide (nao depende de o jogo marcar o golpe como bloqueado).
	bool ParryWindowActive(DWORD now)
	{
		if(!g_cfg.parryEnabled) return false;
		if(!(now < g_parryUntil)) return false;
		if(g_cfg.parryRequireHeld && g_blockHeld == 0) return false;
		return true;
	}

	// ------------------------------------------------------------------ estado do parry do Inpa (globais do NahIDWin.esp fundido)
	// O script do Inpa poe zzzSekiro_control = 1 quando se carrega no bloqueio (janela do parry, dura "parrytimer" segundos) e = 2 depois.
	// Um golpe bloqueado com control == 1 e um parry. Os globais sao procurados pelo nome (EDID) em DataHandler->globals.
	TESGlobal *	g_inpaControl = NULL;
	bool		g_inpaSearched = false;
	volatile LONG g_inpaLogged = 0;

	TESGlobal * FindGlobalByName(const char * name)
	{
		__try
		{
			DataHandler * dh = *((DataHandler **)0x012E2CB8);
			if(!dh) return NULL;
			for(UInt32 i = 0; i < dh->globals.count; i++)
			{
				TESGlobal * g = NULL;
				if(!dh->globals.GetNthItem(i, g) || !g) continue;
				const char * n = *(const char **)((UInt8 *)g + 0x14);	// TESGlobal::unk14 = BSString (nome/EDID), m_data no inicio
				if(n && _stricmp(n, name) == 0) return g;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { }
		return NULL;
	}

	// Valor de zzzSekiro_control (-1 se o Inpa nao esta presente).
	float InpaControlValue()
	{
		if(!g_inpaSearched)
		{
			g_inpaSearched = true;
			g_inpaControl = FindGlobalByName("zzzSekiro_control");
			_MESSAGE("Inpa mode: global zzzSekiro_control %s", g_inpaControl ? "found" : "NOT FOUND (the merged NahIDWin.esp is not loaded?)");
		}
		if(!g_inpaControl) return -1.0f;
		__try { return *(volatile float *)((UInt8 *)g_inpaControl + 0x20); }
		__except(EXCEPTION_EXECUTE_HANDLER) { return -1.0f; }
	}

	// O atacante esta a frente do defensor (dentro de +-parryFrontAngle graus)?
	bool InFront(Actor * self, Actor * attacker)
	{
		__try
		{
			if(!self || !attacker) return false;
			const float dx = attacker->pos.x - self->pos.x;
			const float dy = attacker->pos.y - self->pos.y;
			if(dx * dx + dy * dy < 1.0f) return true;
			const float yaw = self->rot.z;
			float d = atan2f(dx, dy) - yaw;
			const float pi = 3.14159265f;
			while(d > pi) d -= 2.0f * pi;
			while(d < -pi) d += 2.0f * pi;
			if(d < 0.0f) d = -d;
			return d <= (float)g_cfg.parryFrontAngle * pi / 180.0f;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return true;
		}
	}

	Actor * ActorFromFormId(UInt32 id);
	// ---- ataque perigoso (bosses e criaturas grandes): aviso, ignora o bloqueio; esquivar ou fazer parry
	bool PerilousActive(UInt32 id, DWORD now)
	{
		if(!g_cfg.perilEnabled) return false;
		bool r = false;
		Lock();
		{
			std::map<UInt32, PerilSlot>::iterator it = g_peril.find(id);
			if(it != g_peril.end() && now < it->second.until) r = true;
		}
		Unlock();
		return r;
	}
	void PerilousClear(UInt32 id)
	{
		Lock();
		{
			std::map<UInt32, PerilSlot>::iterator it = g_peril.find(id);
			if(it != g_peril.end()) it->second.until = 0;
		}
		Unlock();
	}
	// Estado de ataque do NPC (0 nada, 1 preparar, 2 golpe...) se for boss/grande e estiver perto do jogador; -1 caso contrario.
	int ReadPerilousCandidate(UInt32 id)
	{
		__try
		{
			Actor * a = ActorFromFormId(id);
			PlayerCharacter * pc = Engine::Player();
			if(!a || !pc || id == 0x14 || (a->flags1 & Actor::kFlags_IsPlayerTeammate)) return -1;
			if(Engine::LifeState(a) != 0) return -1;
			ActorStats st;
			if(!ReadActorStats(a, &st)) return -1;
			if(!st.boss && !(st.mass >= g_cfg.kbGiantMass)) return -1;
			const float dx = a->pos.x - pc->pos.x, dy = a->pos.y - pc->pos.y;
			if(dx * dx + dy * dy > g_cfg.perilRange * g_cfg.perilRange) return -1;
			return (int)((a->actorState.flags08 >> 28) & 0xF);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return -1; }
	}
	void SendPerilousEvent(UInt32 id)
	{
		TESObjectREFR * ref = (TESObjectREFR *)ActorFromFormId(id);
		if(!ref) return;
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_Perilous", "", 0.0f, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}
	void TickPerilous(DWORD now)
	{
		if(!g_cfg.perilEnabled || now - g_perilTick < 60) return;
		g_perilTick = now;
		UInt32 ids[256]; int n = 0;
		Lock();
		for(std::set<UInt32>::iterator it = g_combatants.begin(); it != g_combatants.end() && n < 256; ++it) ids[n++] = *it;
		Unlock();
		for(int i = 0; i < n; i++)
		{
			const int as = ReadPerilousCandidate(ids[i]);
			if(as < 0) continue;
			bool start = false;
			Lock();
			{
				PerilSlot & sl = g_peril[ids[i]];
				const bool attacking = (as == 1 || as == 2);
				if(attacking && sl.prev == 0 && now >= sl.ready && (rand() % 100) < g_cfg.perilChance)
				{
					sl.until = now + (DWORD)g_cfg.perilWindowMs;
					sl.ready = now + (DWORD)(g_cfg.perilCooldown * 1000.0f);
					start = true;
				}
				sl.prev = (as == 0) ? 0 : (attacking ? 1 : sl.prev);
				if(as == 0 || as >= 3) sl.prev = 0;
				if(g_peril.size() > 400) g_peril.clear();
			}
			Unlock();
			if(start)
			{
				_MESSAGE("PERILOUS attack: %08X (warning)", ids[i]);
				SendPerilousEvent(ids[i]);
			}
		}
	}

	// ---- vigia de fugas: regista quando um inimigo hostil deixa de estar em combate com vida (para descobrir porque fogem)
	bool ReadFleeInfo(UInt32 id, int * inCombat, float * hp, float * hpMax, float * st, float * stMax, float * dist, float * ex)
	{
		__try
		{
			Actor * a = ActorFromFormId(id);
			PlayerCharacter * pc = Engine::Player();
			if(!a || !pc || Engine::LifeState(a) != 0) return false;
			*inCombat = a->IsInCombat() ? 1 : 0;
			ReadAV(a, 24, hp, hpMax);
			ReadAV(a, 26, st, stMax);
			float t0 = 0.0f, t1 = 0.0f;
			ex[0] = ex[1] = ex[2] = -1.0f;
			if(ReadAV(a, 0, &t0, &t1)) ex[0] = t0;	// Aggression
			if(ReadAV(a, 1, &t0, &t1)) ex[1] = t0;	// Confidence
			if(ReadAV(a, 30, &t0, &t1)) ex[2] = t0;	// SpeedMult
			const float dx = a->pos.x - pc->pos.x, dy = a->pos.y - pc->pos.y;
			*dist = sqrtf(dx * dx + dy * dy);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	void TickFleeWatch(DWORD now)
	{
		if(!g_cfg.diag || now - g_fleeTick < 250) return;
		g_fleeTick = now;
		UInt32 ids[64]; DWORD hitAt[64]; int n = 0;
		Lock();
		for(std::map<UInt32, DWORD>::iterator it = g_hostile.begin(); it != g_hostile.end() && n < 64; ++it) if(now - it->second < 20000) { ids[n] = it->first; hitAt[n] = it->second; n++; }
		Unlock();
		for(int i = 0; i < n; i++)
		{
			int c = 0; float hp = 0, hm = 0, st = 0, sm = 0, d = 0, ex[3];
			if(!ReadFleeInfo(ids[i], &c, &hp, &hm, &st, &sm, &d, ex)) continue;
			bool log = false;
			float poise = -1.0f, posture = -1.0f; int bleed = 0, rage = 0;
			Lock();
			{
				FleeSlot & sl = g_flee[ids[i]];
				if(sl.combat == 1 && c == 0 && now - sl.lastLog > 3000) { log = true; sl.lastLog = now; }
				sl.combat = c;
				if(log)
				{
					StateMap::iterator it = g_states.find(ids[i]);
					if(it != g_states.end())
					{
						const PoiseState & ps = it->second;
						poise = ps.max > 0.0f ? ps.cur / ps.max : -1.0f;
						posture = ps.pMax > 0.0f ? ps.pCur / ps.pMax : -1.0f;
						bleed = (now < ps.bleedUntil || now < ps.bleedImmuneUntil) ? 1 : 0;
						rage = (now < ps.rageUntil) ? 1 : 0;
					}
				}
				if(g_flee.size() > 300) g_flee.clear();
			}
			Unlock();
			if(log) _MESSAGE("FLEE-WATCH: %08X left combat (alive) hp=%.0f/%.0f stamina=%.0f/%.0f poise=%.2f posture=%.2f bleedout=%d rage=%d dist=%.0f aggression=%.0f confidence=%.0f speedMult=%.0f msSinceHitWithPlayer=%d", ids[i], hp, hm, st, sm, poise, posture, bleed, rage, d, ex[0], ex[1], ex[2], (int)(now - hitAt[i]));
		}
	}

	int ParryAttacker(const HitInfo & h, DWORD now)
	{
		if(!h.attackerIsActor || !h.attackerActor || h.attacker.isPlayer || !h.attacker.alive) return 0;
		const float mxA = ComputeMax(h.attacker);
		const float mpA = ComputeMaxPosture(h.attacker);
		int kind = g_cfg.parryAttackerStagger ? 1 : 0;	// stagger leve (recuo)
		Lock();
		{
			PoiseState & st = FindOrCreate(h.attacker.id, mxA, mpA, now);
			Regen(st, now);
			RegenPosture(st, now);
			const bool frozen = (now < st.bleedUntil) || (now < st.bleedImmuneUntil) || (now < st.rageUntil);
			if(frozen) kind = 0;
			else if(g_cfg.postureEnabled)
			{
				float pm = g_cfg.parryAttackerPosture;
				if(PerilousActive(h.attackerId, now)) pm += g_cfg.perilParryPosture;
				st.pCur += mpA * pm;
				st.pLastHit = now;
				if(st.pCur >= st.pMax)
				{
					st.pCur = st.pMax * g_cfg.postureResetFraction;
					st.cur = mxA;
					st.bleedUntil = now + (DWORD)(g_cfg.bleedSeconds * 1000.0f);
					st.bleedImmuneUntil = st.bleedUntil + (DWORD)(g_cfg.bleedExtraImmune * 1000.0f);
					kind = 4;
					InterlockedIncrement(&g_postureBreaks);
				}
			}
		}
		Unlock();
		if(g_cfg.parryAttackerStamina > 0.0f)
		{
			float o = 0.0f, m = 0.0f;
			if(ReadAV(h.attackerActor, 26, &o, &m) && o > 0.0f) RestoreAV(h.attackerActor, 26, -(g_cfg.parryAttackerStamina > o ? o : g_cfg.parryAttackerStamina));
		}
		return kind;
	}

	void OnHit(TESHitEvent * evn)
	{
		if(!evn || !g_cfg.enabled) return;
		const DWORD now = GetTickCount();
		MaybeReloadConfig(now);
		if(g_cfg.diag) WriteSummary(now, false);

		HitInfo h;
		if(!ReadHit(evn, &h)) return;
		if(h.ownSpell) return;
		if(h.victimRef == evn->caster) return;
		if(h.attackerId != 0 && h.attackerId == h.victim.id)
		{
			// um ataque em area (por exemplo o murro no chao de um troll) nao pode atingir quem o fez
			static volatile LONG selfLogged = 0;
			if(g_cfg.diag && selfLogged < 20) { InterlockedIncrement(&selfLogged); _MESSAGE("ignored: self-inflicted hit on %08X (area attack?)", h.victim.id); }
			return;
		}
		if(!h.victim.alive) return;
		if(h.victim.isPlayer ? !g_cfg.applyPlayer : !g_cfg.applyNPCs) return;
		// Hits ambientais sem ator atacante (por exemplo colisao/interacao com teias)
		// nao devem gerar poise, parry ou stagger como se fossem golpes de combate.
		if(!h.attackerIsActor && h.attackerId == 0) return;
		bool dotSustained = false;
		if(h.sourceKind == 2 && h.attackerIsActor && !h.attacker.isPlayer)
		{
			if(h.victim.isPlayer)
			{
				NoteMagicAttacker(h.attackerId, now);
				if(DeflectActive(now))
				{
					// magia durante o deflect: sem dano nem poise/postura; devolvida a quem a lancou (no maximo 1 vez por segundo por conjurador)
					int slot = 0; bool fire = true;
					for(int i = 0; i < 4; i++)
					{
						if(g_reflectId[i] == h.attackerId) { slot = i; if(now - g_reflectTick[i] < 1000) fire = false; break; }
						if(g_reflectTick[i] < g_reflectTick[slot]) slot = i;
					}
					if(fire)
					{
						g_reflectId[slot] = h.attackerId; g_reflectTick[slot] = now;
						_MESSAGE("DEFLECT: magic %08X from %08X reflected", h.sourceId, h.attackerId);
						SendReflectEvent((TESObjectREFR *)h.attackerActor, h.sourceId);
					}
					return;
				}
			}
			if(MagicThrottle(h.victim.id, h.attackerId, h.sourceId, now, &dotSustained)) return;
		}
		if(h.attackerIsActor && h.attackerId != 0 && h.victim.id != 0)
		{
			// quem trocou golpes com o jogador e hostil a ele (usado para escolher a barra do alvo)
			const UInt32 other = h.victim.isPlayer ? h.attackerId : (h.attackerId == 0x14 ? h.victim.id : 0);
			if(other && other != 0x14)
			{
				Lock();
				if(g_hostile.size() > 300) g_hostile.clear();
				g_hostile[other] = now;
				Unlock();
			}
		}
		InterlockedIncrement(&g_hits);
		NoteCombatant(h.victimRef, h.victim.id);
		if(h.attackerIsActor && h.attackerActor) NoteCombatant((TESObjectREFR *)h.attackerActor, h.attacker.id);

		const float mx = ComputeMax(h.victim);
		const float mp = ComputeMaxPosture(h.victim);
		float dmg = PoiseDamage(h);	// dano "cheio" (sem bloqueio)
		if(dotSustained) dmg *= g_cfg.magicDotMult;
		const bool perilHit = h.victim.isPlayer && h.attackerIsActor && h.sourceKind != 2 && PerilousActive(h.attackerId, now);
		if(perilHit) dmg *= g_cfg.perilDmgMult;
		const bool blocked = !perilHit && (h.flags & TESHitEvent::kFlag_Blocked) != 0;	// o ataque perigoso ignora o bloqueio
		const bool heavyHit = (h.flags & (TESHitEvent::kFlag_PowerAttack | TESHitEvent::kFlag_Bash)) != 0;

		// Janela do parry: decidida so pelo tempo. Dentro dela o jogador esta imune (vida, estamina, poise, postura, stagger).
		bool parry = false, immune = false;
		if(g_cfg.inpaMode)
		{
			// Inpa Sekiro: golpe bloqueado enquanto a janela do parry dele esta aberta (zzzSekiro_control == 1). Um parry por janela (0,5 s).
			if(h.victim.isPlayer && blocked && h.attackerActor && h.attackerIsActor && !h.attacker.isPlayer)
			{
				const float ctl = InpaControlValue();
				if(g_cfg.diag && g_inpaLogged < 60) { InterlockedIncrement(&g_inpaLogged); _MESSAGE("Inpa: blocked hit on player, control=%.0f (1 = parry window)", ctl); }
				if(ctl > 0.5f && ctl < 1.5f && now - g_parryReadyAt >= 500)
				{
					g_parryReadyAt = now;
					immune = true; parry = true;
				}
			}
		}
		else if(h.victim.isPlayer && h.attackerActor && h.attackerIsActor && ParryWindowActive(now) && InFront((Actor *)h.victimRef, h.attackerActor))
		{
			immune = true;
			if(InterlockedExchange(&g_parryHitDone, 1) == 0) parry = true;
		}

		if(parry)
		{
			// um ataque conta um parry so (as duas vias de deteccao podiam contar o mesmo golpe duas vezes)
			if(h.attackerId == g_parryAtkId && now - g_parryAtkTick < 500) parry = false;
			else { g_parryAtkId = h.attackerId; g_parryAtkTick = now; }
			if(parry && h.sourceKind == 2 && g_cfg.deflectEnabled)
			{
				g_deflectOn = 1; g_deflectStart = now;
				_MESSAGE("DEFLECT: armed (hold block to keep reflecting magic)");
			}
		}

		if(perilHit)
		{
			if(!parry && !immune) _MESSAGE("PERILOUS attack landed on player (block ignored) by %08X", h.attackerId);
			if(parry) _MESSAGE("PERILOUS attack parried: %08X takes extra posture", h.attackerId);
		}
		const float poiseDmg = (parry || immune) ? 0.0f : (blocked ? dmg * g_cfg.blockedMult : dmg);
		float lowHp = 1.0f;
		if(h.victim.hpMax > 0.0f)
		{
			float frac = h.victim.hp / h.victim.hpMax;
			if(frac < 0.0f) frac = 0.0f;
			if(frac > 1.0f) frac = 1.0f;
			lowHp = 1.0f + g_cfg.postureLowHealthBonus * (1.0f - frac);
		}
		const float postureDmg = (parry || immune) ? 0.0f : dmg * g_cfg.postureDamageMult * lowHp * (blocked ? g_cfg.postureBlockedMult : 1.0f);

		// exaustao: a vitima ficou sem estamina?
		bool exhaustCandidate = false;
		if(g_cfg.exhaustEnabled && (h.victim.isPlayer || g_cfg.exhaustNpc))
		{
			float sc = 100.0f, sm = 100.0f;
			if(ReadAV((Actor *)h.victimRef, 26, &sc, &sm) && sc <= 1.0f) exhaustCandidate = true;
		}

		bool doExec = false, bossStagger = false;
		bool applied = false, poiseBroke = false, postureBroke = false, poiseWasImmune = false, bleeding = false, raging = false, rageStarted = false;
		float before = 0.0f, after = 0.0f, pBefore = 0.0f, pAfter = 0.0f;
		int kind = 0;
		Lock();
		{
			PoiseState & st = FindOrCreate(h.victim.id, mx, mp, now);
			Regen(st, now);
			RegenPosture(st, now);
			before = st.cur;
			pBefore = st.pCur;
			bleeding = (now < st.bleedUntil) || (now < st.bleedImmuneUntil);
			poiseWasImmune = (now < st.immuneUntil);
			raging = (now < st.rageUntil);
			const bool bossP1 = (!h.victim.isPlayer && h.victim.boss && st.bossUsed == 0);
			// execucao: golpe de arma do jogador num alvo com a postura quebrada (o boss so e executavel depois de gastar a 1a vida)
			if(g_cfg.execEnabled && h.attackerId == 0x14 && h.sourceKind == 1 && !h.victim.isPlayer && h.victim.alive && !bossP1 &&
				now < st.bleedUntil && now >= st.invulnUntil && (st.lastExec == 0 || now - st.lastExec > 4000))
			{
				st.lastExec = now;
				doExec = true;
			}

			// furia automatica dos NPCs
			if(!h.victim.isPlayer && g_cfg.rageEnabled && g_cfg.rageNpc && (h.victim.boss || h.victim.mass >= g_cfg.kbGiantMass) && !bleeding && !poiseWasImmune && !raging && now >= st.rageReadyAt)
			{
				const float pf = (st.max > 0.0f) ? (st.cur / st.max) : 1.0f;
				const float sf = (st.pMax > 0.0f) ? (st.pCur / st.pMax) : 0.0f;
				if((sf >= g_cfg.rageNpcPosture || pf <= g_cfg.rageNpcPoise) && (rand() % 100) < g_cfg.rageNpcChance)
				{
					st.rageUntil = now + (DWORD)(g_cfg.rageSeconds * (h.victim.mass >= g_cfg.kbGiantMass ? g_cfg.bigRageMult : 1.0f) * 1000.0f);
					st.rageReadyAt = now + (DWORD)(g_cfg.rageNpcCooldown * 1000.0f);
					raging = true;
					rageStarted = true;
				}
			}

			const bool frozen = bleeding || poiseWasImmune || raging || immune;
			if(parry && !bleeding)
			{
				// parry perfeito: o defensor recupera
				st.pCur -= mp * g_cfg.parryDefPostureRecover;
				if(st.pCur < 0.0f) st.pCur = 0.0f;
				st.cur += mx * g_cfg.parryDefPoiseRecover;
				if(st.cur > mx) st.cur = mx;
			}
			if(!frozen)
			{
				applied = true;
				st.lastHit = now;
				st.pLastHit = now;
				st.cur -= poiseDmg;
				if(st.cur <= 0.0f)
				{
					poiseBroke = true;
					st.cur = mx * g_cfg.resetFraction;
					st.immuneUntil = now + (DWORD)(g_cfg.immuneSeconds * 1000.0f);
				}
				if(g_cfg.postureEnabled)
				{
					st.pCur += postureDmg;
					if(st.pCur >= st.pMax && bossP1)
					{
						// boss na 1a fase: a postura quebrada so da stagger pesado (nao ha bleedout nem execucao)
						bossStagger = true;
						st.pCur = st.pMax * g_cfg.postureResetFraction;
					}
					else if(st.pCur >= st.pMax)
					{
						postureBroke = true;
						st.pCur = st.pMax * g_cfg.postureResetFraction;
						st.cur = mx;
						st.bleedUntil = now + (DWORD)(g_cfg.bleedSeconds * 1000.0f);
						st.bleedImmuneUntil = st.bleedUntil + (DWORD)(g_cfg.bleedExtraImmune * 1000.0f);
					}
				}

				// que stagger? (a postura quebrada vence tudo; depois o poise quebrado; depois leve/medio)
				if(postureBroke) kind = 4;
				else if(poiseBroke || bossStagger) kind = 3;
				else if(!blocked && g_cfg.lightEnabled && (h.victim.isPlayer ? g_cfg.lightPlayer : g_cfg.lightNpc))
				{
					if(heavyHit && g_cfg.mediumOnPower) kind = 2;
					else if(now >= st.lightUntil) kind = 1;
					if(kind) st.lightUntil = now + (DWORD)(h.victim.isPlayer ? g_cfg.lightCooldownPlayerMs : g_cfg.lightCooldownMs);
				}
			}
			// exaustao (estamina a zero): so se nada mais mais forte aconteceu neste golpe
			if(exhaustCandidate && kind == 0 && !bleeding && !raging && !immune && now >= st.exhaustReadyAt)
			{
				st.exhaustReadyAt = now + (DWORD)(g_cfg.exhaustCooldown * 1000.0f);
				kind = 7;
			}
			after = st.cur;
			pAfter = st.pCur;
		}
		Unlock();

		if(doExec) SendExecuteEvent(h.victimRef, (g_cfg.execBigKill && h.victim.mass >= g_cfg.kbGiantMass) ? "big" : "");

		// empurrao: so no stagger pesado e so em condicoes especificas
		if(kind == 3 && g_cfg.kbEnabled && g_cfg.kbForce > 0.0f && (!h.victim.isPlayer || g_cfg.kbPlayer))
		{
			bool heavy = false;
			if(g_cfg.kbOnPower && heavyHit) heavy = true;
			if(g_cfg.kbOnGiant && h.attackerIsActor && h.attacker.mass >= g_cfg.kbGiantMass) heavy = true;
			if(g_cfg.kbOnHeavyWeapon && h.sourceKind == 1 && (h.weaponType == 5 || h.weaponType == 6 || h.weaponWeight >= g_cfg.kbHeavyWeight)) heavy = true;
			if(heavy && h.victim.mass >= g_cfg.kbVictimMassMax && g_cfg.kbVictimMassMax > 0.0f) heavy = false;
			if(heavy) kind = 5;
		}

		// aviso: o poise desceu abaixo do limite (o jogador sabe que esta perto do stagger pesado)
		if(g_cfg.poiseWarnFraction > 0.0f && applied && !poiseBroke && mx > 0.0f &&
			before / mx >= g_cfg.poiseWarnFraction && after / mx < g_cfg.poiseWarnFraction) HudFlash(h.victimRef);

		// estamina do atacante NPC por golpe (desligado por omissao: deixa-se isso aos mods de estamina)
		if(g_cfg.stamEnabled && g_cfg.hitCostNpc > 0.0f && !g_stamBroken && h.attackerActor && h.attackerIsActor &&
			!h.attacker.isPlayer && h.attacker.alive && h.sourceKind != 2)
		{
			float o = 0.0f, n = 0.0f;
			const bool ok = AddStamina(h.attackerActor, -g_cfg.hitCostNpc, &o, &n);
			InterlockedIncrement(&g_stamHits);
			if(!ok || (o > g_cfg.hitCostNpc * 1.5f && n > o - g_cfg.hitCostNpc * 0.5f))
			{
				g_stamBroken = true;
				_MESSAGE("WARNING: writing the attacker's stamina does not work on this game (%s: %.1f -> %.1f). NPC stamina cost disabled.", ok ? "no effect" : "crashed (caught)", o, n);
			}
		}

		// bloqueio normal (sem parry): o atacante gasta estamina (e nada mais)
		if(blocked && !parry && !immune && g_cfg.blockDrain && g_cfg.blockDrainBase > 0.0f && h.victim.isPlayer && h.attackerActor && h.attackerIsActor &&
			!h.attacker.isPlayer && h.attacker.alive)
		{
			const float cost = g_cfg.blockDrainBase * (heavyHit ? g_cfg.blockDrainPower : 1.0f);
			float o = 0.0f, m = 0.0f;
			if(ReadAV(h.attackerActor, 26, &o, &m) && o > 0.0f) RestoreAV(h.attackerActor, 26, -(cost > o ? o : cost));
		}

		if(rageStarted)
		{
			InterlockedIncrement(&g_rages);
			_MESSAGE("RAGE (NPC): %08X is immune for %.1f s", h.victim.id, g_cfg.rageSeconds);
			const bool bigV = h.victim.mass >= g_cfg.kbGiantMass;
			if(h.victimRef && !h.victim.isPlayer) SendRageEvent(h.victimRef, g_cfg.rageSeconds * (bigV ? g_cfg.bigRageMult : 1.0f));
			if(g_cfg.rageNpcCostPercent > 0.0f && h.victimRef && !h.victim.boss && !bigV)
			{
				float cur = 0.0f, m = 0.0f;
				const UInt32 av = g_cfg.rageCostType == 0 ? 24 : 26;
				Actor * va = (Actor *)h.victimRef;
				if(ReadAV(va, av, &cur, &m))
				{
					float cost = m * g_cfg.rageNpcCostPercent / 100.0f;
					if(av == 24 && cost > cur - 1.0f) cost = cur - 1.0f;
					if(cost > 0.0f) RestoreAV(va, av, -cost);
				}
			}
			HudFlash(h.victimRef);
		}

		if(parry)
		{
			InterlockedIncrement(&g_parries);
			_MESSAGE("PARRY: %08X parried %08X", h.victim.id, h.attackerId);
			if(g_cfg.parryFx) SendStaggerEvent(h.victimRef, h.attackerId, 6);
			if(g_cfg.parryDefStaminaRecover > 0.0f)
			{
				float c = 0.0f, m = 0.0f;
				if(ReadAV((Actor *)h.victimRef, 26, &c, &m) && m > 0.0f) RestoreAV((Actor *)h.victimRef, 26, m * g_cfg.parryDefStaminaRecover);
			}
			const int ak = ParryAttacker(h, now);
			PerilousClear(h.attackerId);
			if(ak && h.attackerActor)
			{
				if(g_cfg.staggerMode == 1 && ak != 4) NativeStagger((TESObjectREFR *)h.attackerActor);
				else SendStaggerEvent((TESObjectREFR *)h.attackerActor, h.victim.id, ak);
				if(ak == 4) _MESSAGE("POSTURE BREAK (by parry) -> BLEEDOUT: %08X", h.attacker.id);
			}
		}

		if(applied) InterlockedIncrement(&g_applied);
		if(poiseBroke) InterlockedIncrement(&g_breaks);
		if(postureBroke) InterlockedIncrement(&g_postureBreaks);
		if(kind == 1) InterlockedIncrement(&g_lightSent);
		if(kind == 2) InterlockedIncrement(&g_mediumSent);

		if(g_cfg.diag && g_cfg.logHitsMax > 0 && g_hitLogged < g_cfg.logHitsMax)
		{
			InterlockedIncrement(&g_hitLogged);
			_MESSAGE("hit: victim=%08X%s attacker=%08X src=%08X(kind %d type %d w=%.1f stg=%.2f) flags=%X%s%s | vArmor=%.1f vHpMax=%.0f | dmg=%.1f | poise %.1f -> %.1f/%.1f | posture %.1f -> %.1f/%.1f | %s%s%s%s",
				h.victim.id, h.victim.isPlayer ? "(player)" : "", h.attackerId, h.sourceId, h.sourceKind, h.weaponType, h.weaponWeight, h.weaponStagger,
				h.flags, blocked ? " BLOCKED" : "", parry ? " PARRY" : "", h.victim.armor, h.victim.hpMax, dmg, before, after, mx, pBefore, pAfter, mp,
				bleeding ? "[bleedout/immune] " : "", (raging && !bleeding) ? "[rage] " : "", poiseBroke ? "POISE-BREAK " : "", postureBroke ? "POSTURE-BREAK" : "");
		}

		if(kind && h.victim.isPlayer && g_cfg.logPlayerEvents && g_cfg.diag)
		{
			float sc = 0.0f, sm = 0.0f;
			ReadAV((Actor *)h.victimRef, 26, &sc, &sm);
			_MESSAGE("PLAYER EVENT: kind=%d (1 light, 2 medium, 3 heavy, 4 bleedout, 5 knockback, 7 exhaustion) attacker=%08X flags=%X blocked=%d stamina=%.0f/%.0f poise=%.0f/%.0f posture=%.0f/%.0f weaponDrawn=%d",
				kind, h.attackerId, h.flags, blocked ? 1 : 0, sc, sm, after, mx, pAfter, mp, ((Actor *)h.victimRef)->actorState.IsWeaponDrawn() ? 1 : 0);
		}

		if(kind)
		{
			if(kind == 7) _MESSAGE("EXHAUSTION: victim=%08X%s ran out of stamina", h.victim.id, h.victim.isPlayer ? "(player)" : "");
			if(kind >= 3 && kind <= 5) _MESSAGE("%s: victim=%08X attacker=%08X (mode %d)", kind == 4 ? "POSTURE BREAK -> BLEEDOUT" : (kind == 5 ? "POISE BREAK -> heavy stagger + KNOCKBACK" : "POISE BREAK -> heavy stagger"), h.victim.id, h.attackerId, g_cfg.staggerMode);
			if(g_cfg.staggerMode == 1 && kind != 4 && kind != 5 && kind != 7) NativeStagger(h.victimRef);
			else SendStaggerEvent(h.victimRef, h.attackerId, kind);
			if(kind >= 3 && kind <= 5) HudFlash(h.victimRef);
		}
	}

	class HitSink : public BSTEventSink<TESHitEvent>
	{
	public:
		virtual EventResult ReceiveEvent(TESHitEvent * evn, EventDispatcher<TESHitEvent> * dispatcher)
		{
			__try
			{
				OnHit(evn);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedIncrement(&g_faults);
			}
			return kEvent_Continue;
		}
	};
	HitSink g_hitSink;

	// ------------------------------------------------------------------ dano de vida (ponto unico do motor)
	// Na LE 1.9.32.0 todo o dano de vida passa por Actor::HandleHealthDamage (0x006E4700), chamada pelas tabelas de funcoes
	// de Actor, Character e PlayerCharacter. Assim o parry sem dano, a reducao de dano da furia e o bonus em BleedOut
	// funcionam sem Papyrus e sem perks. Enderecos e assinatura confirmados no TESV.exe 1.9.32.0.8 e no TrueHUD LE.
	typedef void (__thiscall * HealthDamageFn)(Actor *, Actor *, float);
	HealthDamageFn	g_origHD[3] = { NULL, NULL, NULL };
	bool			g_hdInstalled = false;

	// amount < 0 = dano. Devolve o dano ajustado. Sem objetos com destrutor: pode usar __try.
	volatile LONG g_rageCostBusy = 0;	// o custo de vida da furia nao e reduzido pela propria furia

	// ---- vidas extra: 2a vida no nivel Level2, 3a no nivel Level3; enchem ao dormir (o Papyrus repoe g_livesUsed)
	int LivesUnlocked(Actor * a)
	{
		if(!g_cfg.livesEnabled || !a) return 0;
		const int lvl = (int)CALL_MEMBER_FN(a, GetLevel)();
		int n = 0;
		if(lvl >= g_cfg.livesLevel2) n++;
		if(lvl >= g_cfg.livesLevel3) n++;
		return n;
	}
	int LivesAvailable(Actor * a)
	{
		const int u = LivesUnlocked(a) - (int)g_livesUsed;
		return u > 0 ? u : 0;
	}
	// Essencial: quem marca a flag e o Papyrus (GetActorBase().SetEssential), exatamente como o SkyTweak. Aqui so avisamos.
	void SendEssentialEvent(TESObjectREFR * ref, bool on)
	{
		if(!ref) return;
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_Essential", "", on ? 1.0f : 0.0f, (TESForm *)ref);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}
	// Escreve a flag essencial tambem diretamente (o Papyrus tem latencia; assim a flag esta sempre certa no momento do golpe).
	void SetPlayerEssentialDirect(bool on)
	{
		__try
		{
			Actor * pl = (Actor *)Engine::Player();
			if(!pl || !pl->baseForm || pl->baseForm->formType != kFormType_NPC) return;
			UInt32 & fl = ((TESNPC *)pl->baseForm)->actorData.flags;
			if(on) fl |= 0x02; else fl &= ~0x02u;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {}
	}
	int PlayerEssentialFlag()
	{
		__try
		{
			Actor * pl = (Actor *)Engine::Player();
			if(!pl || !pl->baseForm || pl->baseForm->formType != kFormType_NPC) return -1;
			return (((TESNPC *)pl->baseForm)->actorData.flags & 0x02) ? 1 : 0;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return -1; }
	}
	// Essencial so enquanto ha vidas (e durante o bleedout da vida gasta). Reenvia de 10 em 10 s (o evento pode perder-se logo apos o load).
	void SyncEssential()
	{
		if(!g_cfg.enabled || !g_cfg.livesEnabled || !g_cfg.livesEssential) return;
		Actor * pl = (Actor *)Engine::Player();
		if(!pl) return;
		const DWORD now = GetTickCount();
		const LONG want = (LivesAvailable(pl) > 0 || (LONG)(now - g_invulnUntil) < 0) ? 1 : 0;
		SetPlayerEssentialDirect(want != 0);
		if(want != g_essSent || now - g_essLast > 10000)
		{
			if(want != g_essSent) _MESSAGE("essential mark (player): %d", (int)want);
			g_essSent = want; g_essLast = now;
			SendEssentialEvent((TESObjectREFR *)pl, want != 0);
		}
	}
	// Atacantes recentes do jogador: ao reviver, voltam a lutar (o SkyTweak consegue isto com StopCombatAlarm).
	void NoteAttackerOfPlayer(UInt32 id, DWORD now)
	{
		if(id == 0 || id == 0x14) return;
		int slot = -1, oldest = 0;
		for(int i = 0; i < 4; i++)
		{
			if(g_recentAtk[i].id == id) { slot = i; break; }
			if(g_recentAtk[i].tick < g_recentAtk[oldest].tick) oldest = i;
		}
		if(slot < 0) slot = oldest;
		g_recentAtk[slot].id = id; g_recentAtk[slot].tick = now;
	}
	volatile LONG g_bossPending = 0;	// formID do boss cuja vida extra acabou de ser gasta
	float AdjustHealthDamage(Actor * self, Actor * attacker, float amount)
	{
		if(!(amount < 0.0f) || !(amount > -1.0e9f) || !g_cfg.enabled || g_rageCostBusy) return amount;
		__try
		{
			if(!self || self->formType != kFormType_Character) return amount;
			InterlockedIncrement(&g_hdCalls);
			const UInt32 id = self->formID;
			const DWORD now = GetTickCount();
			const bool isPlayer = (id == 0x14);
			if(isPlayer) SyncEssential();
			float result = amount;
			const char * why = "";

			// Janela do parry: so o tempo decide (e o atacante estar a frente). Fora dela o dano e o bloqueio normal do jogo ficam como estao.
			if(isPlayer && attacker && attacker != self && DeflectBlocks(attacker->formID, now))
			{
				result = 0.0f;
				why = "deflect";
				InterlockedIncrement(&g_hdParryZero);
			}
			else if(isPlayer && !g_cfg.inpaMode && g_cfg.parryNoDamage && attacker && attacker != self && ParryWindowActive(now) && InFront(self, attacker))
			{
				result = 0.0f;
				why = "parry";
				InterlockedIncrement(&g_hdParryZero);
			}
			else
			{
				float mult = 1.0f;
				Lock();
				StateMap::iterator it = g_states.find(id);
				if(it != g_states.end())
				{
					const PoiseState & st = it->second;
					if(g_cfg.rageEnabled && now < st.rageUntil)
					{
						float red = isPlayer ? g_cfg.rageDamageReduction : g_cfg.rageNpcDamageReduction;
						if(red < 0.0f) red = 0.0f;
						if(red > 100.0f) red = 100.0f;
						mult *= 1.0f - red / 100.0f;
						why = "rage";
						InterlockedIncrement(&g_hdRage);
					}
					if(now < st.bleedUntil && g_cfg.bleedDamageMult != 1.0f)
					{
						mult *= g_cfg.bleedDamageMult;
						why = "bleedout";
						InterlockedIncrement(&g_hdBleed);
					}
				}
				if(isPlayer && attacker && attacker != self && PerilousActive(attacker->formID, now)) { mult *= g_cfg.perilDmgMult; if(why[0] == 0) why = "perilous"; }
				// bonus de dano de quem esta em furia (jogador ou NPC), em armas e magias
				if(g_cfg.rageEnabled && attacker && attacker != self && attacker->formID != id)
				{
					StateMap::iterator ia = g_states.find(attacker->formID);
					if(ia != g_states.end() && now < ia->second.rageUntil)
					{
						float bonus = (attacker->formID == 0x14) ? g_cfg.rageDamageBonus : g_cfg.rageNpcDamageBonus;
						if(bonus < 0.0f) bonus = 0.0f;
						if(bonus > 300.0f) bonus = 300.0f;
						mult *= 1.0f + bonus / 100.0f;
						if(why[0] == 0) why = "rage bonus";
						InterlockedIncrement(&g_hdRage);
					}
				}
				Unlock();
				result = amount * mult;
			}
			if(!isPlayer && result < 0.0f && g_cfg.enabled)
			{
				bool inv = false, lose = false;
				Lock();
				{
					StateMap::iterator ib = g_states.find(id);
					if(ib != g_states.end())
					{
						if(now < ib->second.invulnUntil) inv = true;
						else if(ib->second.bossUsed == 0) lose = true;
					}
				}
				Unlock();
				if(inv) { result = 0.0f; why = "boss invulnerable"; }
				else if(lose && IsBossActor(self))
				{
					// marca-o como essencial logo no 1o golpe (o jogo faz o bleedout verdadeiro quando a vida acaba)
					bool first = false;
					Lock();
					first = g_bossMarked.insert(id).second;
					Unlock();
					if(first) { SendEssentialEvent((TESObjectREFR *)self, true); _MESSAGE("essential mark (boss %08X): 1", id); }
					float curHp = 0.0f, maxHp = 0.0f;
					if(ReadAV(self, 24, &curHp, &maxHp) && curHp + result <= 0.5f && !first)
					{
						Lock();
						{
							StateMap::iterator ib = g_states.find(id);
							if(ib != g_states.end()) { ib->second.bossUsed = 1; ib->second.invulnUntil = now + (DWORD)((g_cfg.bleedSeconds + 3.0f + g_cfg.bossInvuln) * 1000.0f); }
						}
						Unlock();
						InterlockedExchange(&g_bossPending, (LONG)id);
						why = "boss life (essential bleedout)";
					}
				}
			}
			if(isPlayer && attacker && attacker != self) NoteAttackerOfPlayer(attacker->formID, now);
			if(isPlayer && g_cfg.livesEnabled)
			{
				if(result < 0.0f && now < g_invulnUntil)
				{
					result = 0.0f;
					why = "life invulnerability";
				}
				else if(result < 0.0f)
				{
					float curHp = 0.0f, maxHp = 0.0f;
					if(ReadAV(self, 24, &curHp, &maxHp) && curHp + result <= 0.5f && LivesAvailable(self) > 0)
					{
						InterlockedIncrement(&g_livesUsed);
						if(g_cfg.livesEssential)
						{
							// o dano passa inteiro: o jogo, vendo o jogador essencial (marcado desde o load), faz o bleedout verdadeiro
							SetPlayerEssentialDirect(true);
							const float bs = g_cfg.bleedSeconds < 5.0f ? 5.0f : g_cfg.bleedSeconds;
							g_invulnUntil = now + (DWORD)((bs + 2.0f + g_cfg.livesInvuln) * 1000.0f);
							// o jogador fica "em bleedout" tambem para a nossa logica: sem stagger leve nem golpes a contar (isso cortava o bleedout)
							Lock();
							{
								StateMap::iterator ip = g_states.find(id);
								if(ip != g_states.end())
								{
									ip->second.bleedUntil = now + (DWORD)((bs + 1.0f) * 1000.0f);
									ip->second.bleedImmuneUntil = ip->second.bleedUntil + (DWORD)(g_cfg.bleedExtraImmune * 1000.0f);
									ip->second.pCur = 0.0f; ip->second.cur = ip->second.max;
								}
							}
							Unlock();
							why = "extra life (essential bleedout)";
						}
						else
						{
							result = -(curHp - 1.0f);
							if(result > 0.0f) result = 0.0f;
							g_invulnUntil = now + (DWORD)(g_cfg.livesInvuln * 1000.0f);
							why = "extra life";
						}
						InterlockedExchange(&g_lifePending, 1);
					}
				}
			}
			if(isPlayer)
			{
				static volatile LONG lethalLogged = 0;
				float ch = 0.0f, mh = 0.0f;
				if(lethalLogged < 60 && ReadAV(self, 24, &ch, &mh) && ch + result <= 0.0f)
				{
					InterlockedIncrement(&lethalLogged);
					_MESSAGE("PLAYER LETHAL HIT: hp=%.1f/%.1f damage=%.1f attacker=%08X (%s) essential=%d lives=%d invuln=%d msSinceExecution=%d", ch, mh, -result, attacker ? attacker->formID : 0, why,
						(int)PlayerEssentialFlag(), (int)LivesAvailable(self), (int)((LONG)(now - g_invulnUntil) < 0), g_execTick ? (int)(now - g_execTick) : -1);
				}
			}
			if(isPlayer && g_cfg.diag && g_cfg.logPlayerDamage && g_hdPlayerLogged < 150)
			{
				InterlockedIncrement(&g_hdPlayerLogged);
				_MESSAGE("player health damage: %.1f (after our multipliers %.1f) attacker=%08X", -amount, -result, attacker ? attacker->formID : 0);
			}
			if(g_cfg.diag && result != amount && g_hdLogged < 80)
			{
				InterlockedIncrement(&g_hdLogged);
				_MESSAGE("health hook: victim=%08X attacker=%08X damage %.1f -> %.1f (%s)", id, attacker ? attacker->formID : 0, -amount, -result, why);
			}
			return result;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			InterlockedIncrement(&g_faults);
			return amount;
		}
	}

	void ReviveBoss(Actor * self, UInt32 id);
	void __fastcall HookHD_Actor(Actor * self, void *, Actor * attacker, float amount)
	{
		amount = AdjustHealthDamage(self, attacker, amount);
		g_origHD[0](self, attacker, amount);
		if(g_bossPending && self && (UInt32)g_bossPending == self->formID) ReviveBoss(self, self->formID);
	}
	void __fastcall HookHD_Character(Actor * self, void *, Actor * attacker, float amount)
	{
		amount = AdjustHealthDamage(self, attacker, amount);
		g_origHD[1](self, attacker, amount);
		if(g_bossPending && self && (UInt32)g_bossPending == self->formID) ReviveBoss(self, self->formID);
	}
	// Poe o ator em BleedOut (estado + evento para o Papyrus): usado ao gastar uma vida extra.
	void StartForcedBleed(Actor * self)
	{
		const DWORD now = GetTickCount();
		Lock();
		{
			StateMap::iterator ib = g_states.find(self->formID);
			if(ib != g_states.end())
			{
				ib->second.bleedUntil = now + (DWORD)(g_cfg.bleedSeconds * 1000.0f);
				ib->second.bleedImmuneUntil = ib->second.bleedUntil + (DWORD)(g_cfg.bleedExtraImmune * 1000.0f);
				ib->second.pCur = 0.0f;
				ib->second.cur = ib->second.max;
			}
		}
		Unlock();
		SendStaggerEvent((TESObjectREFR *)self, 0, 4);
	}
	// Depois de o dano que mataria o jogador ter sido limitado a deixar 1 de vida: devolve a vida e avisa o Papyrus (ouro, aviso).
	void ReviveExtraLife(Actor * self)
	{
		InterlockedExchange(&g_lifePending, 0);
		if(g_cfg.livesEssential)
		{
			_MESSAGE("EXTRA LIFE used (#%d): the game does the real bleedout (essential flag=%d), the script restores the health afterwards", (int)g_livesUsed, PlayerEssentialFlag());
			SendLifeEvent((TESObjectREFR *)self, (int)g_livesUsed);
			return;
		}
		float cur = 0.0f, mx = 0.0f;
		if(ReadAV(self, 24, &cur, &mx))
		{
			float target = mx * g_cfg.livesHealthPct / 100.0f;
			if(target < 1.0f) target = 1.0f;
			if(target > cur) RestoreAV(self, 24, target - cur);
		}
		_MESSAGE("EXTRA LIFE used (#%d): health back to %.0f%%, invulnerable %.1f s", (int)g_livesUsed, g_cfg.livesHealthPct, g_cfg.livesInvuln);
		StartForcedBleed(self);
		const DWORD now = GetTickCount();
		g_invulnUntil = now + (DWORD)((g_cfg.bleedSeconds + g_cfg.livesInvuln) * 1000.0f);
		SendLifeEvent((TESObjectREFR *)self, (int)g_livesUsed);
	}
	// Vida do boss gasta: o jogo fez o bleedout (boss essencial); o script espera, repoe a vida e volta ao combate.
	void ReviveBoss(Actor * self, UInt32 id)
	{
		InterlockedExchange(&g_bossPending, 0);
		_MESSAGE("BOSS LIFE used: %08X (essential bleedout; the script restores the health afterwards)", id);
		EventDispatcher<PoiseModEvent> * d = NULL;
		if(g_messaging) d = (EventDispatcher<PoiseModEvent> *)g_messaging->GetEventDispatcher(SKSEMessagingInterface::kDispatcher_ModEvent);
		if(!d) return;
		PoiseModEvent evn("NahIDWin_BossLife", "", g_cfg.bleedSeconds, (TESForm *)self);
		if(!SafeSend(d, &evn)) InterlockedIncrement(&g_faults);
	}

	void __fastcall HookHD_Player(Actor * self, void *, Actor * attacker, float amount)
	{
		amount = AdjustHealthDamage(self, attacker, amount);
		g_origHD[2](self, attacker, amount);
		if(g_lifePending) ReviveExtraLife(self);
	}

	bool IsExecutableAddr(const void * p)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if(!p || !VirtualQuery(p, &mbi, sizeof(mbi))) return false;
		const DWORD x = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		return mbi.State == MEM_COMMIT && (mbi.Protect & x) && !(mbi.Protect & PAGE_GUARD);
	}

	// Troca uma posicao da tabela de funcoes, encadeando com o que la estiver (outro mod pode ja ter trocado).
	bool InstallSlot(UInt32 addr, int idx, void * replacement)
	{
		void ** slot = (void **)addr;
		DWORD protection = 0, ignored = 0;
		if(!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &protection)) return false;
		void * previous = NULL;
		bool ok = false;
		for(int tries = 0; tries < 8; tries++)
		{
			previous = *slot;
			if(!IsExecutableAddr(previous)) break;
			g_origHD[idx] = (HealthDamageFn)previous;
			if(InterlockedCompareExchangePointer(slot, replacement, previous) == previous) { ok = true; break; }
		}
		VirtualProtect(slot, sizeof(void *), protection, &ignored);
		if(ok) _MESSAGE("health hook: slot %08X chained to %p", (unsigned)addr, previous);
		else { g_origHD[idx] = NULL; _MESSAGE("health hook: could not install on slot %08X", (unsigned)addr); }
		return ok;
	}

	void InstallHealthHook()
	{
		if(g_hdInstalled) return;
		// a funcao tem de ser mesmo a que esperamos (assinatura do TESV.exe 1.9.32.0.8)
		static const unsigned char expect[16] = { 0x51,0x56,0x8B,0xF1,0x8B,0x06,0x8B,0x90,0x64,0x02,0x00,0x00,0x6A,0x00,0xFF,0xD2 };
		unsigned char have[16];
		if(!ReadBytes((void *)0x006E4700, have, 16) || memcmp(have, expect, 16) != 0)
		{
			_MESSAGE("health hook: the game function at 006E4700 does not match - NOT installed (parry/rage damage reduction disabled)");
			return;
		}
		const bool a = InstallSlot(0x010D00C8, 0, (void *)&HookHD_Actor);
		const bool c = InstallSlot(0x010D1AE0, 1, (void *)&HookHD_Character);
		const bool p = InstallSlot(0x010D2428, 2, (void *)&HookHD_Player);
		// so e seguro se as tres tabelas ficaram trocadas (uma so trocada deixaria o jogador sem protecao)
		g_hdInstalled = a && c && p;
		_MESSAGE("health hook: %s", g_hdInstalled ? "installed (parry, rage damage reduction, bleedout bonus)" : "PARTIAL - the hooks that did install stay active but some actors are not covered");
		if(!g_hdInstalled) g_hdInstalled = (a || c || p);
	}


	// ------------------------------------------------------------------ escudo de stagger
	// O jogo pede a reacao a um golpe ao grafo de animacao (staggerStart, recoilStart...). Durante a furia (jogador e NPCs)
	// esses pedidos sao ignorados. Os nossos proprios pedidos (g_ownNotify) passam sempre. A funcao original fica encadeada.
	struct NotifyHookEntry { void ** vtbl; void * orig; };
	NotifyHookEntry g_nh[8];
	int				g_nhCount = 0;
	typedef bool (__thiscall * NotifyFnT)(void *, const BSFixedString *);

	void * FindNotifyOrig(void ** vtbl)
	{
		for(int i = 0; i < g_nhCount; i++) if(g_nh[i].vtbl == vtbl) return g_nh[i].orig;
		return NULL;
	}

	volatile DWORD g_exhaustBlockUntil = 0;	// exaustao do jogador: ataque, ataque forte, bloqueio, salto e bash ficam bloqueados no grafo de animacao
	volatile DWORD g_ownAnimUntil = 0;		// o Papyrus esta a enviar a animacao de bleedout/stagger do mod
	struct BleedAcc { UInt32 id; DWORD tick; int ok; };
	BleedAcc g_bleedAcc[8];
	int g_bleedAccN = 0;
	bool IsRagingId(UInt32 id)
	{
		const DWORD now = GetTickCount();
		bool r = false;
		Lock();
		StateMap::iterator it = g_states.find(id);
		if(it != g_states.end() && (now < it->second.rageUntil || now < it->second.bleedUntil)) r = true;
		Unlock();
		return r;
	}

	// Pedido de reacao a golpe? (comeca por stagger/recoil e nao e de paragem)
	bool IsHitReactionName(const char * n)
	{
		if(!n) return false;
		if(_strnicmp(n, "stagger", 7) != 0 && _strnicmp(n, "recoil", 6) != 0) return false;
		if(strstr(n, "Stop") || strstr(n, "stop") || strstr(n, "End") || strstr(n, "Exit")) return false;
		return true;
	}

	// Pedidos de acao do jogador que a exaustao impede (as paragens passam sempre).
	bool IsExhaustBlockedName(const char * n)
	{
		if(!n) return false;
		if(strstr(n, "Stop") || strstr(n, "stop") || strstr(n, "End") || strstr(n, "Exit") || strstr(n, "Release") || strstr(n, "release")) return false;
		if(_strnicmp(n, "attack", 6) == 0 || _strnicmp(n, "bash", 4) == 0 || _strnicmp(n, "blockStart", 10) == 0) return true;
		if(_strnicmp(n, "jump", 4) == 0 && (strstr(n, "Start") || strstr(n, "start"))) return true;
		return false;
	}

	// Sem objetos com destrutor: pode usar __try.
	bool ShouldBlockNotify(void * holder, const BSFixedString * evn)
	{
		__try
		{
			if(!g_ownNotify && g_cfg.enabled && g_exhaustBlockUntil && (LONG)(GetTickCount() - g_exhaustBlockUntil) < 0 && evn && IsExhaustBlockedName(evn->data))
			{
				const UInt32 pid = *(UInt32 *)((UInt8 *)holder - 0x20 + 0x0C);
				if(pid == 0x14) { InterlockedIncrement(&g_shieldBlocked); return true; }
			}
			if(g_ownNotify || !g_cfg.rageStaggerShield || !g_cfg.enabled) return false;
			if((LONG)(GetTickCount() - g_ownAnimUntil) < 0) return false;
			if(!evn || !IsHitReactionName(evn->data)) return false;
			const UInt32 id = *(UInt32 *)((UInt8 *)holder - 0x20 + 0x0C);	// formID do ator (holder = ator + 0x20)
			if(!IsRagingId(id)) return false;
			InterlockedIncrement(&g_shieldBlocked);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	// Regista se o grafo de animacao aceitou o pedido de BleedoutStart (o Papyrus repete ate aceitar).
	void NoteBleedResult(void * holder, const BSFixedString * evn, bool ok)
	{
		__try
		{
			if(!evn || !evn->data || _stricmp(evn->data, "BleedoutStart") != 0) return;
			const UInt32 id = *(UInt32 *)((UInt8 *)holder - 0x20 + 0x0C);
			int k = -1;
			for(int i = 0; i < g_bleedAccN; i++) if(g_bleedAcc[i].id == id) { k = i; break; }
			if(k < 0) { k = (g_bleedAccN < 8) ? g_bleedAccN++ : 0; }
			g_bleedAcc[k].id = id; g_bleedAcc[k].tick = GetTickCount(); g_bleedAcc[k].ok = ok ? 1 : 0;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {}
	}

	bool __fastcall HookNotify(void * holder, void *, const BSFixedString * evn)
	{
		void ** vtbl = NULL;
		__try { vtbl = *(void ***)holder; } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
		void * orig = FindNotifyOrig(vtbl);
		if(!orig) return false;
		if(ShouldBlockNotify(holder, evn)) return true;
		const bool r = ((NotifyFnT)orig)(holder, evn);
		NoteBleedResult(holder, evn, r);
		return r;
	}

	void EnsureNotifyHook(TESObjectREFR * ref)
	{
		if(!ref || !g_cfg.rageStaggerShield) return;
		void * holder = (UInt8 *)ref + 0x20;
		void ** vtbl = NULL;
		__try { vtbl = *(void ***)holder; } __except(EXCEPTION_EXECUTE_HANDLER) { return; }
		if(!vtbl || FindNotifyOrig(vtbl) || g_nhCount >= 8) return;
		void ** slot = &vtbl[g_cfg.nativeSlot];
		void * previous = *slot;
		if(!IsExecutableAddr(previous)) { _MESSAGE("stagger shield: slot not executable, NOT installed"); return; }
		DWORD protection = 0, ignored = 0;
		if(!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &protection)) return;
		g_nh[g_nhCount].vtbl = vtbl;
		g_nh[g_nhCount].orig = previous;
		g_nhCount++;
		*slot = (void *)&HookNotify;
		VirtualProtect(slot, sizeof(void *), protection, &ignored);
		_MESSAGE("stagger shield: hook installed on table %p (slot %d chained to %p)", vtbl, g_cfg.nativeSlot, previous);
	}

	// ------------------------------------------------------------------ dificuldade do jogo (vida estilo Sekiro)
	SettingCollectionMap ** g_gmstColl = (SettingCollectionMap **)0x012E7164;

	bool SetGMSTFloat(const char * name, float value)
	{
		__try
		{
			SettingCollectionMap * c = *g_gmstColl;
			if(!c) return false;
			Setting * st = c->items.Get(name);
			if(!st || !st->name || st->name[0] != 'f') return false;
			st->data.f32 = value;
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	void ApplyHealthGMST()
	{
		if(!g_cfg.healthEnabled || !g_dataLoaded) return;
		static const char * toPC[6] = { "fDiffMultHPToPCVE", "fDiffMultHPToPCE", "fDiffMultHPToPCN", "fDiffMultHPToPCH", "fDiffMultHPToPCVH", "fDiffMultHPToPCL" };
		static const char * byPC[6] = { "fDiffMultHPByPCVE", "fDiffMultHPByPCE", "fDiffMultHPByPCN", "fDiffMultHPByPCH", "fDiffMultHPByPCVH", "fDiffMultHPByPCL" };
		int ok = 0;
		for(int i = 0; i < 6; i++)
		{
			if(SetGMSTFloat(toPC[i], g_cfg.healthTaken)) ok++;
			if(SetGMSTFloat(byPC[i], g_cfg.healthDealt)) ok++;
		}
		_MESSAGE("health: set %d/12 difficulty multipliers (damage taken x%.2f, damage dealt x%.2f - the game difficulty no longer changes health damage)", ok, g_cfg.healthTaken, g_cfg.healthDealt);
	}

	// ------------------------------------------------------------------ furia do jogador (tecla)
	void TryRagePlayer()
	{
		if(!g_cfg.enabled || !g_cfg.rageEnabled) return;
		PlayerCharacter * p = Engine::Player();
		if(!p) return;
		ActorStats s;
		if(!ReadActorStats((Actor *)p, &s) || !s.alive) return;
		EnsureNotifyHook((TESObjectREFR *)p);
		const DWORD now = GetTickCount();
		bool ok = false;
		DWORD left = 0;
		Lock();
		{
			PoiseState & st = FindOrCreate(s.id, ComputeMax(s), ComputeMaxPosture(s), now);
			if(now >= st.rageReadyAt && now >= st.bleedUntil && now >= st.rageUntil)
			{
				st.rageUntil = now + (DWORD)(g_cfg.rageSeconds * 1000.0f);
				st.rageReadyAt = now + (DWORD)(g_cfg.rageCooldown * 1000.0f);
				ok = true;
			}
			else if(st.rageReadyAt > now) left = st.rageReadyAt - now;
		}
		Unlock();
		if(!ok)
		{
			if(g_cfg.diag) _MESSAGE("RAGE (player): refused (cooldown %.1f s left, or in bleedout)", left / 1000.0f);
			return;
		}
		InterlockedIncrement(&g_rages);
		float cost = 0.0f, cur = 0.0f, m = 0.0f;
		const UInt32 av = g_cfg.rageCostType == 0 ? 24 : 26;
		if(ReadAV((Actor *)p, av, &cur, &m))
		{
			cost = m * (av == 24 ? g_cfg.rageLifeCost : g_cfg.rageCostPercent) / 100.0f;
			if(av == 24 && cost > cur - 1.0f) cost = cur - 1.0f;	// nunca mata o jogador
			if(cost < 0.0f) cost = 0.0f;
			g_rageCostBusy = 1;
			if(cost > 0.0f) RestoreAV((Actor *)p, av, -cost);
			g_rageCostBusy = 0;
		}
		_MESSAGE("RAGE (player): active for %.1f s, extra reduction %.0f%%, damage bonus %.0f%%, cost %.1f %s", g_cfg.rageSeconds, g_cfg.rageDamageReduction, g_cfg.rageDamageBonus, cost, g_cfg.rageCostType == 0 ? "health" : "stamina");
		HudFlash((TESObjectREFR *)p);
		SendRageEvent((TESObjectREFR *)p, g_cfg.rageSeconds);
	}

	// ------------------------------------------------------------------ Supremo Rage (aura): fuego em redor, custo por segundo
	// Sem objetos com destrutor: pode usar __try. Queima os inimigos hostis dentro do raio, na celula do jogador.
	int SupremeBurn(Actor * pl, float radius, float dmg)
	{
		int n = 0;
		__try
		{
			TESObjectCELL * cell = pl->parentCell;
			if(!cell) return 0;
			TESObjectREFR ** arr = cell->objectList.arr.entries;
			const UInt32 cnt = cell->objectList.count;
			if(!arr) return 0;
			const float px = pl->pos.x, py = pl->pos.y, pz = pl->pos.z;
			for(UInt32 i = 0; i < cnt && i < 8000 && n < 24; i++)
			{
				TESObjectREFR * r = arr[i];
				if(!r || r == (TESObjectREFR *)pl || r->formType != kFormType_Character) continue;
				const float dx = r->pos.x - px, dy = r->pos.y - py, dz = r->pos.z - pz;
				if(dx * dx + dy * dy + dz * dz > radius * radius) continue;
				Actor * a = (Actor *)r;
				ActorStats st;
				if(!ReadActorStats(a, &st) || !st.alive) continue;
				if(!CALL_MEMBER_FN(a, IsHostileToActor)(pl)) continue;
				float cur = 0.0f, mx = 0.0f;
				if(!ReadAV(a, 24, &cur, &mx) || cur <= 0.0f) continue;
				RestoreAV(a, 24, -dmg);
				n++;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		}
		return n;
	}

	void TickSupreme(DWORD now)
	{
		if(!g_cfg.supEnabled || now >= g_supUntil) return;
		PlayerCharacter * p = Engine::Player();
		if(!p) return;
		if(now - g_supLastHit >= 500)
		{
			g_supLastHit = now;
			if(g_cfg.supDps > 0.0f) SupremeBurn((Actor *)p, g_cfg.supRadius, g_cfg.supDps * 0.5f);
		}
		if(now - g_supLastCost >= 1000)
		{
			g_supLastCost = now;
			float cur = 0.0f, mx = 0.0f;
			if(ReadAV((Actor *)p, 24, &cur, &mx))
			{
				float cost = mx * g_cfg.supCostPerSec / 100.0f;
				if(cost > cur - 1.0f) cost = cur - 1.0f;	// nunca mata o jogador
				if(cost > 0.0f)
				{
					g_rageCostBusy = 1;
					RestoreAV((Actor *)p, 24, -cost);
					g_rageCostBusy = 0;
				}
			}
		}
	}

	void TrySupremePlayer()
	{
		if(!g_cfg.enabled || !g_cfg.rageEnabled || !g_cfg.supEnabled) return;
		PlayerCharacter * p = Engine::Player();
		if(!p) return;
		ActorStats s;
		if(!ReadActorStats((Actor *)p, &s) || !s.alive) return;
		EnsureNotifyHook((TESObjectREFR *)p);
		const DWORD now = GetTickCount();
		if(now < g_supUntil)
		{
			// segunda pressao: desliga e comeca a recarga
			g_supUntil = now;
			Lock();
			{
				PoiseState & st = FindOrCreate(s.id, ComputeMax(s), ComputeMaxPosture(s), now);
				st.rageUntil = now;
			}
			Unlock();
			g_supReadyAt = now + (DWORD)(g_cfg.supCooldown * 1000.0f);
			SendRageEvent((TESObjectREFR *)p, 0.0f, "supreme");
			_MESSAGE("SUPREME RAGE (player): turned off, cooldown %.0f s", g_cfg.supCooldown);
			return;
		}
		bool ok = false;
		DWORD left = 0;
		Lock();
		{
			PoiseState & st = FindOrCreate(s.id, ComputeMax(s), ComputeMaxPosture(s), now);
			if(now >= g_supReadyAt && now >= st.bleedUntil && now >= st.rageUntil)
			{
				st.rageUntil = now + (DWORD)(g_cfg.supSeconds * 1000.0f);
				ok = true;
			}
			else if(g_supReadyAt > now) left = g_supReadyAt - now;
		}
		Unlock();
		if(!ok)
		{
			if(g_cfg.diag) _MESSAGE("SUPREME RAGE (player): refused (cooldown %.0f s left, or another rage/bleedout active)", left / 1000.0f);
			return;
		}
		g_supUntil = now + (DWORD)(g_cfg.supSeconds * 1000.0f);
		g_supReadyAt = g_supUntil + (DWORD)(g_cfg.supCooldown * 1000.0f);
		g_supLastCost = now;
		g_supLastHit = now;
		InterlockedIncrement(&g_rages);
		_MESSAGE("SUPREME RAGE (player): active for %.1f s, cost %.1f%%/s of health, fire %.1f dmg/s in radius %.0f, cooldown %.0f s", g_cfg.supSeconds, g_cfg.supCostPerSec, g_cfg.supDps, g_cfg.supRadius, g_cfg.supCooldown);
		HudFlash((TESObjectREFR *)p);
		SendRageEvent((TESObjectREFR *)p, g_cfg.supSeconds, "supreme");
	}

	bool PlayerRaging()
	{
		PlayerCharacter * p = Engine::Player();
		if(!p) return false;
		const DWORD now = GetTickCount();
		bool r = false;
		Lock();
		{
			StateMap::iterator it = g_states.find(0x14);
			if(it != g_states.end() && now < it->second.rageUntil) r = true;
		}
		Unlock();
		return r;
	}

	// ------------------------------------------------------------------ teclas: bloqueio (parry) e furia
	bool AnyMenuOpen()
	{
		MenuManager * mm = MenuManager::GetSingleton();
		if(!mm) return true;
		static const char * names[] = { "Console", "Journal Menu", "InventoryMenu", "MagicMenu", "ContainerMenu", "BarterMenu", "GiftMenu", "MapMenu", "StatsMenu",
			"TweenMenu", "Book Menu", "Dialogue Menu", "Lockpicking Menu", "Sleep/Wait Menu", "Loading Menu", "MainMenu", "FavoritesMenu", "Crafting Menu",
			"Training Menu", "RaceSex Menu", "MessageBoxMenu", "Console Native UI Menu" };
		for(int i = 0; i < (int)(sizeof(names) / sizeof(names[0])); i++)
		{
			BSFixedString n(names[i]);
			if(mm->IsMenuOpen(&n)) return true;
		}
		return false;
	}

	void OnBlockDown()
	{
		if(InterlockedExchange(&g_blockHeld, 1) != 0) return;
		g_parryUntil = 0;
		if(!g_cfg.enabled || !g_cfg.parryEnabled || g_cfg.inpaMode) return;
		if(AnyMenuOpen()) return;
		PlayerCharacter * p = Engine::Player();
		if(!p || Engine::LifeState((Actor *)p) != 0) return;
		if(!p->actorState.IsWeaponDrawn()) return;
		const DWORD now = GetTickCount();
		if(now < g_parryReadyAt) return;	// recarga curta entre janelas
		g_parryUntil = now + (DWORD)g_cfg.parryWindowMs;
		g_parryReadyAt = g_parryUntil + (DWORD)g_cfg.parryCooldownMs;
		InterlockedExchange(&g_parryHitDone, 0);
		_MESSAGE("parry input: armed for %d ms", g_cfg.parryWindowMs);
	}

	void OnBlockUp()
	{
		InterlockedExchange(&g_blockHeld, 0);
		if(g_cfg.parryRequireHeld) g_parryUntil = 0;
	}

	void OnInput(InputEvent * first)
	{
		FlushBars(GetTickCount());	// limpa as barras quando o TrueHUD deixou de as mostrar
		InputStringHolder * strings = InputStringHolder::GetSingleton();
		for(InputEvent * ev = first; ev; ev = ev->next)
		{
			if(ev->eventType != InputEvent::kEventType_Button) continue;
			ButtonEvent * be = (ButtonEvent *)ev;
			// O campo flags e tratado pelo motor como float value (1.0 premido, 0.0 solto).
			float value = 0.0f;
			memcpy(&value, &be->flags, sizeof(value));
			const bool down = (value != 0.0f) && (be->timer == 0.0f);
			const bool up = (value == 0.0f) && (be->timer != 0.0f);
			const BSFixedString * control = ev->GetControlID();
			const char * id = control ? control->data : NULL;
			const bool isBlockStart = control && strings && (*control == strings->blockStart);
			const bool isBlockStop = control && strings && (*control == strings->blockStop);
			const bool isLeftAttack = control && strings && (*control == strings->leftAttack);
			const bool isLegacyBlockControl = id && strcmp(id, "Left Attack/Block") == 0;
			if(isBlockStart || isBlockStop || isLeftAttack || isLegacyBlockControl)
			{
				if(g_cfg.diag && InterlockedIncrement(&g_blockInputLogCount) <= 80)
					_MESSAGE("block input: control=%s value=%.1f held=%.3f device=%d down=%d up=%d", id ? id : "(null)", value, be->timer, (int)ev->deviceType, down ? 1 : 0, up ? 1 : 0);
				if(isBlockStart || ((isLeftAttack || isLegacyBlockControl) && down)) OnBlockDown();
				else if(isBlockStop || ((isLeftAttack || isLegacyBlockControl) && up)) OnBlockUp();
			}
			else if(down && g_cfg.rageEnabled && ev->deviceType == kDeviceType_Keyboard && be->keyMask == (UInt32)g_cfg.rageKey && !AnyMenuOpen())
			{
				TryRagePlayer();
			}
			else if(down && g_cfg.supEnabled && g_cfg.rageEnabled && ev->deviceType == kDeviceType_Keyboard && be->keyMask == (UInt32)g_cfg.supKey && !AnyMenuOpen())
			{
				TrySupremePlayer();
			}
		}
	}

	class InputSink : public BSTEventSink<InputEvent>
	{
	public:
		virtual EventResult ReceiveEvent(InputEvent ** evn, InputEventDispatcher * dispatcher)
		{
			__try
			{
				if(evn && *evn) OnInput(*evn);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				InterlockedIncrement(&g_faults);
			}
			return kEvent_Continue;
		}
	};
	InputSink g_inputSink;

	// ------------------------------------------------------------------ funcoes nativas para o Papyrus
	void N_ScriptReady(StaticFunctionTag *)
	{
		InterlockedIncrement(&g_scriptReady);
		_MESSAGE("papyrus: NahIDWinBridge script started and registered for NahIDWin_Stagger (#%d)", (int)g_scriptReady);
	}

	void N_SetLivesUsed(StaticFunctionTag *, SInt32 n)
	{
		if(n < 0) n = 0;
		if(n > 2) n = 2;
		g_livesUsed = n;
		_MESSAGE("papyrus: extra lives used set to %d", (int)n);
		SyncEssential();
	}
	void N_Ack(StaticFunctionTag *, SInt32 packed)
	{
		const DWORD now = GetTickCount();
		const LONG seq = (packed >> 3) & 0x1FFFFF;
		InterlockedIncrement(&g_acks);
		Lock();
		if(g_sentSeq[seq & 63] == seq)
		{
			const DWORD lat = now - g_sentTick[seq & 63];
			g_latSum += lat; g_latCount++;
			if(lat > g_latMax) g_latMax = lat;
			if(g_cfg.diag && g_cfg.logHitsMax > 0) _MESSAGE("papyrus: stagger #%d answered after %d ms (tier %d)", (int)seq, (int)lat, (int)(packed & 7));
		}
		Unlock();
		WriteSummary(now, false);
	}

	float N_GetBleedSeconds(StaticFunctionTag *)
	{
		return g_cfg.bleedSeconds;
	}

	// Armadura do jogador (0 se nao conseguir ler).
	float PlayerArmor()
	{
		PlayerCharacter * p = Engine::Player();
		if(!p) return 0.0f;
		ActorStats s;
		if(!ReadActorStats((Actor *)p, &s)) return 0.0f;
		return s.armor;
	}

	// Custo de estamina de um golpe do jogador. mode: 0 balanco normal, 1 golpe forte, 2 golpe de escudo.
	// rightType: codigo do Papyrus (GetEquippedItemType). Devolve 0 se estiver desligado.
	float N_GetSwingCost(StaticFunctionTag *, SInt32 mode, SInt32 rightType)
	{
		if(!g_cfg.stamEnabled || g_cfg.swingCostPlayer <= 0.0f) return 0.0f;
		if(g_cfg.rageEnabled && g_cfg.rageZeroStamina && PlayerRaging()) return 0.0f;	// furia: golpes gratis
		const DWORD now = GetTickCount();
		if(mode == 0 && now < g_powerUntil) return 0.0f;	// o golpe forte ja pagou
		int ty = rightType;
		if(ty < 0 || ty > 12) ty = 1;
		float c = g_cfg.swingCostPlayer * g_cfg.typeMult[ty];
		if(mode == 1) { c = g_cfg.swingCostPlayer * g_cfg.typeMult[ty] * g_cfg.powerOwnCost; g_powerUntil = now + 1500; }
		else if(mode == 2) c = g_cfg.swingCostPlayer * g_cfg.bashCostMult;
		c *= 1.0f + (PlayerArmor() / 100.0f) * g_cfg.armorCostFactor;
		if(!(c > 0.0f && c < 10000.0f)) c = 0.0f;
		static volatile LONG costLogged = 0;
		if(g_cfg.diag && costLogged < 200) { InterlockedIncrement(&costLogged); _MESSAGE("swing cost: mode=%d weaponType=%d -> %.1f stamina (power attacks: own cost x%.2f, the game's own fPowerAttackStaminaPenalty is separate)", (int)mode, (int)rightType, c, g_cfg.powerOwnCost); }
		return c;
	}

	// Regeneracao de estamina do jogador em % (100 = normal). A armadura abranda.
	float N_GetRegenPercent(StaticFunctionTag *)
	{
		if(!g_cfg.stamEnabled || !g_cfg.armorRegenEnabled) return -1.0f;	// -1 = nao mexer na regeneracao
		float r = 100.0f / (1.0f + (PlayerArmor() / 100.0f) * g_cfg.armorRegenFactor);
		return (r < 10.0f) ? 10.0f : r;
	}

	float N_GetExhaustSeconds(StaticFunctionTag *)
	{
		return g_cfg.exhaustSeconds;
	}

	// O Papyrus viu a estamina do jogador a zero depois de pagar um golpe. true = pode entrar em exaustao agora.
	bool N_TryExhaust(StaticFunctionTag *)
	{
		if(!g_cfg.enabled || !g_cfg.exhaustEnabled) return false;
		PlayerCharacter * p = Engine::Player();
		if(!p) return false;
		ActorStats s;
		if(!ReadActorStats((Actor *)p, &s)) return false;
		const DWORD now = GetTickCount();
		bool ok = false;
		Lock();
		{
			PoiseState & st = FindOrCreate(s.id, ComputeMax(s), ComputeMaxPosture(s), now);
			if(now >= st.exhaustReadyAt && now >= st.bleedUntil && now >= st.rageUntil)
			{
				st.exhaustReadyAt = now + (DWORD)(g_cfg.exhaustCooldown * 1000.0f);
				ok = true;
			}
		}
		Unlock();
		if(ok) { g_exhaustBlockUntil = GetTickCount() + (DWORD)(g_cfg.exhaustSeconds * 1000.0f); _MESSAGE("EXHAUSTION: player ran out of stamina by swinging"); }
		return ok;
	}

	void N_PowerStart(StaticFunctionTag *)
	{
		g_powerUntil = GetTickCount() + 1500;
	}

	// Usado pelo MCM: le e escreve o .ini (a DLL recarrega logo a seguir).
	float N_IniGetF(StaticFunctionTag *, BSFixedString section, BSFixedString key, float def)
	{
		if(!section.data || !key.data) return def;
		return IniF(section.data, key.data, def);
	}

	void N_IniSetF(StaticFunctionTag *, BSFixedString section, BSFixedString key, float value)
	{
		if(!section.data || !key.data) return;
		char buf[48];
		_snprintf_s(buf, sizeof(buf), _TRUNCATE, "%.3f", value);
		WritePrivateProfileStringA(section.data, key.data, buf, g_iniPath);
		LoadConfig();
	}

	float N_GetKnockbackForce(StaticFunctionTag *)
	{
		return g_cfg.kbForce;
	}

	// O Papyrus escreve no log do plugin (para descobrirmos o que se passa no jogo).
	void N_Log(StaticFunctionTag *, BSFixedString msg)
	{
		if(g_cfg.diag && msg.data) _MESSAGE("papyrus: %s", msg.data);
	}

	SInt32 N_GetLanguage(StaticFunctionTag *)
	{
		return g_cfg.language;
	}

	// Depois do ResetHealthAndLimbs (vida cheia): baixa a vida para HealthPercent.
	void N_LifeRecover(StaticFunctionTag *)
	{
		Actor * pl = (Actor *)Engine::Player();
		float cur = 0.0f, mx = 0.0f;
		if(pl && ReadAV(pl, 24, &cur, &mx))
		{
			float target = mx * g_cfg.livesHealthPct / 100.0f;
			if(target < 1.0f) target = 1.0f;
			InterlockedExchange(&g_rageCostBusy, 1);
			if(target != cur) RestoreAV(pl, 24, target - cur);
			InterlockedExchange(&g_rageCostBusy, 0);
		}
		g_invulnUntil = GetTickCount() + (DWORD)(g_cfg.livesInvuln * 1000.0f);
		SyncEssential();
		_MESSAGE("papyrus: extra life recovery - health set to %.0f%%, invulnerable %.1f s, essential=%d", g_cfg.livesHealthPct, g_cfg.livesInvuln, LivesAvailable(pl) > 0 ? 1 : 0);
	}
	SInt32 N_LifeMethod(StaticFunctionTag *)
	{
		return g_cfg.livesEssential ? 1 : 0;
	}
	void N_SyncEssential(StaticFunctionTag *)
	{
		SyncEssential();
	}
	void N_BossRecover(StaticFunctionTag *, Actor * b)
	{
		if(!b) return;
		float cur = 0.0f, mx = 0.0f;
		if(ReadAV(b, 24, &cur, &mx))
		{
			float target = mx * g_cfg.bossHealthPct / 100.0f;
			if(target < 1.0f) target = 1.0f;
			if(target > cur) RestoreAV(b, 24, target - cur);
		}
		_MESSAGE("papyrus: boss %08X health restored to %.0f%%", (UInt32)b->formID, g_cfg.bossHealthPct);
	}
	SInt32 N_RecentAttacker(StaticFunctionTag *, SInt32 idx)
	{
		if(idx < 0 || idx > 3) return 0;
		if(GetTickCount() - g_recentAtk[idx].tick > 20000) return 0;
		return (SInt32)g_recentAtk[idx].id;
	}
	BSFixedString N_GetSupremeAnim(StaticFunctionTag *)
	{
		return BSFixedString(g_cfg.supAnim);
	}
	void N_OwnAnim(StaticFunctionTag *, SInt32 ms)
	{
		g_ownAnimUntil = GetTickCount() + (DWORD)(ms > 0 ? ms : 0);
	}
	bool N_BleedAccepted(StaticFunctionTag *, SInt32 id)
	{
		bool r = false;
		const DWORD now = GetTickCount();
		for(int i = 0; i < g_bleedAccN; i++) if((SInt32)g_bleedAcc[i].id == id && g_bleedAcc[i].ok && now - g_bleedAcc[i].tick < 2500) r = true;
		return r;
	}
	void N_BleedDone(StaticFunctionTag *, SInt32 packed)
	{
		if(g_cfg.diag) _MESSAGE("papyrus: bleedout #%d finished (the victim should be standing again)", (int)((packed >> 3) & 0x1FFFFF));
	}

	bool RegisterPapyrus(VMClassRegistry * registry)
	{
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetBleedSeconds", "NahIDWinNative", N_GetBleedSeconds, registry));
		registry->RegisterFunction(new NativeFunction2 <StaticFunctionTag, float, SInt32, SInt32>("GetSwingCost", "NahIDWinNative", N_GetSwingCost, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetRegenPercent", "NahIDWinNative", N_GetRegenPercent, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetExhaustSeconds", "NahIDWinNative", N_GetExhaustSeconds, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, bool>("TryExhaust", "NahIDWinNative", N_TryExhaust, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("PowerStart", "NahIDWinNative", N_PowerStart, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, BSFixedString>("Log", "NahIDWinNative", N_Log, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, SInt32>("GetLanguage", "NahIDWinNative", N_GetLanguage, registry));
		registry->RegisterFunction(new NativeFunction3 <StaticFunctionTag, float, BSFixedString, BSFixedString, float>("IniGetF", "NahIDWinNative", N_IniGetF, registry));
		registry->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, BSFixedString, BSFixedString, float>("IniSetF", "NahIDWinNative", N_IniSetF, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetKnockbackForce", "NahIDWinNative", N_GetKnockbackForce, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, SInt32>("BleedDone", "NahIDWinNative", N_BleedDone, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("ScriptReady", "NahIDWinNative", N_ScriptReady, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, SInt32>("SetLivesUsed", "NahIDWinNative", N_SetLivesUsed, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("LifeRecover", "NahIDWinNative", N_LifeRecover, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, SInt32>("LifeMethod", "NahIDWinNative", N_LifeMethod, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("SyncEssential", "NahIDWinNative", N_SyncEssential, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, Actor *>("BossRecover", "NahIDWinNative", N_BossRecover, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, SInt32, SInt32>("RecentAttacker", "NahIDWinNative", N_RecentAttacker, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, BSFixedString>("GetSupremeAnim", "NahIDWinNative", N_GetSupremeAnim, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, SInt32>("OwnAnim", "NahIDWinNative", N_OwnAnim, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, bool, SInt32>("BleedAccepted", "NahIDWinNative", N_BleedAccepted, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, SInt32>("Ack", "NahIDWinNative", N_Ack, registry));
		_MESSAGE("papyrus functions registered (NahIDWinNative)");
		return true;
	}

	// ------------------------------------------------------------------ HUD proprio (NahIDWinHUD.swf)
	// O SWF (AS2) e carregado dentro do "HUD Menu" e chama skse.plugins.NahIDWinHUD.GetBars(obj) de poucos em poucos
	// quadros; aqui preenchemos obj com os numeros e o SWF desenha as barras. O alvo e o da mira (lock-on).
	bool BarForActor(Actor * a, BarInfo * out)
	{
		ActorStats s;
		if(!a || !ReadActorStats(a, &s) || !s.alive) return false;
		if((s.isPlayer && !g_cfg.applyPlayer) || (!s.isPlayer && !g_cfg.applyNPCs)) return false;
		const float mx = ComputeMax(s), mp = ComputeMaxPosture(s);
		const DWORD now = GetTickCount();
		Lock();
		{
			PoiseState & st = FindOrCreate(s.id, mx, mp, now);
			Regen(st, now);
			RegenPosture(st, now);
			float poiseFrac = (st.max > 0.0f) ? (st.cur / st.max) : 1.0f;
			if(poiseFrac < 0.0f) poiseFrac = 0.0f;
			if(poiseFrac > 1.0f) poiseFrac = 1.0f;
			out->poise = poiseFrac;
			out->posture = (st.pMax > 0.0f) ? (st.pCur / st.pMax) : 0.0f;
			if(out->posture > 1.0f) out->posture = 1.0f;
			if(out->posture < 0.0f) out->posture = 0.0f;
			out->warn = false;
			RageDisplay(st, now, !s.isPlayer, &out->rageState, &out->rageFrac);
		}
		Unlock();
		return true;
	}

	// Ator de um handle (so usado de imediato, na mesma chamada do SWF). Sem objetos com destrutor.
	Actor * ActorFromHandle(UInt32 h)
	{
		__try
		{
			if(h == 0 || h == 0x00100000 || h == 0xFFFFFFFF) return NULL;
			TESObjectREFR * ref = NULL;
			if(!LookupREFRByHandle(&h, &ref) || !ref) return NULL;
			Actor * res = NULL;
			if(ref->formType == kFormType_Character) res = (Actor *)ref;
			ref->handleRefObject.DecRefHandle();	// a procura incrementa a contagem
			return res;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return NULL;
		}
	}

	// Lock-on do mod Soft Target LE (se estiver instalado e com as funcoes exportadas).
	UInt32 SoftTargetId()
	{
		static bool searched = false;
		typedef UInt32 (__cdecl * GetFn)();
		static GetFn lockFn = NULL, assistFn = NULL;
		if(!searched)
		{
			searched = true;
			HMODULE m = GetModuleHandleA("SoftTargetLE.dll");
			if(m)
			{
				lockFn = (GetFn)GetProcAddress(m, "SoftTarget_GetLockedFormID");
				assistFn = (GetFn)GetProcAddress(m, "SoftTarget_GetAssistFormID");
			}
			_MESSAGE("hud target: Soft Target LE %s", (lockFn || assistFn) ? "found (lock-on read directly)" : "not found/without exports");
		}
		__try
		{
			UInt32 id = lockFn ? lockFn() : 0;
			if(!id && assistFn) id = assistFn();
			return id;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return 0; }
	}

	Actor * ActorFromFormId(UInt32 id)
	{
		__try
		{
			if(!id) return NULL;
			TESForm * f = LookupFormByID(id);
			if(f && f->formType == kFormType_Character) return (Actor *)f;
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { }
		return NULL;
	}

	// Hostil ao jogador segundo o proprio jogo (so estes ganham barra).
	bool IsHostileActor(Actor * a)
	{
		__try
		{
			PlayerCharacter * pc = Engine::Player();
			if(!a || !pc || a == (Actor *)pc) return false;
			if(a->flags1 & Actor::kFlags_IsPlayerTeammate) return false;
			return CALL_MEMBER_FN(a, IsHostileToActor)((Actor *)pc);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return false; }
	}

	// Distancia (px) do ator ao centro do ecra; <0 se invalido/fora do ecra.
	float ScreenDistFromCenter(UInt32 id)
	{
		__try
		{
			Actor * a = ActorFromFormId(id);
			if(!a || id == 0x14 || (a->flags1 & Actor::kFlags_IsPlayerTeammate)) return -1.0f;
			if(Engine::LifeState(a) != 0) return -1.0f;
			PlayerCharacter * pc = Engine::Player();
			if(!pc) return -1.0f;
			if(!IsHostileActor(a)) return -1.0f;
			{
				const float ddx = a->pos.x - pc->pos.x, ddy = a->pos.y - pc->pos.y, ddz = a->pos.z - pc->pos.z;
				const float md = (float)g_cfg.wTargetMaxDist;
				if(ddx * ddx + ddy * ddy + ddz * ddz > md * md) return -1.0f;
			}
			float x, y, z, sx, sy;
			if(!ReadHeadPos(a, &x, &y, &z) || !ProjectPoint(x, y, z, &sx, &sy)) return -1.0f;
			if(sx < 0.0f || sx > 1.0f || sy < 0.0f || sy > 1.0f) return -1.0f;
			const float dx = (sx - 0.5f) * (float)g_cfg.wScreenW, dy = (sy - 0.5f) * (float)g_cfg.wScreenH;
			return sqrtf(dx * dx + dy * dy);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { return -1.0f; }
	}

	// Inimigos que o jogo considera hostis ao jogador neste momento (lista do proprio jogador).
	int CollectEngineHostiles(UInt32 * ids, int n, int cap)
	{
		__try
		{
			PlayerCharacter * pc = Engine::Player();
			if(!pc) return n;
			for(UInt32 i = 0; i < pc->hostileHandles.count && n < cap; i++)
			{
				Actor * a = ActorFromHandle(pc->hostileHandles[i]);
				if(a) ids[n++] = a->formID;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER) { }
		return n;
	}

	UInt32		g_radId = 0;
	DWORD		g_radTick = 0, g_radSeen = 0;
	// Inimigo em combate mais perto do centro do ecra, dentro do raio. Calculado no maximo 10 vezes por segundo.
	UInt32 PickByRadius(DWORD now)
	{
		if(g_cfg.wTargetRadius <= 0) return 0;
		if(now - g_radTick < 100) return g_radId;
		g_radTick = now;
		UInt32 ids[256]; int n = 0;
		Lock();
		for(std::map<UInt32, DWORD>::iterator it = g_hostile.begin(); it != g_hostile.end() && n < 256; ++it) if(now - it->second < 60000) ids[n++] = it->first;
		Unlock();
		n = CollectEngineHostiles(ids, n, 256);
		UInt32 best = 0; float bestD = (float)g_cfg.wTargetRadius;
		for(int i = 0; i < n; i++)
		{
			const float d = ScreenDistFromCenter(ids[i]);
			if(d >= 0.0f && d <= bestD) { bestD = d; best = ids[i]; }
		}
		g_radId = best;
		return best;
	}

	UInt32		g_stickyId = 0;
	DWORD		g_stickySeen = 0;
	// Alvo atual: lock-on do Soft Target LE; lock-on do jogo; crosshair; inimigo perto do centro do ecra; e o ultimo alvo durante uns segundos.
	// how: 3 = Soft Target, 1 = lock-on do jogo, 2 = crosshair, 4 = raio no ecra, 5 = persistente
	Actor * CurrentTarget(int * how)
	{
		__try
		{
			*how = 0;
			PlayerCharacter * pc = Engine::Player();
			if(!pc) return NULL;
			const DWORD now = GetTickCount();
			Actor * t = ActorFromFormId(SoftTargetId());
			if(t && Engine::LifeState(t) == 0 && IsHostileActor(t)) *how = 3;
			else
			{
				t = ActorFromHandle(pc->targetHandle);
				if(t && !IsHostileActor(t)) t = NULL;
				if(t) *how = 1;
				else
				{
					CrosshairRefHandleHolder * ch = CrosshairRefHandleHolder::GetSingleton();
					if(ch) t = ActorFromHandle(ch->CrosshairRefHandle());
					if(t && !IsHostileActor(t)) t = NULL;
					if(t) *how = 2;
					else
					{
						t = ActorFromFormId(PickByRadius(now));
						if(t) *how = 4;
					}
				}
			}
			if(t && t->formID != 0x14)
			{
				g_stickyId = t->formID; g_stickySeen = now;
				return t;
			}
			if(g_stickyId && g_cfg.wTargetStickyMs > 0 && now - g_stickySeen < (DWORD)g_cfg.wTargetStickyMs)
			{
				t = ActorFromFormId(g_stickyId);
				if(t && Engine::LifeState(t) == 0) { *how = 5; return t; }
			}
			return NULL;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return NULL;
		}
	}

	DWORD g_wActiveUntil = 0;

	void PutNum(GFxValue * o, const char * name, double v)
	{
		GFxValue x; x.SetNumber(v); o->SetMember(name, &x);
	}
	void PutBool(GFxValue * o, const char * name, bool v)
	{
		GFxValue x; x.SetBool(v); o->SetMember(name, &x);
	}

	void FillHudData(GFxValue * o)
	{
		const DWORD now = GetTickCount();
		const bool ready = g_cfg.wEnabled && g_dataLoaded && Engine::Player() && !AnyMenuOpen();
		PutBool(o, "ready", ready);
		if(!ready) return;
		TickSupreme(now);
		TickPerilous(now);
		TickFleeWatch(now);
		if(g_execTick && now - g_execTick < 15000)
		{
			// vigia da vida do jogador depois de uma execucao (para descobrir o que o mata)
			static DWORD lastW = 0; static float lastHp = -1.0f;
			if(now - lastW >= 200)
			{
				lastW = now;
				float c = 0.0f, m = 0.0f;
				if(ReadAV((Actor *)Engine::Player(), 24, &c, &m) && (lastHp < 0.0f || c < lastHp - 2.0f || c <= 0.5f))
					_MESSAGE("EXEC-WATCH: %d ms after execution: player hp %.1f/%.1f (before %.1f) essential=%d lives=%d", (int)(now - g_execTick), c, m, lastHp, (int)PlayerEssentialFlag(), (int)LivesAvailable((Actor *)Engine::Player()));
				if(c > 0.0f || lastHp < 0.0f) lastHp = c;
			}
		}
		SyncEssential();

		BarInfo pi = { 1.0f, 0, 1.0f, false, 0.0f };
		bool pOn = false;
		if(g_cfg.wPlayer && BarForActor((Actor *)Engine::Player(), &pi))
		{
			if(pi.posture > 0.005f || pi.rageState != 0) g_wActiveUntil = now + 4000;
			pOn = g_cfg.wPlayerAlways || now < g_wActiveUntil;
		}
		PutBool(o, "pOn", pOn);
		{
			Actor * pa = (Actor *)Engine::Player();
			PutNum(o, "lifeUnl", LivesUnlocked(pa));
			PutNum(o, "lifeAvail", LivesAvailable(pa));
			PutBool(o, "supOn", g_cfg.supEnabled && g_cfg.rageEnabled);
			PutNum(o, "supState", now < g_supUntil ? 2 : (now < g_supReadyAt ? 1 : 0));
			PutBool(o, "iconsOn", g_cfg.iconsAlways || pOn);
		}
		PutNum(o, "sw", g_cfg.wScreenW); PutNum(o, "sh", g_cfg.wScreenH); PutNum(o, "style", g_cfg.wStyle);
		PutNum(o, "pAH", g_cfg.wPosH); PutNum(o, "pAV", g_cfg.wPosV); PutNum(o, "pSX", g_cfg.wShiftX); PutNum(o, "pSY", g_cfg.wShiftY);
		PutNum(o, "tAH", g_cfg.wTPosH); PutNum(o, "tAV", g_cfg.wTPosV); PutNum(o, "tSX", g_cfg.wTShiftX); PutNum(o, "tSY", g_cfg.wTShiftY);
		PutNum(o, "pW", g_cfg.wPW); PutNum(o, "pH", g_cfg.wPH);
		PutNum(o, "pPosture", pi.posture); PutNum(o, "pRageState", pi.rageState); PutNum(o, "pRageFrac", pi.rageFrac);

		BarInfo ti = { 1.0f, 0, 1.0f, false, 0.0f };
		bool tOn = false;
		if(g_cfg.wTarget)
		{
			int how = 0;
			Actor * t = CurrentTarget(&how);
			if(t && (UInt32)t->formID != 0x14) tOn = BarForActor(t, &ti);
			static UInt32 lastId = 0xFFFFFFFF; static int logged = 0;
			const UInt32 nowId = (t && tOn) ? (UInt32)t->formID : 0;
			if(nowId != lastId)
			{
				lastId = nowId;
				if(logged < 40) { logged++; _MESSAGE("hud widget: target %08X (%s)", nowId, nowId ? (how == 3 ? "soft target" : (how == 1 ? "lock-on" : (how == 2 ? "crosshair" : (how == 4 ? "screen radius" : "sticky")))) : "none"); }
			}
		}
		PutBool(o, "tOn", tOn);
		{
			Actor * ta = NULL; int hw2 = 0;
			if(tOn) ta = CurrentTarget(&hw2);
			int bu = 0, bl = 0;
			if(ta && IsBossActor(ta)) { bu = 1; bl = 1; Lock(); StateMap::iterator ib = g_states.find(ta->formID); if(ib != g_states.end() && ib->second.bossUsed) bl = 0; Unlock(); }
			PutNum(o, "tLifeUnl", bu); PutNum(o, "tLifeAvail", bl);
		}
		PutNum(o, "tPosture", ti.posture); PutNum(o, "tRageState", ti.rageState); PutNum(o, "tRageFrac", ti.rageFrac);
		PutNum(o, "tAnchor", g_cfg.wAnchor ? 1 : 0);
		PutNum(o, "tOffX", g_cfg.wTOffX); PutNum(o, "tOffY", g_cfg.wTOffY);
		PutNum(o, "tW", g_cfg.wTW); PutNum(o, "tH", g_cfg.wTH);
		
		PutNum(o, "iconMode", g_cfg.wIcon);
	}

	class GetBarsHandler : public GFxFunctionHandler
	{
	public:
		virtual void Invoke(Args * args)
		{
			if(!args || args->numArgs < 1 || !args->args || !args->args[0].IsObject()) return;
			FillHudData(&args->args[0]);
		}
	};

	bool RegisterHudScaleform(GFxMovieView * view, GFxValue * root)
	{
		RegisterFunction<GetBarsHandler>(root, view, "GetBars");
		return true;
	}

	// Funcao interna com objetos (GFxValue tem destrutor): sem __try aqui.
	bool LoadHudInner(GFxMovieView * v)
	{
		GFxValue c;
		if(v->GetVariable(&c, "_root.NahIDWinContainer") && c.GetType() != GFxValue::kType_Undefined) return true;	// ja carregado
		v->Invoke("_root.createEmptyMovieClip", NULL, "%s%d", "NahIDWinContainer", 9400);
		v->Invoke("_root.NahIDWinContainer.loadMovie", NULL, "%s", "NahIDWinHUD.swf");
		return true;
	}

	bool InvokeLoadHud(GFxMovieView * v)
	{
		__try
		{
			return LoadHudInner(v);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	class HudMenuSink : public BSTEventSink<MenuOpenCloseEvent>
	{
	public:
		virtual EventResult ReceiveEvent(MenuOpenCloseEvent * evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
		{
			if(evn && evn->opening && g_cfg.wEnabled && evn->menuName.data && strcmp(evn->menuName.data, "HUD Menu") == 0)
			{
				GFxMovieView * v = GetHudView();
				if(v)
				{
					const bool ok = InvokeLoadHud(v);
					_MESSAGE("hud widget: NahIDWinHUD.swf load requested (%s)", ok ? "ok" : "FAILED");
				}
			}
			return kEvent_Continue;
		}
	};
	HudMenuSink g_hudMenuSink;

	void InstallHudSink()
	{
		MenuManager * mm = MenuManager::GetSingleton();
		if(!mm) { _MESSAGE("hud widget: MenuManager not available"); return; }
		mm->MenuOpenCloseEventDispatcher()->AddEventSink(&g_hudMenuSink);
		_MESSAGE("hud widget: HUD Menu listener registered");
	}

	// ------------------------------------------------------------------ mensagens do SKSE
	void OnMessage(SKSEMessagingInterface::Message * message)
	{
		if(!message) return;
		switch(message->type)
		{
			case SKSEMessagingInterface::kMessage_PostPostLoad:
				TryHud();
				break;

			case SKSEMessagingInterface::kMessage_DataLoaded:
				g_hitEventDispatcher->AddEventSink(&g_hitSink);
				_MESSAGE("hit event receiver registered");
				if(g_inputEventDispatcher && *g_inputEventDispatcher)
				{
					(*g_inputEventDispatcher)->AddEventSink(&g_inputSink);
					_MESSAGE("input receiver registered (parry on the block button, rage on key %d)", g_cfg.rageKey);
				}
				else _MESSAGE("WARNING: the input dispatcher is not available - no parry and no rage key");
				g_dataLoaded = true;
				InstallHudSink();
				InstallHealthHook();
				ApplyHealthGMST();
				TryHud();
				break;

			case SKSEMessagingInterface::kMessage_NewGame:
			case SKSEMessagingInterface::kMessage_PostLoadGame:
				ClearStates(message->type == SKSEMessagingInterface::kMessage_NewGame ? "new game" : "load game");
				if(Engine::Player()) EnsureNotifyHook((TESObjectREFR *)Engine::Player());
				ApplyHealthGMST();
				DumpVtable();
				SendLoadEvent();
				break;

			default:
				break;
		}
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, PLUGIN_LOG_FILE);
	_MESSAGE("NahIDWin " PLUGIN_VERSION_STR " (built " __DATE__ " " __TIME__ ")");

	info->infoVersion	= PluginInfo::kInfoVersion;
	info->name			= PLUGIN_NAME;
	info->version		= 1;

	g_pluginHandle = skse->GetPluginHandle();

	if(skse->isEditor) { _MESSAGE("ABORTING: loaded in the editor"); return false; }

	_MESSAGE("game runtime %08X (1.9.32.0 is %08X)", skse->runtimeVersion, RUNTIME_VERSION_1_9_32_0);
	if(skse->runtimeVersion != RUNTIME_VERSION_1_9_32_0)
	{
		_MESSAGE("ABORTING: unsupported runtime");
		return false;
	}
	return true;
}

bool SKSEPlugin_Load(const SKSEInterface * skse)
{
	ResolveIniPath();
	LoadConfig();

	g_messaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
	if(!g_messaging || !g_messaging->RegisterListener(g_pluginHandle, "SKSE", OnMessage))
	{
		_MESSAGE("ABORTING: the messaging interface is not available");
		return false;
	}

	g_papyrus = (SKSEPapyrusInterface *)skse->QueryInterface(kInterface_Papyrus);
	if(g_papyrus && g_papyrus->Register(RegisterPapyrus))
		_MESSAGE("papyrus interface: registration queued");
	else
		_MESSAGE("papyrus interface not available - stagger mode 0 will not be confirmed by the script");

	SKSEScaleformInterface * scaleform = (SKSEScaleformInterface *)skse->QueryInterface(kInterface_Scaleform);
	if(scaleform && scaleform->Register("NahIDWinHUD", RegisterHudScaleform))
		_MESSAGE("scaleform interface: NahIDWinHUD registered");
	else
		_MESSAGE("scaleform interface not available - own HUD disabled");

	_MESSAGE("running");
	return true;
}

};
