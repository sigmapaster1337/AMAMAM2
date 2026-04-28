#include "Entities.h"

#include "../../SDK.h"
#include "../../../Utils/Hash/FNV1A.h"
#include "../../../Features/Players/PlayerUtils.h"
#include "../../../Features/Backtrack/Backtrack.h"
#include "../../../Features/CheaterDetection/CheaterDetection.h"
#include "../../../Features/Resolver/Resolver.h"

void CEntities::Store()
{
	auto pLocal = I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer());
	if (!pLocal)
		return;

	m_pLocal = pLocal->As<CTFPlayer>();
	m_pLocalWeapon = m_pLocal->m_hActiveWeapon()->As<CTFWeaponBase>();

	PollActiveSounds();

	int iLag;
	{
		static int iStaticTickcout = I::GlobalVars->tickcount;
		iLag = I::GlobalVars->tickcount - iStaticTickcout - 1;
		iStaticTickcout = I::GlobalVars->tickcount;
	}

	for (int n = I::EngineClient->GetMaxClients() + 1; n <= I::ClientEntityList->GetHighestEntityIndex(); n++)
	{
		auto pEntity = I::ClientEntityList->GetClientEntity(n)->As<CBaseEntity>();
		if (!pEntity || ManageDormancy(pEntity))
			continue;

		auto nClassID = pEntity->GetClassID();
		switch (nClassID)
		{
		case ETFClassID::CTFPlayerResource:
			m_pPlayerResource = pEntity->As<CTFPlayerResource>();
			break;
		case ETFClassID::CObjectSentrygun:
		case ETFClassID::CObjectDispenser:
		case ETFClassID::CObjectTeleporter:
			m_aModels[n] = FNV1A::Hash32(I::ModelInfoClient->GetModelName(pEntity->GetModel()));
			m_aGroups[EntityEnum::BuildingAll].push_back(pEntity);
			m_aGroups[pEntity->m_iTeamNum() != m_pLocal->m_iTeamNum() ? EntityEnum::BuildingEnemy : EntityEnum::BuildingTeam].push_back(pEntity);
			break;
		case ETFClassID::CBaseProjectile:
		case ETFClassID::CBaseGrenade:
		case ETFClassID::CTFWeaponBaseGrenadeProj:
		case ETFClassID::CTFWeaponBaseMerasmusGrenade:
		case ETFClassID::CTFGrenadePipebombProjectile:
		case ETFClassID::CTFStunBall:
		case ETFClassID::CTFBall_Ornament:
		case ETFClassID::CTFProjectile_Jar:
		case ETFClassID::CTFProjectile_Cleaver:
		case ETFClassID::CTFProjectile_JarGas:
		case ETFClassID::CTFProjectile_JarMilk:
		case ETFClassID::CTFProjectile_SpellBats:
		case ETFClassID::CTFProjectile_SpellKartBats:
		case ETFClassID::CTFProjectile_SpellMeteorShower:
		case ETFClassID::CTFProjectile_SpellMirv:
		case ETFClassID::CTFProjectile_SpellPumpkin:
		case ETFClassID::CTFProjectile_SpellSpawnBoss:
		case ETFClassID::CTFProjectile_SpellSpawnHorde:
		case ETFClassID::CTFProjectile_SpellSpawnZombie:
		case ETFClassID::CTFProjectile_SpellTransposeTeleport:
		case ETFClassID::CTFProjectile_Throwable:
		case ETFClassID::CTFProjectile_ThrowableBreadMonster:
		case ETFClassID::CTFProjectile_ThrowableBrick:
		case ETFClassID::CTFProjectile_ThrowableRepel:
		case ETFClassID::CTFBaseRocket:
		case ETFClassID::CTFFlameRocket:
		case ETFClassID::CTFProjectile_Arrow:
		case ETFClassID::CTFProjectile_GrapplingHook:
		case ETFClassID::CTFProjectile_HealingBolt:
		case ETFClassID::CTFProjectile_Rocket:
		case ETFClassID::CTFProjectile_BallOfFire:
		case ETFClassID::CTFProjectile_MechanicalArmOrb:
		case ETFClassID::CTFProjectile_SentryRocket:
		case ETFClassID::CTFProjectile_SpellFireball:
		case ETFClassID::CTFProjectile_SpellLightningOrb:
		case ETFClassID::CTFProjectile_SpellKartOrb:
		case ETFClassID::CTFProjectile_EnergyBall:
		case ETFClassID::CTFProjectile_Flare:
		case ETFClassID::CTFBaseProjectile:
		case ETFClassID::CTFProjectile_EnergyRing:
			//case ETFClassID::CTFProjectile_Syringe:
		{
			if ((nClassID == ETFClassID::CTFProjectile_Cleaver || nClassID == ETFClassID::CTFStunBall) && pEntity->As<CTFGrenadePipebombProjectile>()->m_bTouched()
				|| (nClassID == ETFClassID::CTFProjectile_Arrow || nClassID == ETFClassID::CTFProjectile_GrapplingHook) && !pEntity->m_MoveType())
				break;

			m_aGroups[EntityEnum::WorldProjectile].push_back(pEntity);

			if (nClassID == ETFClassID::CTFGrenadePipebombProjectile)
			{
				auto pPipebomb = pEntity->As<CTFGrenadePipebombProjectile>();
				if (pPipebomb->m_hThrower().Get() == pLocal && pPipebomb->m_iType() == TF_GL_MODE_REMOTE_DETONATE /*pPipebomb->HasStickyEffects()*/)
					m_aGroups[EntityEnum::LocalStickies].push_back(pEntity);
			}

			if (nClassID == ETFClassID::CTFProjectile_Flare)
			{
				auto pLauncher = pEntity->As<CTFProjectile_Flare>()->m_hLauncher()->As<CTFWeaponBase>();
				if (pEntity->m_hOwnerEntity().Get() == m_pLocal && pLauncher && pLauncher->As<CTFFlareGun>()->GetFlareGunType() == FLAREGUN_DETONATE)
					m_aGroups[EntityEnum::LocalFlares].push_back(pEntity);
			}

			break;
		}
		case ETFClassID::CTFBaseBoss:
		case ETFClassID::CTFTankBoss:
		case ETFClassID::CMerasmus:
		case ETFClassID::CEyeballBoss:
		case ETFClassID::CHeadlessHatman:
		case ETFClassID::CZombie:
			m_aGroups[EntityEnum::WorldNPC].push_back(pEntity);
			break;
		case ETFClassID::CTFGenericBomb:
		case ETFClassID::CTFPumpkinBomb:
			m_aGroups[EntityEnum::WorldBomb].push_back(pEntity);
			break;
		case ETFClassID::CBaseAnimating:
		{
			m_aModels[n] = FNV1A::Hash32(I::ModelInfoClient->GetModelName(pEntity->GetModel()));
			//if (IsHealth(GetModel(n)))
			//	m_aGroups[EntityEnum::PickupHealth].push_back(pEntity);
			//else if (IsAmmo(GetModel(n)))
			//	m_aGroups[EntityEnum::PickupAmmo].push_back(pEntity);
			//else if (IsPowerup(GetModel(n)))
			//	m_aGroups[EntityEnum::PickupPowerup].push_back(pEntity);
			//else if (IsSpellbook(GetModel(n)))
			//	m_aGroups[EntityEnum::PickupSpellbook].push_back(pEntity);
			break;
		}
		//case ETFClassID::CTFAmmoPack:
		//	m_aGroups[EntityEnum::PickupAmmo].push_back(pEntity);
		//	break;
		//case ETFClassID::CCurrencyPack:
		//	m_aGroups[EntityEnum::PickupMoney].push_back(pEntity);
		//	break;
		//case ETFClassID::CHalloweenGiftPickup:
		//	if (pEntity->As<CHalloweenGiftPickup>()->m_hTargetPlayer().Get() == m_pLocal)
		//		m_aGroups[EntityEnum::PickupGargoyle].push_back(pEntity);
		//	break;
		//case ETFClassID::CCaptureFlag:
		//	m_aGroups[EntityEnum::WorldObjective].push_back(pEntity);
		//	break;
		case ETFClassID::CSniperDot:
			m_aGroups[EntityEnum::SniperDots].push_back(pEntity);
			break;
		}
	}

	static Timer tTimer = {};
	bool bUpdateInfo = tTimer.Run(1.f);
	std::unordered_map<uint32_t, uint64_t> mParties = {};
	std::unordered_map<uint32_t, bool> mF2P = {};
	std::unordered_map<uint32_t, int> mLevels = {};
	if (bUpdateInfo)
	{
		m_mIPriorities.clear();
		m_mUPriorities.clear();
		m_mIFriends.clear();
		m_mUFriends.clear();
		m_mIParty.clear();
		m_mUParty.clear();
		m_mIF2P.clear();
		m_mUF2P.clear();
		m_mILevels.clear();
		m_mULevels.clear();

		if (auto pLobby = I::TFGCClientSystem->GetLobby())
		{
			auto pGameRules = I::TFGameRules();
			auto pMatchDesc = pGameRules ? pGameRules->GetMatchGroupDescription() : nullptr;

			int iMembers = pLobby->GetNumMembers();
			for (int i = 0; i < iMembers; i++)
			{
				auto tSteamID = CSteamID(); pLobby->GetMember(&tSteamID, i);
				auto uAccountID = tSteamID.GetAccountID();

				ConstTFLobbyPlayer pDetails;
				pLobby->GetMemberDetails(&pDetails, i);

				auto pProto = pDetails.Proto();
				mF2P[uAccountID] = pProto->chat_suspension;
				mParties[uAccountID] = pProto->original_party_id;
				if (pMatchDesc && pMatchDesc->m_pProgressionDesc)
					mLevels[uAccountID] = std::max(int(pProto->rank), pMatchDesc->GetLevelForSteamID(&tSteamID));
				else
					mLevels[uAccountID] = pProto->rank;
			}
		}
		if (auto pParty = I::TFGCClientSystem->GetParty())
		{
			int iMembers = pParty->GetNumMembers();
			for (int i = 0; i < iMembers; i++)
			{
				auto tSteamID = CSteamID(); pParty->GetMember(&tSteamID, i);
				auto uAccountID = tSteamID.GetAccountID();
				mParties[uAccountID] = 1;
			}
		}

		std::map<uint64_t, std::vector<uint32_t>> mParties2 = {};
		for (auto& [uAccountID, uParty] : mParties)
		{
			if (uParty)
				mParties2[uParty].push_back(uAccountID);
		}
		for (auto it = mParties2.begin(); it != mParties2.end();)
		{
			if (it->second.size() > 1)
				it++;
			else
				it = mParties2.erase(it);
		}
		mParties.clear();
		uint64_t uPartyCount = 0; for (auto& [uParty, vAccountIDs] : mParties2)
		{
			int iParty = uParty == 1 ? 1 : ++uPartyCount + 1;
			for (auto uAccountID : vAccountIDs)
				mParties[uAccountID] = iParty;
		}
		m_iPartyCount = uPartyCount;
	}
	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		if (bUpdateInfo)
		{
			auto pResource = GetResource();
			if (pResource && pResource->m_bValid(n))
			{
				bool bLocal = n == I::EngineClient->GetLocalPlayer();
				uint32_t uAccountID = pResource->m_iAccountID(n);
				if (bLocal) m_uAccountID = uAccountID;

				m_mIPriorities[n] = m_mUPriorities[uAccountID] = !bLocal ? F::PlayerUtils.GetPriority(uAccountID, false) : 0;
				m_mIFriends[n] = m_mUFriends[uAccountID] = !pResource->IsFakePlayer(n) ? I::SteamFriends->HasFriend({ uAccountID, 1, k_EUniversePublic, k_EAccountTypeIndividual }, k_EFriendFlagImmediate) : false;
				m_mIParty[n] = m_mUParty[uAccountID] = mParties.contains(uAccountID) ? mParties[uAccountID] : 0;
				m_mIF2P[n] = m_mUF2P[uAccountID] = mF2P.contains(uAccountID) ? mF2P[uAccountID] : false;
				m_mILevels[n] = m_mULevels[uAccountID] = mLevels.contains(uAccountID) ? mLevels[uAccountID] : -2;
			}
		}

		auto pPlayer = I::ClientEntityList->GetClientEntity(n)->As<CTFPlayer>();
		if (!pPlayer || !pPlayer->IsPlayer() || ManageDormancy(pPlayer))
			continue;

		m_aModels[n] = FNV1A::Hash32(I::ModelInfoClient->GetModelName(pPlayer->GetModel()));
		m_aGroups[EntityEnum::PlayerAll].push_back(pPlayer);
		m_aGroups[pPlayer->m_iTeamNum() != m_pLocal->m_iTeamNum() ? EntityEnum::PlayerEnemy : EntityEnum::PlayerTeam].push_back(pPlayer);

		if (n != I::EngineClient->GetLocalPlayer())
		{
			bool bDormant = pPlayer->IsDormant();
			float flSimTime = pPlayer->m_flSimulationTime(), flOldSimTime = pPlayer->m_flOldSimulationTime();
			if (float flDeltaTime = m_aDeltaTimes[n] = TICKS_TO_TIME(std::clamp(TIME_TO_TICKS(flSimTime - flOldSimTime) - iLag, 0, 24)))
			{
				m_aLagTimes[n] = flDeltaTime;
				m_aSetTicks[n] = I::GlobalVars->tickcount;
				if (!bDormant)
				{
					m_aOrigins[n].emplace_front(pPlayer->m_vecOrigin() + Vec3(0, 0, pPlayer->GetSize().z), flSimTime);
					if (m_aOrigins[n].size() > Vars::Aimbot::Projectile::VelocityAverageCount.Value)
						m_aOrigins[n].pop_back();

					if (pPlayer->IsAlive())
						F::CheaterDetection.ReportChoke(pPlayer, m_aChokes[n]);
				}
				else
					m_aOrigins[n].clear();

				m_aOldAngles[n] = m_aEyeAngles[n];
				m_aEyeAngles[n] = pPlayer->As<CTFPlayer>()->GetEyeAngles();
			}
			m_aChokes[n] = I::GlobalVars->tickcount - m_aSetTicks[n];
		}
	}
	F::Resolver.FrameStageNotify();
	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (!pPlayer->IsAlive() || pPlayer->entindex() == I::EngineClient->GetLocalPlayer() && !I::EngineClient->IsPlayingDemo()) // local player managed in CreateMove
			continue;

		bool bResolver = F::Resolver.GetAngles(pPlayer);
		if (!Vars::Visuals::Removals::Interpolation.Value && !bResolver)
			continue;

		int iDeltaTicks = TIME_TO_TICKS(H::Entities.GetDeltaTime(pPlayer->entindex()));
		if (!iDeltaTicks)
			continue;

		float flOldFrameTime = I::GlobalVars->frametime;
		I::GlobalVars->frametime = I::Prediction->m_bEnginePaused ? 0.f : TICK_INTERVAL;
		for (int i = 0; i < iDeltaTicks; i++)
		{
			G::UpdatingAnims = true;

			if (bResolver)
			{
				float flYaw, flPitch;
				F::Resolver.GetAngles(pPlayer, &flYaw, &flPitch, nullptr, i + 1 == iDeltaTicks);

				float flOriginalYaw = pPlayer->m_angEyeAnglesY(), flOriginalPitch = pPlayer->m_angEyeAnglesX();
				pPlayer->m_angEyeAnglesY() = flYaw, pPlayer->m_angEyeAnglesX() = flPitch;
				pPlayer->UpdateClientSideAnimation();
				pPlayer->m_angEyeAnglesY() = flOriginalYaw, pPlayer->m_angEyeAnglesX() = flOriginalPitch;
			}
			else
				pPlayer->UpdateClientSideAnimation();

			G::UpdatingAnims = false;
		}
		I::GlobalVars->frametime = flOldFrameTime;
	}
}

static std::unordered_map<unsigned short, DormantData> s_mDormancy = {};
static std::array<DormantData, MAX_EDICTS> s_aSoundDormancy = {};

static float GetDormantDuration(CBaseEntity* pEntity, bool bFromSound)
{
	switch (pEntity->GetClassID())
	{
	case ETFClassID::CTFPlayer: return bFromSound ? 10.f : 1.f;
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser:
	case ETFClassID::CObjectTeleporter: return 5.f;
	}

	return 0.f;
}

static bool AdjustDormantSound(CBaseEntity* pEntity, Vec3& vOrigin, int& iFlags)
{
	if (vOrigin.IsZero())
		return false;

	CGameTrace trace = {};
	CTraceFilterWorldAndPropsOnly filter = {};
	filter.pSkip = pEntity;

	const Vec3 vStart = vOrigin + Vec3(0.f, 0.f, 1.f);
	const Vec3 vEnd = vStart - Vec3(0.f, 0.f, 100.f);
	SDK::Trace(vStart, vEnd, MASK_SOLID, &filter, &trace);
	if (trace.allsolid)
		return false;

	if (trace.fraction <= 0.97f)
		vOrigin = trace.endpos;

	if (pEntity->IsPlayer())
	{
		iFlags = pEntity->As<CTFPlayer>()->m_fFlags();
		if (trace.fraction < 0.50f)
			iFlags |= FL_DUCKING;
		else
			iFlags &= ~FL_DUCKING;

		if (trace.fraction < 1.f)
			iFlags |= FL_ONGROUND;
		else
			iFlags &= ~FL_ONGROUND;
	}

	return true;
}

static bool ShouldStoreDormantSound(unsigned short n, const Vec3& vOrigin, float flCurTime)
{
	const auto& tLastSound = s_aSoundDormancy[n];
	if (tLastSound.m_flReceiveTime <= 0.f)
		return true;

	if (flCurTime - tLastSound.m_flReceiveTime > 0.05f)
		return true;

	return tLastSound.m_vLocation.DistToSqr(vOrigin) > 1.f;
}

static void StoreDormantPoint(unsigned short n, const Vec3& vOrigin, float flDuration, bool bFromSound, int iFlags = 0)
{
	const float flCurTime = I::GlobalVars->curtime;
	const DormantData tDormancy = { vOrigin, flCurTime + flDuration, flCurTime, iFlags, bFromSound };
	s_mDormancy[n] = tDormancy;
	if (bFromSound)
		s_aSoundDormancy[n] = tDormancy;
}

void CEntities::Clear(bool bShutdown)
{
	m_pLocal = nullptr;
	m_pLocalWeapon = nullptr;
	m_pPlayerResource = nullptr;
	m_aGroups = {};

	if (bShutdown)
	{
		m_aDeltaTimes = {};
		m_aLagTimes = {};
		m_aChokes = {};
		m_aSetTicks = {};
		m_aOldAngles = {};
		m_aEyeAngles = {};
		m_aLagCompensation = {};
		m_aAvgVelocities = {};
		m_aOrigins = {};
		m_aModels = {};
		m_aDormancy = {};
		s_mDormancy.clear();
		s_aSoundDormancy = {};
	}
}

void CEntities::ManualNetwork(const StartSoundParams_t& params)
{
	int n = params.soundsource;
	if (n <= 0 || n > MAX_EDICTS - 1 || params.origin.IsZero() || n == I::EngineClient->GetLocalPlayer())
		return;

	auto pClientEntity = I::ClientEntityList->GetClientEntity(n);
	if (!pClientEntity)
		return;

	auto pEntity = pClientEntity->As<CBaseEntity>();
	if (!pEntity || !pEntity->IsDormant() || !pEntity->IsPlayer() && !pEntity->IsBuilding())
		return;

	float flDuration = GetDormantDuration(pEntity, true);
	if (!flDuration)
		return;

	Vec3 vOrigin = params.origin;
	int iFlags = 0;
	if (!AdjustDormantSound(pEntity, vOrigin, iFlags) || !ShouldStoreDormantSound(n, vOrigin, I::GlobalVars->curtime))
		return;

	StoreDormantPoint(n, vOrigin, flDuration, true, iFlags);
	m_aDormancy[n] = true;
}
bool CEntities::ManageDormancy(CBaseEntity* pEntity)
{
	bool bDormant = pEntity->IsDormant();

	float flDuration = GetDormantDuration(pEntity, false);
	if (!flDuration)
		return bDormant;

	int n = pEntity->entindex();
	if (n < 0 || n > MAX_EDICTS - 1)
		return bDormant;

	if (bDormant)
	{
		if (pEntity->IsPlayer())
		{
			if (auto pResource = GetResource(); pResource)
			{
				bool bAlive = pResource->m_bAlive(n);
				pEntity->As<CTFPlayer>()->m_lifeState() = bAlive ? LIFE_ALIVE : LIFE_DEAD;
				pEntity->As<CTFPlayer>()->m_iHealth() = pResource->m_iHealth(n);
				if (!bAlive)
				{
					s_mDormancy.erase(n);
					s_aSoundDormancy[n] = {};
					m_aDormancy[n] = false;
					return bDormant;
				}
			}
		}
		if (s_mDormancy.contains(n))
		{
			auto& tDormancy = s_mDormancy[n];
			if (tDormancy.m_flLastUpdate - I::GlobalVars->curtime > 0.f)
			{
				if (pEntity->IsPlayer() && tDormancy.m_bFromSound)
					pEntity->As<CTFPlayer>()->m_fFlags() = tDormancy.m_iFlags;
				pEntity->SetAbsOrigin(pEntity->m_vecOrigin() = tDormancy.m_vLocation);
			}
			else
			{
				s_mDormancy.erase(n);
				s_aSoundDormancy[n] = {};
				m_aDormancy[n] = false;
			}
		}
	}
	else if (!pEntity->IsPlayer() || pEntity->As<CTFPlayer>()->IsAlive())
	{
		StoreDormantPoint(n, pEntity->m_vecOrigin(), flDuration, false, pEntity->IsPlayer() ? pEntity->As<CTFPlayer>()->m_fFlags() : 0);
		m_aDormancy[n] = true;
	}
	else
	{
		s_mDormancy.erase(n);
		s_aSoundDormancy[n] = {};
		m_aDormancy[n] = false;
	}
	return bDormant;
}

bool CEntities::IsHealth(uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("models/items/banana/plate_banana.mdl"):
	case FNV1A::Hash32Const("models/items/medkit_small.mdl"):
	case FNV1A::Hash32Const("models/items/medkit_medium.mdl"):
	case FNV1A::Hash32Const("models/items/medkit_large.mdl"):
	case FNV1A::Hash32Const("models/items/medkit_small_bday.mdl"):
	case FNV1A::Hash32Const("models/items/medkit_medium_bday.mdl"):
	case FNV1A::Hash32Const("models/items/medkit_large_bday.mdl"):
	case FNV1A::Hash32Const("models/items/plate.mdl"):
	case FNV1A::Hash32Const("models/items/plate_sandwich_xmas.mdl"):
	case FNV1A::Hash32Const("models/items/plate_robo_sandwich.mdl"):
	case FNV1A::Hash32Const("models/props_medieval/medieval_meat.mdl"):
	case FNV1A::Hash32Const("models/workshopweapons/c_models/c_chocolate/plate_chocolate.mdl"):
	case FNV1A::Hash32Const("models/workshopweapons/c_models/c_fishcake/plate_fishcake.mdl"):
	case FNV1A::Hash32Const("models/props_halloween/halloween_medkit_small.mdl"):
	case FNV1A::Hash32Const("models/props_halloween/halloween_medkit_medium.mdl"):
	case FNV1A::Hash32Const("models/props_halloween/halloween_medkit_large.mdl"):
	case FNV1A::Hash32Const("models/items/ld1/mushroom_large.mdl"):
	case FNV1A::Hash32Const("models/items/plate_steak.mdl"):
	case FNV1A::Hash32Const("models/props_brine/foodcan.mdl"):
		return true;
	}
	return false;
}

bool CEntities::IsAmmo(uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("models/items/ammopack_small.mdl"):
	case FNV1A::Hash32Const("models/items/ammopack_medium.mdl"):
	case FNV1A::Hash32Const("models/items/ammopack_large.mdl"):
	case FNV1A::Hash32Const("models/items/ammopack_large_bday.mdl"):
	case FNV1A::Hash32Const("models/items/ammopack_medium_bday.mdl"):
	case FNV1A::Hash32Const("models/items/ammopack_small_bday.mdl"):
		return true;
	}
	return false;
}

bool CEntities::IsPowerup(uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_agility.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_crit.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_defense.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_haste.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_king.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_knockout.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_plague.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_precision.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_reflect.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_regen.mdl"):
		//case FNV1A::Hash32Const("models/pickups/pickup_powerup_resistance.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_strength.mdl"):
		//case FNV1A::Hash32Const("models/pickups/pickup_powerup_strength_arm.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_supernova.mdl"):
		//case FNV1A::Hash32Const("models/pickups/pickup_powerup_thorns.mdl"):
		//case FNV1A::Hash32Const("models/pickups/pickup_powerup_uber.mdl"):
	case FNV1A::Hash32Const("models/pickups/pickup_powerup_vampire.mdl"):
		return true;
	}
	return false;
}

bool CEntities::IsSpellbook(uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("models/props_halloween/hwn_spellbook_flying.mdl"):
	case FNV1A::Hash32Const("models/props_halloween/hwn_spellbook_upright.mdl"):
	case FNV1A::Hash32Const("models/props_halloween/hwn_spellbook_upright_major.mdl"):
	case FNV1A::Hash32Const("models/items/crystal_ball_pickup.mdl"):
	case FNV1A::Hash32Const("models/items/crystal_ball_pickup_major.mdl"):
	case FNV1A::Hash32Const("models/props_monster_mash/flask_vial_green.mdl"):
	case FNV1A::Hash32Const("models/props_monster_mash/flask_vial_purple.mdl"): // prop_dynamic in the map, probably won't work
		return true;
	}
	return false;
}

CTFPlayer* CEntities::GetLocal()
{
	return I::ClientEntityList->GetClientEntity(I::EngineClient->GetLocalPlayer())->As<CTFPlayer>();
	//return m_pLocal;
}
CTFWeaponBase* CEntities::GetWeapon()
{
	auto pLocal = GetLocal();
	return pLocal ? pLocal->m_hActiveWeapon()->As<CTFWeaponBase>() : nullptr;
	//return m_pLocalWeapon;
}
CTFPlayerResource* CEntities::GetResource()
{
	return m_pPlayerResource;
}

void CEntities::PollActiveSounds()
{
	if (!I::EngineSound)
		return;

	static CUtlVector<SndInfo_t> activeSounds;
	I::EngineSound->GetActiveSounds(activeSounds);

	if (!activeSounds.Count())
		return;

	for (int i = 0; i < activeSounds.Count(); i++)
	{
		auto& sound = activeSounds[i];

		if (!sound.m_pOrigin || sound.m_nSoundSource < 1 || sound.m_nSoundSource > I::EngineClient->GetMaxClients())
			continue;

		int iIndex = sound.m_nSoundSource;

		if (iIndex == I::EngineClient->GetLocalPlayer())
			continue;

		auto pPlayer = I::ClientEntityList->GetClientEntity(iIndex)->As<CTFPlayer>();
		if (!pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsDormant())
			continue;

		if (pPlayer->m_iTeamNum() == m_pLocal->m_iTeamNum())
			continue;

		// FIX: Change m_mDormancy to s_mDormancy
		float flDuration = 1.f;
		s_mDormancy[iIndex] = { *sound.m_pOrigin, I::GlobalVars->curtime + flDuration, I::GlobalVars->curtime, 0, true };

		// Also set the boolean flag so H::Entities.GetDormancy() works correctly
		m_aDormancy[iIndex] = true;

		pPlayer->SetAbsOrigin(*sound.m_pOrigin);
	}

	activeSounds.RemoveAll();
}

const std::vector<CBaseEntity*>& CEntities::GetGroup(byte iGroup) { return m_aGroups[iGroup]; }

float CEntities::GetDeltaTime(byte iIndex) { return iIndex < MAX_PLAYERS ? m_aDeltaTimes[iIndex] : TICK_INTERVAL; }
float CEntities::GetLagTime(byte iIndex) { return iIndex < MAX_PLAYERS ? m_aLagTimes[iIndex] : TICK_INTERVAL; }
int CEntities::GetChoke(byte iIndex) { return iIndex < MAX_PLAYERS ? m_aChokes[iIndex] : 0; }
Vec3 CEntities::GetEyeAngles(byte iIndex) { return iIndex < MAX_PLAYERS ? m_aEyeAngles[iIndex] : Vec3(); }
Vec3 CEntities::GetDeltaAngles(byte iIndex) { return iIndex < MAX_PLAYERS ? m_aEyeAngles[iIndex].DeltaAngle(m_aOldAngles[iIndex]) / GetLagTime(iIndex) * (F::Backtrack.GetReal() + TICKS_TO_TIME(F::Backtrack.GetAnticipatedChoke())) : Vec3(); }
bool CEntities::GetLagCompensation(byte iIndex) { return iIndex < MAX_PLAYERS ? m_aLagCompensation[iIndex] : false; }
void CEntities::SetLagCompensation(byte iIndex, bool bLagComp) { if (iIndex < MAX_PLAYERS) m_aLagCompensation[iIndex] = bLagComp; }
Vec3* CEntities::GetAvgVelocity(byte iIndex) { return iIndex < MAX_PLAYERS && iIndex != I::EngineClient->GetLocalPlayer() ? &m_aAvgVelocities[iIndex] : nullptr; }
void CEntities::SetAvgVelocity(byte iIndex, Vec3 vAvgVelocity) { if (iIndex < MAX_PLAYERS) m_aAvgVelocities[iIndex] = vAvgVelocity; }
std::deque<VelFixRecord>* CEntities::GetOrigins(byte iIndex) { return iIndex < MAX_PLAYERS ? &m_aOrigins[iIndex] : nullptr; }
uint32_t CEntities::GetModel(unsigned short iIndex) { return iIndex < MAX_EDICTS ? m_aModels[iIndex] : 0; }
bool CEntities::GetDormancy(unsigned short iIndex) { return iIndex < MAX_EDICTS ? m_aDormancy[iIndex] : false; }
DormantData* CEntities::GetDormancyData(unsigned short iIndex)
{
	if (iIndex >= MAX_EDICTS)
		return nullptr;

	auto it = s_mDormancy.find(iIndex);
	return it != s_mDormancy.end() ? &it->second : nullptr;
}

int CEntities::GetPriority(int iIndex) { return m_mIPriorities[iIndex]; }
int CEntities::GetPriority(uint32_t uAccountID) { return m_mUPriorities[uAccountID]; }
bool CEntities::IsFriend(int iIndex) { return m_mIFriends[iIndex]; }
bool CEntities::IsFriend(uint32_t uAccountID) { return m_mUFriends[uAccountID]; }
bool CEntities::InParty(int iIndex) { return iIndex != I::EngineClient->GetLocalPlayer() && m_mIParty[iIndex] == 1; }
bool CEntities::InParty(uint32_t uAccountID) { return uAccountID != m_uAccountID && m_mUParty[uAccountID] == 1; }
bool CEntities::IsF2P(int iIndex) { return m_mIF2P[iIndex]; }
bool CEntities::IsF2P(uint32_t uAccountID) { return m_mUF2P[uAccountID]; }
int CEntities::GetLevel(int iIndex) { return m_mILevels.contains(iIndex) ? m_mILevels[iIndex] : -2; }
int CEntities::GetLevel(uint32_t uAccountID) { return m_mULevels.contains(uAccountID) ? m_mULevels[uAccountID] : -2; }
int CEntities::GetParty(int iIndex) { return m_mIParty.contains(iIndex) ? m_mIParty[iIndex] : 0; }
int CEntities::GetParty(uint32_t uAccountID) { return m_mUParty.contains(uAccountID) ? m_mUParty[uAccountID] : 0; }
int CEntities::GetPartyCount() { return m_iPartyCount; }
