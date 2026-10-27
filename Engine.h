// Engine.h - os poucos enderecos e deslocamentos da LE 1.9.32.0 que o PoiseLE usa.
// Todos foram CONFIRMADOS no projeto FreeClimb LE / SoftTarget LE (GPL-3.0), que ja funciona nesse executavel.
// Nao acrescentar endereco novo sem confirmar no jogo.
#pragma once

#include "skse/GameForms.h"
#include "skse/GameReferences.h"

namespace Engine
{
	inline PlayerCharacter *	Player()		{ return *(PlayerCharacter **)0x01310588; }	// g_thePlayer

	// Actor: primeira palavra de flags em 0x68; estado de vida nos bits 21..24 (0 = vivo).
	inline UInt32 &	ActorFlags1(Actor * actor)	{ return *(UInt32 *)((UInt8 *)actor + 0x68); }
	inline UInt32	LifeState(Actor * actor)	{ return (ActorFlags1(actor) >> 21) & 0x0F; }
}
