
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "PassiveMonster.h"
#include "../World.h"
#include "../Entities/Player.h"
#include "../ClientHandle.h"
#include "BoundingBox.h"

// Ticks to wait in Tick function to optimize calculations
#define TICK_STEP 10


cPassiveMonster::cPassiveMonster(const AString & a_ConfigName, eMonsterType a_MobType, const AString & a_SoundHurt, const AString & a_SoundDeath, double a_Width, double a_Height) :
	super(a_ConfigName, a_MobType, a_SoundHurt, a_SoundDeath, a_Width, a_Height),
	m_LovePartner(nullptr),
	m_LoveTimer(0),
	m_LoveCooldown(0),
	m_MatingTimer(0),
	m_LeashedTo(nullptr),
	m_LeashToPos(nullptr),
	m_LeadActionJustDone(false)
{
	m_EMPersonality = PASSIVE;
}





bool cPassiveMonster::DoTakeDamage(TakeDamageInfo & a_TDI)
{
	if (!super::DoTakeDamage(a_TDI))
	{
		return false;
	}
	if ((a_TDI.Attacker != this) && (a_TDI.Attacker != nullptr))
	{
		m_EMState = ESCAPING;
	}
	return true;
}





void cPassiveMonster::EngageLoveMode(cPassiveMonster * a_Partner)
{
	m_LovePartner = a_Partner;
	m_MatingTimer = 50;  // about 3 seconds of mating
}





void cPassiveMonster::ResetLoveMode()
{
	m_LovePartner = nullptr;
	m_LoveTimer = 0;
	m_MatingTimer = 0;
	m_LoveCooldown = 20 * 60 * 5;  // 5 minutes

	// when an animal is in love mode, the client only stops sending the hearts if we let them know it's in cooldown, which is done with the "age" metadata
	m_World->BroadcastEntityMetadata(*this);
}





void cPassiveMonster::SpawnOn(cClientHandle & a_Client)
{
	super::SpawnOn(a_Client);

	if (IsLeashed())
	{
		a_Client.SendLeashEntity(*this, *this->GetLeashedTo());
	}
}





void cPassiveMonster::Destroy(bool a_ShouldBroadcast)
{	
	if (IsLeashed())
	{
		cEntity * LeashedTo = GetLeashedTo();
		LeashedTo->RemoveLeashedMob(this, false, a_ShouldBroadcast);

		// Remove leash knot if there are no more mobs leashed to
		if (!LeashedTo->HasAnyMobLeashed() && LeashedTo->IsLeashKnot())
		{
			LeashedTo->Destroy();
		}
	}

	if (m_LovePartner != nullptr)
	{
		m_LovePartner->ResetLoveMode();
	}

	super::Destroy(a_ShouldBroadcast);
}





void cPassiveMonster::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	super::Tick(a_Dt, a_Chunk);
	if (!IsTicking() || ((m_TicksAlive % TICK_STEP) != 0)) // only do this calcs twice per second
	{
		// The base class tick destroyed us
		return;
	}

	// This mob just spotted in the world and should be leahed to an entity
	if (!IsLeashed() && (m_LeashToPos != nullptr))
	{
		LOGD("Mob was leashed to pos %f, %f, %f, looking for leash knot...", m_LeashToPos->x, m_LeashToPos->y, m_LeashToPos->z);

		class LookForKnots : public cEntityCallback
		{
		public:
			cPassiveMonster * m_Monster;

			LookForKnots(cPassiveMonster * a_Monster) :
				m_Monster(a_Monster)
			{
			}

			virtual bool Item(cEntity * a_Entity) override
			{
				if (a_Entity->IsLeashKnot())
				{
					LOGD("Leash knot found");
					a_Entity->AddLeashedMob(m_Monster);											
				}
				return false;
			}
		} Callback(this);
		m_World->ForEachEntityInBox(cBoundingBox(m_LeashToPos, 0.5, 1), Callback);
		m_LeashToPos = nullptr;		
	}

	if (m_EMState == ESCAPING)
	{
		CheckEventLostPlayer();
	}

	// if we have a partner, mate
	if (m_LovePartner != nullptr)
	{

		if (m_MatingTimer > 0)
		{
			// If we should still mate, keep bumping into them until baby is made
			Vector3d Pos = m_LovePartner->GetPosition();
			MoveToPosition(Pos);
		}
		else
		{
			// Mating finished. Spawn baby
			Vector3f Pos = (GetPosition() + m_LovePartner->GetPosition()) * 0.5;
			UInt32 BabyID = m_World->SpawnMob(Pos.x, Pos.y, Pos.z, GetMobType(), true);

			class cBabyInheritCallback :
				public cEntityCallback
			{
			public:
				cPassiveMonster * Baby;
				cBabyInheritCallback() : Baby(nullptr) { }
				virtual bool Item(cEntity * a_Entity) override
				{
					Baby = static_cast<cPassiveMonster *>(a_Entity);
					return true;
				}
			} Callback;

			m_World->DoWithEntityByID(BabyID, Callback);
			if (Callback.Baby != nullptr)
			{
				Callback.Baby->InheritFromParents(this, m_LovePartner);
			}

			m_World->SpawnExperienceOrb(Pos.x, Pos.y, Pos.z, GetRandomProvider().RandInt(1, 6));

			m_LovePartner->ResetLoveMode();
			ResetLoveMode();
		}
	}
	else if (IsLeashed())
	{
		//Mob is leashed to an entity
				
		// TODO: leashed mobs can move around up to 5 blocks distance from leash origin
		MoveToPosition(m_LeashedTo->GetPosition());

		// If distance to target > 10 break lead
		Vector3f a_Distance(m_LeashedTo->GetPosition() - GetPosition());
		double Distance(a_Distance.Length());
		if (Distance > 10.0)
		{
			LOGD("Lead broken (distance)");
			m_LeashedTo->RemoveLeashedMob(this, false);			
		}
			
	}
	else
	{
		// We have no partner and no lead, so we just chase the player if they have our breeding item
		cItems FollowedItems;
		GetFollowedItems(FollowedItems);
		if (FollowedItems.Size() > 0)
		{
			cPlayer * a_Closest_Player = m_World->FindClosestPlayer(GetPosition(), static_cast<float>(m_SightDistance));
			if (a_Closest_Player != nullptr)
			{
				cItem EquippedItem = a_Closest_Player->GetEquippedItem();
				if (FollowedItems.ContainsType(EquippedItem))
				{
					Vector3d PlayerPos = a_Closest_Player->GetPosition();
					MoveToPosition(PlayerPos);
				}
			}
		}		
	}

	// If we are in love mode but we have no partner, search for a partner neabry
	if (m_LoveTimer > 0)
	{
		if (m_LovePartner == nullptr)
		{
			class LookForLover : public cEntityCallback
			{
			public:
				cEntity * m_Me;

				LookForLover(cEntity * a_Me) :
					m_Me(a_Me)
				{
				}

				virtual bool Item(cEntity * a_Entity) override
				{
					// If the entity is not a monster, don't breed with it
					// Also, do not self-breed
					if ((a_Entity->GetEntityType() != etMonster) || (a_Entity == m_Me))
					{
						return false;
					}

					cPassiveMonster * Me = static_cast<cPassiveMonster*>(m_Me);
					cPassiveMonster * PotentialPartner = static_cast<cPassiveMonster*>(a_Entity);

					// If the potential partner is not of the same species, don't breed with it
					if (PotentialPartner->GetMobType() != Me->GetMobType())
					{
						return false;
					}

					// If the potential partner is not in love
					// Or they already have a mate, do not breed with them
					if ((!PotentialPartner->IsInLove()) || (PotentialPartner->GetPartner() != nullptr))
					{
						return false;
					}

					// All conditions met, let's breed!
					PotentialPartner->EngageLoveMode(Me);
					Me->EngageLoveMode(PotentialPartner);
					return true;
				}
			} Callback(this);

			m_World->ForEachEntityInBox(cBoundingBox(GetPosition(), 8, 8), Callback);
		}

		m_LoveTimer-=TICK_STEP;
	}
	if (m_MatingTimer > 0)
	{
		m_MatingTimer-=TICK_STEP;
	}
	if (m_LoveCooldown > 0)
	{
		m_LoveCooldown-=TICK_STEP;
	}
}





void cPassiveMonster::OnRightClicked(cPlayer & a_Player)
{
	super::OnRightClicked(a_Player);

	short HeldItem = a_Player.GetEquippedItem().m_ItemType;

	// If a player holding breeding items right-clicked me, go into love mode
	if ((m_LoveCooldown == 0) && !IsInLove() && !IsBaby())
	{
		cItems Items;
		GetBreedingItems(Items);
		if (Items.ContainsType(HeldItem))
		{
			if (!a_Player.IsGameModeCreative())
			{
				a_Player.GetInventory().RemoveOneEquippedItem();
			}
			m_LoveTimer = 20 * 30;  // half a minute
			m_World->BroadcastEntityStatus(*this, esMobInLove);
		}
	}

	// Using leads
	m_LeadActionJustDone = false;
	if (IsLeashed() && (GetLeashedTo() == &a_Player)) // a player can only unleash a mob leashed to him
	{		
		a_Player.RemoveLeashedMob(this, !a_Player.IsGameModeCreative());
	}
	else if (IsLeashed())
	{
		// Mob is already leashed but client anticipates the server action and draws a leash link, so we need to send current leash to cancel it 
		m_World->BroadcastLeashEntity(*this, *this->GetLeashedTo());
	}
	else if (HeldItem == E_ITEM_LEAD)
	{
		if (!a_Player.IsGameModeCreative())
		{
			a_Player.GetInventory().RemoveOneEquippedItem();
		}
		a_Player.AddLeashedMob(this);

	}

}





void cPassiveMonster::SetLeashedTo(cEntity * a_Entity)
{	
	m_LeashedTo = a_Entity;

	m_LeadActionJustDone = true;
	LOGD("Mob leashed");
}





void cPassiveMonster::SetUnleashed(bool a_DropPickup)
{
	ASSERT(this->GetLeashedTo() != nullptr);

	m_LeashedTo = nullptr;

	// Drop pickup leash?
	if (a_DropPickup)
	{
		cItems Pickups;
		Pickups.Add(cItem(E_ITEM_LEAD, 1, 0));
		GetWorld()->SpawnItemPickups(Pickups, GetPosX() + 0.5, GetPosY() + 0.5, GetPosZ() + 0.5);
	}
	
	m_LeadActionJustDone = true;
	LOGD("Mob unleashed");
}




