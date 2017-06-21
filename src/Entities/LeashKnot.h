
#pragma once

#include "HangingEntity.h"





// tolua_begin
class cLeashKnot :
	public cHangingEntity
{
	typedef cHangingEntity super;

public:

	// tolua_end

	CLASS_PROTODEF(cLeashKnot)

	cLeashKnot(eBlockFace a_BlockFace, double a_X, double a_Y, double a_Z);

private:

	virtual void OnRightClicked(cPlayer & a_Player) override;
	virtual void KilledBy(TakeDamageInfo & a_TDI) override;
	virtual void GetDrops(cItems & a_Items, cEntity * a_Killer) override;
	virtual void SpawnOn(cClientHandle & a_ClientHandle) override;
	virtual void Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk) override;

	/** When a fence is destroyed, the knot on it gets destroyed after a while. This flag turns on the countdown to self destroy. */
	bool m_SelfDestroy;
	int m_TicksToSelfDestroy;

};  // tolua_export




