
#pragma once

#include "BlockHandler.h"
#include "MetaRotator.h"
#include "../EffectID.h"
#include "Entities/LeashKnot.h"



class cBlockFenceHandler :
	public cMetaRotator<cBlockHandler, 0x03, 0x02, 0x03, 0x00, 0x01, true>
{
public:
	cBlockFenceHandler(BLOCKTYPE a_BlockType) :
		cMetaRotator<cBlockHandler, 0x03, 0x02, 0x03, 0x00, 0x01, true>(a_BlockType)
	{
	}

	virtual bool OnUse(cChunkInterface & a_ChunkInterface, cWorldInterface & a_WorldInterface, cPlayer * a_Player, int a_BlockX, int a_BlockY, int a_BlockZ, eBlockFace a_BlockFace, int a_CursorX, int a_CursorY, int a_CursorZ) override
	{
		// TODO: check if player has any mobs leashed and tight them to a knot on the fence
		
		// Create the leash knot
		cLeashKnot * LeashKnot = new cLeashKnot(a_BlockFace, a_BlockX, a_BlockY, a_BlockZ);
		if (!LeashKnot->Initialize(* a_Player->GetWorld()))
		{
			delete LeashKnot;
			LeashKnot = nullptr;
			return false;
		}
		
		a_Player->GetWorld()->BroadcastSoundEffect("entity.leashknot.place", a_Player->GetPosX(), a_Player->GetPosY(), a_Player->GetPosZ(), 1, 1);
		return true;
	}

	virtual void OnCancelRightClick(cChunkInterface & a_ChunkInterface, cWorldInterface & a_WorldInterface, cPlayer * a_Player, int a_BlockX, int a_BlockY, int a_BlockZ, eBlockFace a_BlockFace) override
	{
		a_WorldInterface.SendBlockTo(a_BlockX, a_BlockY, a_BlockZ, a_Player);
	}

	virtual bool IsUseable(void) override
	{
		return true;
	}

};




