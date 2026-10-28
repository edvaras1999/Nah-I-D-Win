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
// Barra visivel: usa a API do TrueHUD LE (barras especiais), se o TrueHUD estiver instalado.
//
// Licenca GPL-3.0. A base de compilacao e os enderecos vem do FreeClimb LE / SoftTarget LE (GPL-3.0).

#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameRTTI.h"
#include "skse/GameForms.h"
#include "skse/GameObjects.h"
#include "skse/GameReferences.h"
#include "skse/GameEvents.h"
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
#include <functional>

#define PLUGIN_NAME			"NahIDWin"
#define PLUGIN_VERSION_STR	"0.4.0 (Poise + Postura + BleedOut + parry + furia)"
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

		// empurrao no stagger pesado (so em condicoes especificas)
		int		kbEnabled;
		float	kbForce;				// 0 = desligado
		int		kbPlayer;				// tambem empurrar o jogador
		int		kbOnPower;				// golpe forte (inclui o de corrida e o carregado)
		int		kbOnGiant;				// atacante de raca grande (massa >= kbGiantMass)
		float	kbGiantMass;
		int		kbOnHeavyWeapon;		// arma de duas maos ou pesada
		float	kbHeavyWeight;

		// avisos e golpes proprios
		// parry proprio (janela depois de levantar o bloqueio)
		int		parryEnabled;
		int		parryWindowMs;
		float	parryAttackerPosture;	// fracao da postura maxima do atacante que enche
		int		parryAttackerStagger;	// 1 = o atacante leva stagger medio
		float	parryDefPostureRecover;	// fracao da postura maxima que o defensor recupera
		float	parryDefPoiseRecover;	// fracao do poise maximo que o defensor recupera
		int		parryFx;				// faiscas e som (usa os do Inpa Sekiro se instalado)

		// furia: imunidade temporaria em troca de vida ou estamina
		int		rageEnabled;
		int		rageKey;				// tecla (codigo DirectInput) do jogador. 34 = G
		float	rageSeconds;
		float	rageCooldown;
		int		rageCostType;			// 0 = vida, 1 = estamina
		float	rageCostPercent;		// percentagem do maximo
		int		rageNpc;				// NPCs ativam sozinhos
		float	rageNpcPosture;			// ativa quando a postura passa desta fracao
		float	rageNpcPoise;			// ... ou o poise desce abaixo desta fracao
		int		rageNpcChance;			// probabilidade (%) por golpe recebido quando as condicoes se cumprem
		float	rageNpcCooldown;
		float	rageNpcCostPercent;		// 0 = gratis para NPCs

		float	poiseWarnFraction;		// a barra pisca quando o poise desce abaixo disto (0 = desligado)
		int		ignoreOwnSpells;		// 1 = nao contar os feiticos de stagger deste mod como dano
		// TrueHUD
		int		hudEnabled;
		int		hudPlayer;			// mostrar a barra tambem para o jogador
		int		hudFlash;			// piscar a barra quando algo quebra
		int		hudBar;				// 0 = barra do poise, 1 = barra da postura, 2 = a mais perto de quebrar

		// diagnostico
		int		diag;				// 1 = diagnostico ligado
		int		logHitsMax;			// quantos golpes escrever no log por sessao (0 = nenhum)
		int		summarySec;			// de quantos em quantos segundos escrever o resumo (0 = nunca)
		int		dumpVtable;			// 1 = despejar a tabela de funcoes de animacao do jogador
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
		c.swingCostPlayer	= IniF("Stamina", "SwingCostPlayer", 8.0f);
		c.hitCostNpc		= IniF("Stamina", "HitCostNPC", 0.0f);

		c.kbEnabled			= IniI("Knockback", "Enabled", 1);
		c.kbForce			= IniF("Knockback", "Force", 1.0f);
		c.kbPlayer			= IniI("Knockback", "Player", 1);
		c.kbOnPower			= IniI("Knockback", "OnPowerAttack", 1);
		c.kbOnGiant			= IniI("Knockback", "OnGiantCreature", 1);
		c.kbGiantMass		= IniF("Knockback", "GiantMassMin", 3.0f);
		c.kbOnHeavyWeapon	= IniI("Knockback", "OnHeavyWeapon", 0);
		c.kbHeavyWeight		= IniF("Knockback", "HeavyWeaponMinWeight", 15.0f);

		c.poiseWarnFraction	= IniF("TrueHUD", "PoiseWarnFraction", 0.3f);

		c.parryEnabled		= IniI("Parry", "Enabled", 1);
		c.parryWindowMs		= IniI("Parry", "WindowMs", 250);
		c.parryAttackerPosture = IniF("Parry", "AttackerPostureFraction", 0.25f);
		c.parryAttackerStagger = IniI("Parry", "AttackerStagger", 1);
		c.parryDefPostureRecover = IniF("Parry", "DefenderPostureRecoverFraction", 0.15f);
		c.parryDefPoiseRecover = IniF("Parry", "DefenderPoiseRecoverFraction", 0.25f);
		c.parryFx			= IniI("Parry", "Effects", 1);

		c.rageEnabled		= IniI("Rage", "Enabled", 1);
		c.rageKey			= IniI("Rage", "Key", 34);
		c.rageSeconds		= IniF("Rage", "DurationSeconds", 5.0f);
		c.rageCooldown		= IniF("Rage", "CooldownSeconds", 30.0f);
		c.rageCostType		= IniI("Rage", "CostType", 0);
		c.rageCostPercent	= IniF("Rage", "CostPercent", 15.0f);
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
		c.hudBar		= IniI("TrueHUD", "Bar", 2);

		c.diag			= IniI("Diagnostics", "Enabled", 1);
		c.logHitsMax	= IniI("Diagnostics", "LogHitsMax", 400);
		c.summarySec	= IniI("Diagnostics", "SummarySeconds", 30);
		c.dumpVtable	= IniI("Diagnostics", "DumpVtable", 1);

		WIN32_FILE_ATTRIBUTE_DATA fad;
		if(GetFileAttributesExA(g_iniPath, GetFileExInfoStandard, &fad)) g_iniTime = fad.ftLastWriteTime;

		_MESSAGE("config: enabled=%d player=%d npcs=%d staggerMode=%d (slot %d, '%s') posture=%d (difficulty %.2f) hud=%d bar=%d diag=%d  [%s]",
			c.enabled, c.applyPlayer, c.applyNPCs, c.staggerMode, c.nativeSlot, c.nativeEvent, c.postureEnabled, c.postureDifficulty, c.hudEnabled, c.hudBar, c.diag, g_iniPath);
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
	};
	typedef std::map<UInt32, PoiseState> StateMap;

	struct LockHolder { LockHolder() { InitializeCriticalSection(&cs); } CRITICAL_SECTION cs; };
	LockHolder	g_lockHolder;
	StateMap	g_states;

	// contadores e estatisticas (diagnostico)
	volatile DWORD	g_blockStart = 0;		// quando o jogador levantou o bloqueio (para o parry)
	volatile LONG	g_parries = 0, g_rages = 0;
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
			s.pCur = 0.0f; s.pMax = maxPosture; s.pLastHit = 0; s.pTick = now; s.bleedUntil = 0; s.bleedImmuneUntil = 0; s.lightUntil = 0; s.rageUntil = 0; s.rageReadyAt = 0;
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
	};

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
		return (m < 5.0f) ? 5.0f : m;
	}

	// Postura maxima: base + vida maxima (cresce com o nivel) + armadura, vezes a dificuldade.
	float ComputeMaxPosture(const ActorStats & s)
	{
		float m = g_cfg.postureBase + s.hpMax * g_cfg.postureHealthFrac + s.armor * g_cfg.postureArmorFrac;
		m *= s.isPlayer ? g_cfg.postureScalePlayer : g_cfg.postureScaleNpc;
		m *= (g_cfg.postureDifficulty > 0.05f) ? g_cfg.postureDifficulty : 0.05f;	// dificuldade maior = postura maior
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

	bool CallNotify(void * holder, void * fn, const BSFixedString * name, bool * result)
	{
		__try
		{
			typedef bool (__thiscall * NotifyFn)(void *, const BSFixedString *);
			*result = ((NotifyFn)fn)(holder, name);
			return true;
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
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

	void AddAV(Actor * a, UInt32 av, float delta)
	{
		__try
		{
			const float cur = a->actorValueOwner.GetCurrent(av);
			a->actorValueOwner.SetCurrent(av, cur + delta);
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

	// Poise: a barra mostra o que resta. Postura: a barra mostra o que ja encheu (cheia = BleedOut).
	float HudGetCur(Actor * a)
	{
		ActorStats s;
		if(!a || !ReadActorStats(a, &s)) return 0.0f;
		const float mx = ComputeMax(s), mp = ComputeMaxPosture(s);
		if((s.isPlayer && !g_cfg.applyPlayer) || (!s.isPlayer && !g_cfg.applyNPCs)) return (g_cfg.hudBar == 0) ? mx : 0.0f;
		const DWORD now = GetTickCount();
		Lock();
		PoiseState & st = FindOrCreate(s.id, mx, mp, now);
		Regen(st, now);
		RegenPosture(st, now);
		float cur;
		if(g_cfg.hudBar == 2)
		{
			// "risco": o que estiver mais perto de quebrar (0 = tranquilo, 100 = vai quebrar)
			float rp = (st.max > 0.0f) ? (1.0f - st.cur / st.max) : 0.0f;
			float rs = (st.pMax > 0.0f) ? (st.pCur / st.pMax) : 0.0f;
			cur = 100.0f * (rp > rs ? rp : rs);
		}
		else cur = (g_cfg.hudBar == 1) ? st.pCur : st.cur;
		Unlock();
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
		_MESSAGE("SUMMARY: hits=%d applied=%d poiseBreaks=%d postureBreaks=%d light=%d medium=%d parries=%d rages=%d | events sent=%d acked=%d pending=%d | script started=%d | latency avg=%d ms max=%d ms (n=%d) | tracked=%d faults=%d | truehud=%s",
			(int)g_hits, (int)g_applied, (int)g_breaks, (int)g_postureBreaks, (int)g_lightSent, (int)g_mediumSent, (int)g_parries, (int)g_rages, (int)g_sent, (int)g_acks, (int)(g_sent - g_acks), (int)g_scriptReady,
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
	int ParryAttacker(const HitInfo & h, DWORD now)
	{
		if(!h.attackerIsActor || !h.attackerActor || h.attacker.isPlayer || !h.attacker.alive) return 0;
		const float mxA = ComputeMax(h.attacker);
		const float mpA = ComputeMaxPosture(h.attacker);
		int kind = g_cfg.parryAttackerStagger ? 2 : 0;
		Lock();
		{
			PoiseState & st = FindOrCreate(h.attacker.id, mxA, mpA, now);
			Regen(st, now);
			RegenPosture(st, now);
			const bool frozen = (now < st.bleedUntil) || (now < st.bleedImmuneUntil) || (now < st.rageUntil);
			if(frozen) kind = 0;
			else if(g_cfg.postureEnabled)
			{
				st.pCur += mpA * g_cfg.parryAttackerPosture;
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
		if(!h.victim.alive) return;
		if(h.victim.isPlayer ? !g_cfg.applyPlayer : !g_cfg.applyNPCs) return;
		InterlockedIncrement(&g_hits);

		const float mx = ComputeMax(h.victim);
		const float mp = ComputeMaxPosture(h.victim);
		const float dmg = PoiseDamage(h);	// dano "cheio" (sem bloqueio)
		const bool blocked = (h.flags & TESHitEvent::kFlag_Blocked) != 0;
		const bool heavyHit = (h.flags & (TESHitEvent::kFlag_PowerAttack | TESHitEvent::kFlag_Bash)) != 0;

		// parry: bloqueio nos primeiros instantes depois de levantar a guarda (so o jogador)
		bool parry = false;
		if(g_cfg.parryEnabled && blocked && h.victim.isPlayer)
		{
			const DWORD bs = g_blockStart;
			if(bs && (now - bs) <= (DWORD)g_cfg.parryWindowMs) { parry = true; g_blockStart = 0; }
		}

		const float poiseDmg = parry ? 0.0f : (blocked ? dmg * g_cfg.blockedMult : dmg);
		float lowHp = 1.0f;
		if(h.victim.hpMax > 0.0f)
		{
			float frac = h.victim.hp / h.victim.hpMax;
			if(frac < 0.0f) frac = 0.0f;
			if(frac > 1.0f) frac = 1.0f;
			lowHp = 1.0f + g_cfg.postureLowHealthBonus * (1.0f - frac);
		}
		const float postureDmg = parry ? 0.0f : dmg * g_cfg.postureDamageMult * lowHp * (blocked ? g_cfg.postureBlockedMult : 1.0f);

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

			// furia automatica dos NPCs
			if(!h.victim.isPlayer && g_cfg.rageEnabled && g_cfg.rageNpc && !bleeding && !poiseWasImmune && !raging && now >= st.rageReadyAt)
			{
				const float pf = (st.max > 0.0f) ? (st.cur / st.max) : 1.0f;
				const float sf = (st.pMax > 0.0f) ? (st.pCur / st.pMax) : 0.0f;
				if((sf >= g_cfg.rageNpcPosture || pf <= g_cfg.rageNpcPoise) && (rand() % 100) < g_cfg.rageNpcChance)
				{
					st.rageUntil = now + (DWORD)(g_cfg.rageSeconds * 1000.0f);
					st.rageReadyAt = now + (DWORD)(g_cfg.rageNpcCooldown * 1000.0f);
					raging = true;
					rageStarted = true;
				}
			}

			const bool frozen = bleeding || poiseWasImmune || raging;
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
					if(st.pCur >= st.pMax)
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
				else if(poiseBroke) kind = 3;
				else if(!blocked && g_cfg.lightEnabled && (h.victim.isPlayer ? g_cfg.lightPlayer : g_cfg.lightNpc))
				{
					if(heavyHit && g_cfg.mediumOnPower) kind = 2;
					else if(now >= st.lightUntil) kind = 1;
					if(kind) st.lightUntil = now + (DWORD)(h.victim.isPlayer ? g_cfg.lightCooldownPlayerMs : g_cfg.lightCooldownMs);
				}
			}
			after = st.cur;
			pAfter = st.pCur;
		}
		Unlock();

		// empurrao: so no stagger pesado e so em condicoes especificas
		if(kind == 3 && g_cfg.kbEnabled && g_cfg.kbForce > 0.0f && (!h.victim.isPlayer || g_cfg.kbPlayer))
		{
			bool heavy = false;
			if(g_cfg.kbOnPower && heavyHit) heavy = true;
			if(g_cfg.kbOnGiant && h.attackerIsActor && h.attacker.mass >= g_cfg.kbGiantMass) heavy = true;
			if(g_cfg.kbOnHeavyWeapon && h.sourceKind == 1 && (h.weaponType == 5 || h.weaponType == 6 || h.weaponWeight >= g_cfg.kbHeavyWeight)) heavy = true;
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

		if(rageStarted)
		{
			InterlockedIncrement(&g_rages);
			_MESSAGE("RAGE (NPC): %08X is immune for %.1f s", h.victim.id, g_cfg.rageSeconds);
			if(g_cfg.rageNpcCostPercent > 0.0f && h.victimRef)
			{
				float cur = 0.0f, m = 0.0f;
				const UInt32 av = g_cfg.rageCostType == 0 ? 24 : 26;
				Actor * va = (Actor *)h.victimRef;
				if(ReadAV(va, av, &cur, &m))
				{
					float cost = m * g_cfg.rageNpcCostPercent / 100.0f;
					if(av == 24 && cost > cur - 1.0f) cost = cur - 1.0f;
					if(cost > 0.0f) AddAV(va, av, -cost);
				}
			}
			HudFlash(h.victimRef);
		}

		if(parry)
		{
			InterlockedIncrement(&g_parries);
			_MESSAGE("PARRY: %08X parried %08X", h.victim.id, h.attackerId);
			if(g_cfg.parryFx) SendStaggerEvent(h.victimRef, h.attackerId, 6);
			const int ak = ParryAttacker(h, now);
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

		if(kind)
		{
			if(kind >= 3) _MESSAGE("%s: victim=%08X attacker=%08X (mode %d)", kind == 4 ? "POSTURE BREAK -> BLEEDOUT" : (kind == 5 ? "POISE BREAK -> heavy stagger + KNOCKBACK" : "POISE BREAK -> heavy stagger"), h.victim.id, h.attackerId, g_cfg.staggerMode);
			if(g_cfg.staggerMode == 1 && kind != 4 && kind != 5) NativeStagger(h.victimRef);
			else SendStaggerEvent(h.victimRef, h.attackerId, kind);
			if(kind >= 3) HudFlash(h.victimRef);
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

	// ------------------------------------------------------------------ funcoes nativas para o Papyrus
	void N_ScriptReady(StaticFunctionTag *)
	{
		InterlockedIncrement(&g_scriptReady);
		_MESSAGE("papyrus: NahIDWinBridge script started and registered for NahIDWin_Stagger (#%d)", (int)g_scriptReady);
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

	float N_GetSwingCost(StaticFunctionTag *)
	{
		return g_cfg.stamEnabled ? g_cfg.swingCostPlayer : 0.0f;
	}

	float N_GetKnockbackForce(StaticFunctionTag *)
	{
		return g_cfg.kbForce;
	}

	SInt32 N_GetRageKey(StaticFunctionTag *)
	{
		return g_cfg.rageEnabled ? g_cfg.rageKey : 0;
	}

	SInt32 N_GetRageCostType(StaticFunctionTag *)
	{
		return g_cfg.rageCostType;
	}

	void N_BlockStart(StaticFunctionTag *)
	{
		g_blockStart = GetTickCount();
	}

	// O jogador carregou na tecla da furia. Devolve -1 se nao pode (recarga, BleedOut...) ou o custo a pagar (>= 0).
	float N_TryRage(StaticFunctionTag *)
	{
		if(!g_cfg.enabled || !g_cfg.rageEnabled) return -1.0f;
		PlayerCharacter * p = Engine::Player();
		if(!p) return -1.0f;
		ActorStats s;
		if(!ReadActorStats((Actor *)p, &s)) return -1.0f;
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
			if(g_cfg.diag) _MESSAGE("RAGE (player): refused (cooldown %.1f s left or in bleedout)", left / 1000.0f);
			return -1.0f;
		}
		InterlockedIncrement(&g_rages);
		float cost = 0.0f, cur = 0.0f, m = 0.0f;
		const UInt32 av = g_cfg.rageCostType == 0 ? 24 : 26;
		if(ReadAV((Actor *)p, av, &cur, &m))
		{
			cost = m * g_cfg.rageCostPercent / 100.0f;
			if(av == 24 && cost > cur - 1.0f) cost = cur - 1.0f;	// nunca mata o jogador
			if(cost < 0.0f) cost = 0.0f;
		}
		_MESSAGE("RAGE (player): immune for %.1f s, cost %.1f %s", g_cfg.rageSeconds, cost, g_cfg.rageCostType == 0 ? "health" : "stamina");
		HudFlash((TESObjectREFR *)p);
		return cost;
	}

	void N_BleedDone(StaticFunctionTag *, SInt32 packed)
	{
		if(g_cfg.diag) _MESSAGE("papyrus: bleedout #%d finished (the victim should be standing again)", (int)((packed >> 3) & 0x1FFFFF));
	}

	bool RegisterPapyrus(VMClassRegistry * registry)
	{
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetBleedSeconds", "NahIDWinNative", N_GetBleedSeconds, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, SInt32>("GetRageKey", "NahIDWinNative", N_GetRageKey, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, SInt32>("GetRageCostType", "NahIDWinNative", N_GetRageCostType, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("BlockStart", "NahIDWinNative", N_BlockStart, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("TryRage", "NahIDWinNative", N_TryRage, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetSwingCost", "NahIDWinNative", N_GetSwingCost, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, float>("GetKnockbackForce", "NahIDWinNative", N_GetKnockbackForce, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, SInt32>("BleedDone", "NahIDWinNative", N_BleedDone, registry));
		registry->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("ScriptReady", "NahIDWinNative", N_ScriptReady, registry));
		registry->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, SInt32>("Ack", "NahIDWinNative", N_Ack, registry));
		_MESSAGE("papyrus functions registered (NahIDWinNative)");
		return true;
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
				TryHud();
				break;

			case SKSEMessagingInterface::kMessage_NewGame:
			case SKSEMessagingInterface::kMessage_PostLoadGame:
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

	_MESSAGE("running");
	return true;
}

};
