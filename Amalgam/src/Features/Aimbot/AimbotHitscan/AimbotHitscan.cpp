#include "AimbotHitscan.h"

#include "../Aimbot.h"
#include "../../Resolver/Resolver.h"
#include "../../Ticks/Ticks.h"
#include "../../Visuals/Visuals.h"
#include "../../Simulation/MovementSimulation/MovementSimulation.h"

std::vector<Target_t> CAimbotHitscan::GetTargets(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	std::vector<Target_t> vTargets;
	const auto iSort = Vars::Aimbot::General::TargetSelection.Value;

	Vec3 vLocalPos = F::Ticks.GetShootPos();
	Vec3 vLocalAngles = I::EngineClient->GetViewAngles();

	{
		auto eGroupType = EntityEnum::Invalid;
		if (Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Players)
		{
			eGroupType = !F::AimbotGlobal.FriendlyFire() || Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Team ? EntityEnum::PlayerEnemy : EntityEnum::PlayerAll;
			if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::ExtinguishTeam &&
				!F::AimbotGlobal.FriendlyFire() && SDK::AttribHookValue(0, "jarate_duration", pWeapon) > 0)
				eGroupType = EntityEnum::PlayerAll;
		}
		if (pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN)
			eGroupType = Vars::Aimbot::Healing::AutoHeal.Value ? EntityEnum::PlayerTeam : EntityEnum::Invalid;
		bool bHeal = pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN;

		for (auto pEntity : H::Entities.GetGroup(eGroupType))
		{
			if (F::AimbotGlobal.ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			bool bTeam = pEntity->m_iTeamNum() == pLocal->m_iTeamNum();
			if (bTeam)
			{
				if (bHeal)
				{
					if (pEntity->As<CTFPlayer>()->InCond(TF_COND_STEALTHED)
						|| Vars::Aimbot::Healing::HealPriority.Value == Vars::Aimbot::Healing::HealPriorityEnum::FriendsOnly
						&& !H::Entities.IsFriend(pEntity->entindex()) && !H::Entities.InParty(pEntity->entindex()))
						continue;
				}
				if (!F::AimbotGlobal.FriendlyFire() && SDK::AttribHookValue(0, "jarate_duration", pWeapon) > 0)
				{
					if (!pEntity->As<CTFPlayer>()->InCond(TF_COND_BURNING))
						continue;
				}
			}

			float flFOVTo; Vec3 vPos, vAngleTo;
			if (!F::AimbotGlobal.PlayerBoneInFOV(pEntity->As<CTFPlayer>(), vLocalPos, vLocalAngles, flFOVTo, vPos, vAngleTo, Vars::Aimbot::General::AimFOV.Value, Vars::Aimbot::Hitscan::Hitboxes.Value))
				continue;

			int iPriority = F::AimbotGlobal.GetPriority(pEntity->entindex());
			if (!F::AimbotGlobal.ShouldTargetPriority(iPriority))
				continue;
			if (bTeam && bHeal)
			{
				iPriority = 0;
				switch (Vars::Aimbot::Healing::HealPriority.Value)
				{
				case Vars::Aimbot::Healing::HealPriorityEnum::PrioritizeFriends:
					if (H::Entities.IsFriend(pEntity->entindex()) || H::Entities.InParty(pEntity->entindex()))
						iPriority = std::numeric_limits<int>::max();
					break;
				case Vars::Aimbot::Healing::HealPriorityEnum::PrioritizeTeam:
					iPriority = std::numeric_limits<int>::max();
				}
			}

			float flDistTo = iSort == Vars::Aimbot::General::TargetSelectionEnum::Distance ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back(pEntity, TargetEnum::Player, vPos, vAngleTo, flFOVTo, flDistTo, iPriority);
		}

		if (pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN)
			return vTargets;
	}

	if (Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Building)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::BuildingEnemy))
		{
			if (F::AimbotGlobal.ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			int iPriority = 0;
			if (auto pOwner = pEntity->As<CBaseObject>()->m_hBuilder().Get())
				iPriority = F::AimbotGlobal.GetPriority(pOwner->entindex());
			if (!F::AimbotGlobal.ShouldTargetPriority(iPriority))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > Vars::Aimbot::General::AimFOV.Value)
				continue;

			float flDistTo = iSort == Vars::Aimbot::General::TargetSelectionEnum::Distance ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back(pEntity, pEntity->IsSentrygun() ? TargetEnum::Sentry : pEntity->IsDispenser() ? TargetEnum::Dispenser : TargetEnum::Teleporter, vPos, vAngleTo, flFOVTo, flDistTo);
		}
	}

	if (Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Stickies)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::WorldProjectile))
		{
			if (F::AimbotGlobal.ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			int iPriority = 0;
			if (auto pOwner = pEntity->As<CTFGrenadePipebombProjectile>()->m_hThrower().Get())
				iPriority = F::AimbotGlobal.GetPriority(pOwner->entindex());
			if (!F::AimbotGlobal.ShouldTargetPriority(iPriority))
				continue;

			Vec3 vPos = pEntity->m_vecOrigin();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > Vars::Aimbot::General::AimFOV.Value)
				continue;

			float flDistTo = iSort == Vars::Aimbot::General::TargetSelectionEnum::Distance ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back(pEntity, TargetEnum::Sticky, vPos, vAngleTo, flFOVTo, flDistTo);
		}
	}

	if (Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::NPCs)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::WorldNPC))
		{
			if (F::AimbotGlobal.ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			if (!F::AimbotGlobal.ShouldTargetPriority(0))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > Vars::Aimbot::General::AimFOV.Value)
				continue;

			float flDistTo = iSort == Vars::Aimbot::General::TargetSelectionEnum::Distance ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back(pEntity, TargetEnum::NPC, vPos, vAngleTo, flFOVTo, flDistTo);
		}
	}

	if (Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Bombs)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::WorldBomb))
		{
			if (F::AimbotGlobal.ShouldIgnore(pEntity, pLocal, pWeapon))
				continue;

			if (!F::AimbotGlobal.ShouldTargetPriority(0))
				continue;

			Vec3 vPos = pEntity->GetCenter();
			Vec3 vAngleTo = Math::CalcAngle(vLocalPos, vPos);
			float flFOVTo = Math::CalcFov(vLocalAngles, vAngleTo);
			if (flFOVTo > Vars::Aimbot::General::AimFOV.Value)
				continue;

			float flDistTo = iSort == Vars::Aimbot::General::TargetSelectionEnum::Distance ? vLocalPos.DistTo(vPos) : 0.f;
			vTargets.emplace_back(pEntity, TargetEnum::Bomb, vPos, vAngleTo, flFOVTo, flDistTo);
		}
	}

	return vTargets;
}

std::vector<Target_t> CAimbotHitscan::SortTargets(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	auto vTargets = GetTargets(pLocal, pWeapon);

	F::AimbotGlobal.SortTargets(vTargets, Vars::Aimbot::General::TargetSelection.Value);
	vTargets.resize(std::min(size_t(Vars::Aimbot::General::MaxTargets.Value), vTargets.size()));
	F::AimbotGlobal.SortPriority(vTargets);
	return vTargets;
}



int CAimbotHitscan::GetHitboxPriority(int nHitbox, CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CBaseEntity* pTarget)
{
	if (!F::AimbotGlobal.IsHitboxValid(pTarget, nHitbox, Vars::Aimbot::Hitscan::Hitboxes.Value))
		return -1;

	// x32-style: class-specific hitbox priority
	// Classes: 1=scout, 2=soldier, 3=pyro, 4=demoman, 5=heavy, 6=engineer, 7=medic, 8=sniper, 9=spy
	if (pTarget->IsPlayer())
	{
		auto pPlayer = pTarget->As<CTFPlayer>();
		int nClass = pPlayer->m_iClass();

		// Heavy (5) and Scout (1): prioritize body hitboxes
		if (nClass == 5 || nClass == 1)
		{
			switch (pTarget->GetHitboxToBase(nHitbox))
			{
			case HITBOX_SPINE0:
			case HITBOX_SPINE1:
			case HITBOX_SPINE2:
			case HITBOX_SPINE3:
			case HITBOX_PELVIS:
				return 0; // Body first
			case HITBOX_HEAD:
				return 1;
			}
			return 2;
		}

		// Sniper (8) and Spy (9): head first, fallback to body
		if (nClass == 8 || nClass == 9)
		{
			bool bHeadshot = false;
			switch (pWeapon->GetWeaponID())
			{
			case TF_WEAPON_SNIPERRIFLE:
			case TF_WEAPON_SNIPERRIFLE_DECAP:
			case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			{
				auto pSniperRifle = pWeapon->As<CTFSniperRifle>();
				if (G::CanHeadshot
					|| pLocal->InCond(TF_COND_AIMING) && (pSniperRifle->GetRifleType() == RIFLE_JARATE && SDK::AttribHookValue(0, "jarate_duration", pWeapon) > 0 || Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::WaitForHeadshot))
					bHeadshot = true;
				break;
			}
			case TF_WEAPON_REVOLVER:
			{
				if (SDK::AttribHookValue(0, "set_weapon_mode", pWeapon) == 1 && (pWeapon->AmbassadorCanHeadshot() || Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::WaitForHeadshot))
					bHeadshot = true;
			}
			}

			switch (pTarget->GetHitboxToBase(nHitbox))
			{
			case HITBOX_HEAD:
				return bHeadshot ? 0 : 1;
			case HITBOX_SPINE0:
			case HITBOX_SPINE1:
			case HITBOX_SPINE2:
			case HITBOX_SPINE3:
				return bHeadshot ? 1 : 0;
			case HITBOX_PELVIS:
				return 2;
			}
			return 3;
		}

		// Other classes: neck/pelvis first
		bool bHeadshot = false;
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		{
			auto pSniperRifle = pWeapon->As<CTFSniperRifle>();
			if (G::CanHeadshot
				|| pLocal->InCond(TF_COND_AIMING) && (pSniperRifle->GetRifleType() == RIFLE_JARATE && SDK::AttribHookValue(0, "jarate_duration", pWeapon) > 0 || Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::WaitForHeadshot))
				bHeadshot = true;
			break;
		}
		case TF_WEAPON_REVOLVER:
		{
			if (SDK::AttribHookValue(0, "set_weapon_mode", pWeapon) == 1 && (pWeapon->AmbassadorCanHeadshot() || Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::WaitForHeadshot))
				bHeadshot = true;
		}
		}

		switch (pTarget->GetHitboxToBase(nHitbox))
		{
		case HITBOX_HEAD:
			return bHeadshot ? 0 : 2;
		case HITBOX_SPINE0:
		case HITBOX_SPINE1:
		case HITBOX_SPINE2:
		case HITBOX_SPINE3:
		case HITBOX_PELVIS:
			return 0; // Body/neck first for other classes
		}
		return 1;
	}

	// Default priority for non-players
	return 0;
}

int CAimbotHitscan::CanHit(Target_t& tTarget, CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	if (Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Unsimulated && H::Entities.GetChoke(tTarget.m_pEntity->entindex()) > Vars::Aimbot::General::TickTolerance.Value)
		return false;

	m_vEyePos = pLocal->GetShootPos();
	const float flMaxRange = powf(pWeapon->GetRange(), 2.f);

	auto pModel = tTarget.m_pEntity->GetModel();
	if (!pModel) return false;
	auto pHDR = I::ModelInfoClient->GetStudiomodel(pModel);
	if (!pHDR) return false;
	auto pSet = pHDR->pHitboxSet(tTarget.m_pEntity->As<CBaseAnimating>()->m_nHitboxSet());
	if (!pSet) return false;

	std::vector<TickRecord*> vRecords = {};
	if (F::Backtrack.GetRecords(tTarget.m_pEntity, vRecords))
	{
		vRecords = F::Backtrack.GetValidRecords(vRecords, pLocal);
		if (vRecords.empty())
			return false;
	}
	else
	{
		F::Backtrack.m_tRecord = { tTarget.m_pEntity->m_flSimulationTime(), I::GlobalVars->curtime, tTarget.m_pEntity->m_vecOrigin(), Vec3(), Vec3() };
		if (!tTarget.m_pEntity->SetupBones(F::Backtrack.m_tRecord.m_aBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, tTarget.m_pEntity->m_flSimulationTime()))
			return false;

		vRecords = { &F::Backtrack.m_tRecord };
	}

	bool bPeekCheck = false;
	if (Vars::Aimbot::Hitscan::PeekAmount.Value && pWeapon->GetWeaponSpread())
	{
		switch (Vars::Aimbot::Hitscan::PeekCheck.Value)
		{
		case Vars::Aimbot::Hitscan::PeekCheckEnum::Off: break;
		case Vars::Aimbot::Hitscan::PeekCheckEnum::DoubletapOnly: bPeekCheck = F::Ticks.GetTicks(pWeapon); break;
		case Vars::Aimbot::Hitscan::PeekCheckEnum::Always: bPeekCheck = true; break;
		}
	}
	Vec3 vPeekPos = bPeekCheck ? m_vEyePos + pLocal->m_vecVelocity() * TICKS_TO_TIME(-Vars::Aimbot::Hitscan::PeekAmount.Value) : Vec3();

	// if we're doubletapping, we can't change viewangles so work around that
	static int iTargetBone = 0;
	Vec3* pDoubletapAngle = F::Ticks.GetShootAngle();
	if (pDoubletapAngle && tTarget.m_iTargetType == TargetEnum::Player)
	{
		std::sort(vRecords.begin(), vRecords.end(), [&](const TickRecord* a, const TickRecord* b) -> bool
			{
				Vec3 vPosA = { a->m_aBones[iTargetBone][0][3], a->m_aBones[iTargetBone][1][3], a->m_aBones[iTargetBone][2][3] };
				Vec3 vPosB = { b->m_aBones[iTargetBone][0][3], b->m_aBones[iTargetBone][1][3], b->m_aBones[iTargetBone][2][3] };
				Vec3 vAnglesA = Math::CalcAngle(m_vEyePos, vPosA);
				Vec3 vAnglesB = Math::CalcAngle(m_vEyePos, vPosB);
				return pDoubletapAngle->DeltaAngle(vAnglesA).Length2D() < pDoubletapAngle->DeltaAngle(vAnglesB).Length2D();
			});
	}

	int iReturn = false;
	for (auto pRecord : vRecords)
	{
		bool bRunPeekCheck = bPeekCheck;

		if (pWeapon->GetWeaponID() == TF_WEAPON_LASER_POINTER)
		{
			tTarget.m_vPos = tTarget.m_pEntity->m_vecOrigin();

			// not lag compensated (i assume) so run movesim based on ping
			MoveStorage tStorage;
			F::MoveSim.Initialize(tTarget.m_pEntity, tStorage);
			if (!tStorage.m_bFailed)
			{
				for (int i = 1 - TIME_TO_TICKS(F::Backtrack.GetReal()); i <= 0; i++)
				{
					F::MoveSim.RunTick(tStorage);
					tTarget.m_vPos = tStorage.m_vPredictedOrigin;
				}
			}
			F::MoveSim.Restore(tStorage);

			float flBoneScale = std::max(Vars::Aimbot::Hitscan::BoneSizeMinimumScale.Value, Vars::Aimbot::Hitscan::MultipointScale.Value / 100.f);
			float flBoneSubtract = Vars::Aimbot::Hitscan::BoneSizeSubtract.Value;

			Vec3 vMins = tTarget.m_pEntity->m_vecMins();
			Vec3 vMaxs = tTarget.m_pEntity->m_vecMaxs();
			Vec3 vCheckMins = (vMins + flBoneSubtract) * flBoneScale;
			Vec3 vCheckMaxs = (vMaxs - flBoneSubtract) * flBoneScale;

			const matrix3x4 mTransform = { { 1, 0, 0, tTarget.m_vPos.x }, { 0, 1, 0, tTarget.m_vPos.y }, { 0, 0, 1, tTarget.m_vPos.z } };

			tTarget.m_vPos += tTarget.m_pEntity->GetOffset() / 2;
			if (m_vEyePos.DistToSqr(tTarget.m_vPos) > flMaxRange)
				break;

			if (SDK::VisPosWorld(pLocal, tTarget.m_pEntity, m_vEyePos, tTarget.m_vPos))
			{
				Vec3 vAngles; bool bChanged = Aim(G::CurrentUserCmd->viewangles, Math::CalcAngle(m_vEyePos, tTarget.m_vPos), vAngles);
				Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
				float flDist = m_vEyePos.DistTo(tTarget.m_vPos);

				if (!bChanged || Math::RayToOBB(m_vEyePos, vForward, vCheckMins, vCheckMaxs, mTransform) && SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, m_vEyePos + vForward * flDist))
				{
					tTarget.m_vAngleTo = vAngles;
					tTarget.m_pRecord = pRecord;
					return true;
				}
				else if (iReturn == 2 ? vAngles.DeltaAngle(G::CurrentUserCmd->viewangles).Length2D() < tTarget.m_vAngleTo.DeltaAngle(G::CurrentUserCmd->viewangles).Length2D() : true)
					tTarget.m_vAngleTo = vAngles;
				iReturn = 2;
			}

			break;
		}

		if (tTarget.m_iTargetType == TargetEnum::Player)
		{
			auto aBones = pRecord->m_aBones;
			if (!aBones)
				continue;

			// Predict server-side interpolation offset.
			// When the server lag-compensates, it interpolates between records.
			// We record bones at pRecord->m_vOrigin, but the server places the entity
			// at an interpolated origin between this record and the newer one.
			// The difference = undershoot that we need to compensate for.
			Vec3 vBoneOffset = {};
			std::vector<TickRecord*> vAllRecords;
			F::Backtrack.GetRecords(tTarget.m_pEntity, vAllRecords);

			int iRecordIdx = -1;
			for (int r = 0; r < vAllRecords.size(); r++)
			{
				if (vAllRecords[r] == pRecord)
				{
					iRecordIdx = r;
					break;
				}
			}

			// If there's a newer record, server may interpolate between them
			if (iRecordIdx > 0)
			{
				auto pNewerRecord = vAllRecords[iRecordIdx - 1];

				float flCorrect = F::Backtrack.GetServerCorrect();
				int iServerTick = F::Backtrack.GetServerTickCount();
				int iLerpTicks = TIME_TO_TICKS(F::Backtrack.GetFakeInterp());
				int iTickCount = TIME_TO_TICKS(pRecord->m_flSimTime) + iLerpTicks;
				float flTargetTime = TICKS_TO_TIME(iTickCount - iLerpTicks);

				// Server interpolates if targetTime falls between records
				if (pRecord->m_flSimTime < flTargetTime &&
					pRecord->m_flSimTime < pNewerRecord->m_flSimTime)
				{
					float flDenom = pNewerRecord->m_flSimTime - pRecord->m_flSimTime;
					if (flDenom > 0.0001f)
					{
						float frac = (flTargetTime - pRecord->m_flSimTime) / flDenom;
						frac = std::clamp(frac, 0.f, 1.f);

						Vec3 vInterpOrigin = pRecord->m_vOrigin + (pNewerRecord->m_vOrigin - pRecord->m_vOrigin) * frac;
						vBoneOffset = vInterpOrigin - pRecord->m_vOrigin;
					}
				}
			}

			// Shotgun center-of-mass aiming: compute average center of all torso hitboxes
			// This maximizes pellet coverage instead of aiming at a single hitbox center
			bool bIsShotgun = (pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PRIMARY ||
				pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_SOLDIER ||
				pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_HWG ||
				pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PYRO ||
				pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_BUILDING_RESCUE);

			if (bIsShotgun)
			{
				Vec3 vCombinedCenter = {};
				int iCount = 0;
				std::vector<int> vTorsoHitboxes;

				for (int nHitbox = 0; nHitbox < pSet->numhitboxes; nHitbox++)
				{
					if (!F::AimbotGlobal.IsHitboxValid(tTarget.m_pEntity, nHitbox, Vars::Aimbot::Hitscan::Hitboxes.Value))
						continue;
					auto pBox = pSet->pHitbox(nHitbox);
					if (!pBox) continue;

					int nBase = tTarget.m_pEntity->GetHitboxToBase(nHitbox);
					// Only count torso hitboxes for center-of-mass
					if (nBase != HITBOX_PELVIS && nBase != HITBOX_SPINE0 &&
						nBase != HITBOX_SPINE1 && nBase != HITBOX_SPINE2 && nBase != HITBOX_SPINE3)
						continue;

					vTorsoHitboxes.push_back(nHitbox);
					Vec3 vCenter;
					Math::VectorTransform((pBox->bbmin + pBox->bbmax) / 2, aBones[pBox->bone], vCenter);
					vCenter += vBoneOffset;
					vCombinedCenter += vCenter;
					iCount++;
				}

				if (iCount > 0)
				{
					vCombinedCenter /= (float)iCount;

					if (m_vEyePos.DistToSqr(vCombinedCenter) <= flMaxRange &&
						SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, vCombinedCenter))
					{
						Vec3 vAngles;
						Aim(G::CurrentUserCmd->viewangles, Math::CalcAngle(m_vEyePos, vCombinedCenter), vAngles);
						tTarget.m_vAngleTo = vAngles;
						tTarget.m_pRecord = pRecord;
						tTarget.m_vPos = vCombinedCenter;
						tTarget.m_nAimedHitbox = -1; // center of mass, not specific hitbox
						tTarget.m_vAimedHitboxes = vTorsoHitboxes;
						tTarget.m_bBacktrack = true;
						return true;
					}
				}
				// If center-of-mass is not visible, fall through to normal hitbox logic
			}

			std::vector<std::tuple<const mstudiobbox_t*, int, int>> vHitboxes;
			for (int nHitbox = 0; nHitbox < pSet->numhitboxes; nHitbox++)
			{
				int iPriority = GetHitboxPriority(nHitbox, pLocal, pWeapon, tTarget.m_pEntity);
				if (iPriority == -1)
					continue;

				auto pBox = pSet->pHitbox(nHitbox);
				if (!pBox) continue;

				vHitboxes.emplace_back(pBox, nHitbox, iPriority);
			}
			std::sort(vHitboxes.begin(), vHitboxes.end(), [&](const auto& a, const auto& b) -> bool
				{
					return std::get<2>(a) < std::get<2>(b);
				});

			float flModelScale = tTarget.m_pEntity->As<CBaseAnimating>()->m_flModelScale();
			float flBoneScale = Vars::Aimbot::Hitscan::BoneSizeMinimumScale.Value;
			float flBoneSubtract = Vars::Aimbot::Hitscan::BoneSizeSubtract.Value;

			auto pGameRules = I::TFGameRules();
			auto pViewVectors = pGameRules ? pGameRules->GetViewVectors() : nullptr;
			Vec3 vHullMins = (pViewVectors ? pViewVectors->m_vHullMin : Vec3(-24, -24, 0)) * flModelScale;
			Vec3 vHullMaxs = (pViewVectors ? pViewVectors->m_vHullMax : Vec3(24, 24, 82)) * flModelScale;

			const matrix3x4 mTransform = { { 1, 0, 0, pRecord->m_vOrigin.x }, { 0, 1, 0, pRecord->m_vOrigin.y }, { 0, 0, 1, pRecord->m_vOrigin.z } };

			for (auto& [pBox, nHitbox, _] : vHitboxes)
			{
				Vec3 vMins = pBox->bbmin;
				Vec3 vMaxs = pBox->bbmax;
				Vec3 vCheckMins = (vMins + flBoneSubtract / flModelScale) * flBoneScale;
				Vec3 vCheckMaxs = (vMaxs - flBoneSubtract / flModelScale) * flBoneScale;
				Vec3 vAngle; Math::MatrixAngles(aBones[pBox->bone], vAngle);

				Vec3 vOffset;
				{
					Vec3 vOrigin, vCenter;
					Math::VectorTransform({}, aBones[pBox->bone], vOrigin);
					Math::VectorTransform((vMins + vMaxs) / 2, aBones[pBox->bone], vCenter);
					vOffset = vCenter - vOrigin;
				}

				int nHitboxBase = tTarget.m_pEntity->GetHitboxToBase(nHitbox);
				bool bIsHead = (nHitboxBase == HITBOX_HEAD);

				// Shotguns: aim at center of mass (computed separately below)
				bool bIsShotgun = (pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PRIMARY ||
					pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_SOLDIER ||
					pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_HWG ||
					pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_PYRO ||
					pWeapon->GetWeaponID() == TF_WEAPON_SHOTGUN_BUILDING_RESCUE);

				std::vector<Vec3> vPoints = { Vec3() }; // center point always

				// Multipoint: generate points along the hitbox's LONGEST axis
				// Fixed: previous code always used Z axis, but different bones have different longest axes
				if (!bIsHead && !bIsShotgun && F::AimbotGlobal.ShouldMultipoint(tTarget.m_pEntity, nHitbox, Vars::Aimbot::Hitscan::MultipointHitboxes.Value))
				{
					flBoneScale = std::max(flBoneScale, Vars::Aimbot::Hitscan::MultipointScale.Value / 100.f);
					bool bTriggerbot = (Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Smooth
						|| Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Assistive)
						&& !Vars::Aimbot::General::AssistStrength.Value;

					if (!bTriggerbot)
					{
						// Find the longest axis of the hitbox in bone-local space
						Vec3 vSize = vMaxs - vMins;
						float flScale = 0.4f;

						Vec3 vPointA, vPointB;
						if (vSize.x >= vSize.y && vSize.x >= vSize.z)
						{
							// X is longest axis
							vPointA = Vec3(vSize.x * flScale, 0.f, 0.f);
							vPointB = Vec3(-vSize.x * flScale, 0.f, 0.f);
						}
						else if (vSize.y >= vSize.x && vSize.y >= vSize.z)
						{
							// Y is longest axis
							vPointA = Vec3(0.f, vSize.y * flScale, 0.f);
							vPointB = Vec3(0.f, -vSize.y * flScale, 0.f);
						}
						else
						{
							// Z is longest axis
							vPointA = Vec3(0.f, 0.f, vSize.z * flScale);
							vPointB = Vec3(0.f, 0.f, -vSize.z * flScale);
						}

						vPoints = { Vec3(), vPointA, vPointB };
					}
				}

				for (auto& vPoint : vPoints)
				{
					Vec3 vOrigin; Math::VectorTransform(vPoint, aBones[pBox->bone], vOrigin); vOrigin += vOffset; vOrigin += vBoneOffset;

					if (m_vEyePos.DistToSqr(vOrigin) > flMaxRange)
						continue;

					// x32-style: simpler peek check
					if (bRunPeekCheck)
					{
						bRunPeekCheck = false;
						if (!SDK::VisPos(pLocal, tTarget.m_pEntity, vPeekPos, vOrigin))
							goto nextTick;
					}

					// x32-style: simpler visibility check - just use VisPos with MASK_SHOT | CONTENTS_GRATE
					Vec3 vAngles; bool bChanged = Aim(G::CurrentUserCmd->viewangles, Math::CalcAngle(m_vEyePos, vOrigin), vAngles);

					if (bChanged || SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, vOrigin))
					{
						iTargetBone = pBox->bone;

						tTarget.m_vAngleTo = vAngles;
						tTarget.m_pRecord = pRecord;
						tTarget.m_vPos = vOrigin;
						tTarget.m_nAimedHitbox = nHitbox;
						tTarget.m_vAimedHitboxes = { nHitbox };
						tTarget.m_bBacktrack = true;
						return true;
					}
					else if (bChanged && SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, vOrigin))
					{
						if (iReturn != 2 || vAngles.DeltaAngle(G::CurrentUserCmd->viewangles).Length2D() < tTarget.m_vAngleTo.DeltaAngle(G::CurrentUserCmd->viewangles).Length2D())
							tTarget.m_vAngleTo = vAngles;
						iReturn = 2;
					}
				}
			}
		}
		else
		{
			float flBoneScale = std::max(Vars::Aimbot::Hitscan::BoneSizeMinimumScale.Value, Vars::Aimbot::Hitscan::MultipointScale.Value / 100.f);
			float flBoneSubtract = Vars::Aimbot::Hitscan::BoneSizeSubtract.Value;

			Vec3 vMins = tTarget.m_pEntity->m_vecMins();
			Vec3 vMaxs = tTarget.m_pEntity->m_vecMaxs();
			Vec3 vCheckMins = (vMins + flBoneSubtract) * flBoneScale;
			Vec3 vCheckMaxs = (vMaxs - flBoneSubtract) * flBoneScale;

			const matrix3x4& mTransform = tTarget.m_pEntity->m_Collision()->CollisionToWorldTransform();

			std::vector<Vec3> vPoints = { Vec3() };
			//if (Vars::Aimbot::Hitscan::MultipointScale.Value > 0.f)
			{
				bool bTriggerbot = (Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Smooth
					|| Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Assistive)
					&& !Vars::Aimbot::General::AssistStrength.Value;

				if (!bTriggerbot)
				{
					float flScale = 0.5f; //Vars::Aimbot::Hitscan::MultipointScale.Value / 100;
					Vec3 vMinsS = (vMins - vMaxs) / 2 * flScale;
					Vec3 vMaxsS = (vMaxs - vMins) / 2 * flScale;

					vPoints = {
						Vec3(),
						Vec3(vMinsS.x, vMinsS.y, vMaxsS.z),
						Vec3(vMaxsS.x, vMinsS.y, vMaxsS.z),
						Vec3(vMinsS.x, vMaxsS.y, vMaxsS.z),
						Vec3(vMaxsS.x, vMaxsS.y, vMaxsS.z),
						Vec3(vMinsS.x, vMinsS.y, vMinsS.z),
						Vec3(vMaxsS.x, vMinsS.y, vMinsS.z),
						Vec3(vMinsS.x, vMaxsS.y, vMinsS.z),
						Vec3(vMaxsS.x, vMaxsS.y, vMinsS.z)
					};
				}
			}

			for (auto& vPoint : vPoints)
			{
				Vec3 vOrigin = tTarget.m_pEntity->GetCenter() + vPoint;

				if (m_vEyePos.DistToSqr(vOrigin) > flMaxRange)
					continue;

				Vec3 vAngles; bool bChanged = Aim(G::CurrentUserCmd->viewangles, Math::CalcAngle(m_vEyePos, vOrigin), vAngles);
				Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
				float flDist = m_vEyePos.DistTo(vOrigin);

				if (bChanged || SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, vOrigin))
				{
					if (!bChanged || Math::RayToOBB(m_vEyePos, vForward, vCheckMins, vCheckMaxs, mTransform) && SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, m_vEyePos + vForward * flDist))
					{
						tTarget.m_vAngleTo = vAngles;
						tTarget.m_pRecord = pRecord;
						tTarget.m_vPos = vOrigin;
						return true;
					}
					else if (bChanged && SDK::VisPos(pLocal, tTarget.m_pEntity, m_vEyePos, vOrigin))
					{
						if (iReturn != 2 || vAngles.DeltaAngle(G::CurrentUserCmd->viewangles).Length2D() < tTarget.m_vAngleTo.DeltaAngle(G::CurrentUserCmd->viewangles).Length2D())
							tTarget.m_vAngleTo = vAngles;
						iReturn = 2;
					}
				}
			}
		}

	nextTick:
		continue;
	}

	return iReturn;
}



bool CAimbotHitscan::ShouldFire(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, const Target_t& tTarget)
{
	if (!Vars::Aimbot::General::AutoShoot.Value)
		return false;

	if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::WaitForHeadshot
		&& tTarget.m_pEntity->IsPlayer())
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
			if (!G::CanHeadshot && pLocal->InCond(TF_COND_AIMING) && pWeapon->As<CTFSniperRifle>()->GetRifleType() != RIFLE_JARATE)
				return false;
			break;
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			if (!G::CanHeadshot)
				return false;
			break;
		case TF_WEAPON_REVOLVER:
			if (SDK::AttribHookValue(0, "set_weapon_mode", pWeapon) == 1 && !pWeapon->AmbassadorCanHeadshot())
				return false;
		}
	}

	if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::WaitForCharge)
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_SNIPERRIFLE_DECAP:
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		{
			auto pSniperRifle = pWeapon->As<CTFSniperRifle>();
			if (!pLocal->InCond(TF_COND_AIMING) || pSniperRifle->m_flChargedDamage() == 150.f)
				break;

			if (tTarget.m_pEntity->IsPlayer())
			{
				auto pPlayer = tTarget.m_pEntity->As<CTFPlayer>();
				if (tTarget.m_nAimedHitbox == HITBOX_HEAD && (pWeapon->GetWeaponID() != TF_WEAPON_SNIPERRIFLE_CLASSIC || pSniperRifle->m_flChargedDamage() == 150.f))
				{
					int iDamage = std::ceil(std::max(pSniperRifle->m_flChargedDamage(), 50.f) * pSniperRifle->GetHeadshotMult(pPlayer));
					if (pPlayer->m_iHealth() <= iDamage && (G::CanHeadshot || pLocal->IsCritBoosted()))
						break;
				}
				else
				{
					int iDamage = std::ceil(std::max(pSniperRifle->m_flChargedDamage(), 50.f) * pSniperRifle->GetBodyshotMult(pPlayer));
					if (pPlayer->m_iHealth() <= iDamage)
						break;
				}
			}
			else if (tTarget.m_pEntity->IsBuilding())
			{
				auto pBuilding = tTarget.m_pEntity->As<CBaseObject>();
				int iDamage = std::ceil(std::max(pSniperRifle->m_flChargedDamage(), 50.f));
				if (pBuilding->m_iHealth() <= iDamage)
					break;
			}
			else
				break;

			return false;
		}
		}
	}

	return true;
}

bool CAimbotHitscan::Aim(Vec3 vCurAngle, Vec3 vToAngle, Vec3& vOut, int iMethod)
{
	auto pLocal = H::Entities.GetLocal();
	Vec3 vPunch = pLocal ? pLocal->m_vecPunchAngle() : Vec3();

	if (Vec3* pDoubletapAngle = F::Ticks.GetShootAngle())
	{
		vOut = *pDoubletapAngle - vPunch;
		return true;
	}

	bool bReturn = false;
	vToAngle -= vPunch;
	switch (iMethod)
	{
	case Vars::Aimbot::General::AimTypeEnum::Plain:
	case Vars::Aimbot::General::AimTypeEnum::Silent:
	case Vars::Aimbot::General::AimTypeEnum::Locking:
		vOut = vToAngle;
		break;
	case Vars::Aimbot::General::AimTypeEnum::Smooth:
		vOut = vCurAngle.LerpAngle(vToAngle, Vars::Aimbot::General::AssistStrength.Value / 100.f);
		bReturn = true;
		break;
	case Vars::Aimbot::General::AimTypeEnum::Assistive:
		Vec3 vMouseDelta = G::CurrentUserCmd->viewangles.DeltaAngle(G::LastUserCmd->viewangles);
		Vec3 vTargetDelta = vToAngle.DeltaAngle(G::LastUserCmd->viewangles);
		float flMouseDelta = vMouseDelta.Length2D(), flTargetDelta = vTargetDelta.Length2D();
		vTargetDelta = vTargetDelta.Normalized() * std::min(flMouseDelta, flTargetDelta);
		vOut = vCurAngle - vMouseDelta + vMouseDelta.LerpAngle(vTargetDelta, Vars::Aimbot::General::AssistStrength.Value / 100.f);
		bReturn = true;
		break;
	}

	Math::ClampAngles(vOut);
	return bReturn;
}

// assume angle calculated outside with other overload
void CAimbotHitscan::Aim(CUserCmd* pCmd, Vec3& vAngle, int iMethod)
{
	bool bUnsure = F::Ticks.IsTimingUnsure();
	switch (iMethod)
	{
	case Vars::Aimbot::General::AimTypeEnum::Plain:
		if (G::Attacking != 1 && !bUnsure)
			break;
		[[fallthrough]];
	case Vars::Aimbot::General::AimTypeEnum::Smooth:
	case Vars::Aimbot::General::AimTypeEnum::Assistive:
		pCmd->viewangles = vAngle;
		I::EngineClient->SetViewAngles(vAngle);
		break;
	case Vars::Aimbot::General::AimTypeEnum::Silent:
		if (G::Attacking == 1 || bUnsure)
		{
			SDK::FixMovement(pCmd, vAngle);
			pCmd->viewangles = vAngle;
			G::SilentAngles = true;
		}
		break;
	case Vars::Aimbot::General::AimTypeEnum::Locking:
		SDK::FixMovement(pCmd, vAngle);
		pCmd->viewangles = vAngle;
		G::SilentAngles = true;
	}
}

static inline void DrawVisuals(CTFPlayer* pLocal, Target_t& tTarget, int nWeaponID)
{
	if (G::Attacking == 1 && nWeaponID != TF_WEAPON_LASER_POINTER)
	{
		bool bLine = Vars::Visuals::Line::Enabled.Value;
		bool bBoxes = Vars::Visuals::Hitbox::BonesEnabled.Value & Vars::Visuals::Hitbox::BonesEnabledEnum::OnShot;
		if (G::CanPrimaryAttack && (bLine || bBoxes))
		{
			G::LineStorage.clear();
			G::BoxStorage.clear();
			G::PathStorage.clear();

			if (bLine)
			{
				Vec3 vEyePos = pLocal->GetShootPos();
				float flDist = vEyePos.DistTo(tTarget.m_vPos);
				Vec3 vForward; Math::AngleVectors(tTarget.m_vAngleTo + pLocal->m_vecPunchAngle(), &vForward);

				if (Vars::Colors::LineIgnoreZ.Value.a)
					G::LineStorage.emplace_back(std::pair<Vec3, Vec3>(vEyePos, vEyePos + vForward * flDist), I::GlobalVars->curtime + Vars::Visuals::Line::DrawDuration.Value, Vars::Colors::LineIgnoreZ.Value);
				if (Vars::Colors::Line.Value.a)
					G::LineStorage.emplace_back(std::pair<Vec3, Vec3>(vEyePos, vEyePos + vForward * flDist), I::GlobalVars->curtime + Vars::Visuals::Line::DrawDuration.Value, Vars::Colors::Line.Value, true);
			}
			if (bBoxes)
			{
				auto vBoxes = F::Visuals.GetHitboxes(tTarget.m_pRecord->m_aBones, tTarget.m_pEntity->As<CBaseAnimating>(), {}, tTarget.m_vAimedHitboxes);

				// Check if fade is enabled
				if (Vars::Colors::HitboxFade.Value)
				{
					// Store in fade storage for on-shot hitboxes
					for (auto& tBox : vBoxes)
					{
						F::Visuals.AddFadeHitbox(tBox.m_vOrigin, tBox.m_vMins, tBox.m_vMaxs, tBox.m_vAngles,
							tBox.m_flTime, tBox.m_tColorEdge, tBox.m_tColorFace, tBox.m_bZBuffer);
					}
				}
				else
				{
					// Regular storage
					G::BoxStorage.insert(G::BoxStorage.end(), vBoxes.begin(), vBoxes.end());
				}
			}
		}
		// Register shoot-time bones for the on-hit hitbox drawing feature.
		// Event() will consume this when player_hurt fires.
		if (tTarget.m_iTargetType == TargetEnum::Player
			&& tTarget.m_pRecord
			&& tTarget.m_pRecord->m_aBones)
		{
			F::Visuals.RegisterPendingHit(
				tTarget.m_pEntity->entindex(),
				tTarget.m_pRecord->m_aBones,
				tTarget.m_nAimedHitbox,
				tTarget.m_vAimedHitboxes);
		}
	}
}

void CAimbotHitscan::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	const int nWeaponID = pWeapon->GetWeaponID();

	static int iStaticAimType = Vars::Aimbot::General::AimType.Value;
	const int iLastAimType = iStaticAimType;
	const int iRealAimType = Vars::Aimbot::General::AimType.Value;

	switch (nWeaponID)
	{
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		if (G::Attacking && !iRealAimType && iLastAimType)
			Vars::Aimbot::General::AimType.Value = iLastAimType;
	}
	iStaticAimType = Vars::Aimbot::General::AimType.Value;

	if (F::AimbotGlobal.ShouldHoldAttack(pWeapon))
		pCmd->buttons |= IN_ATTACK;
	if (!Vars::Aimbot::General::AimType.Value
		|| !F::AimbotGlobal.ShouldAim() && (nWeaponID != TF_WEAPON_MINIGUN || pWeapon->As<CTFMinigun>()->m_iWeaponState() == AC_STATE_FIRING || pWeapon->As<CTFMinigun>()->m_iWeaponState() == AC_STATE_SPINNING))
		return;

	switch (nWeaponID)
	{
	case TF_WEAPON_MINIGUN:
		if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::AutoRev)
			pCmd->buttons |= IN_ATTACK2;
		if (pWeapon->As<CTFMinigun>()->m_iWeaponState() != AC_STATE_FIRING && pWeapon->As<CTFMinigun>()->m_iWeaponState() != AC_STATE_SPINNING)
			return;
		break;
	}

	auto vTargets = SortTargets(pLocal, pWeapon);
	if (vTargets.empty())
		return;

	switch (nWeaponID)
	{
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	{
		bool bScoped = pLocal->InCond(TF_COND_ZOOMED);
		if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::AutoScope && !bScoped)
		{
			pCmd->buttons |= IN_ATTACK2;
			return;
		}
		else if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::ScopedOnly && !bScoped)
			return;
		else if (!bScoped && SDK::AttribHookValue(0, "sniper_only_fire_zoomed", pWeapon))
			return;
		break;
	}
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
		if (iRealAimType)
			pCmd->buttons |= IN_ATTACK;
	}

	if (!G::AimTarget.m_iEntIndex)
		G::AimTarget = { vTargets.front().m_pEntity->entindex(), I::GlobalVars->tickcount, 0 };

	for (auto& tTarget : vTargets)
	{
		if (nWeaponID == TF_WEAPON_MEDIGUN && pWeapon->As<CWeaponMedigun>()->m_hHealingTarget().Get() == tTarget.m_pEntity)
		{
			if (G::LastUserCmd->buttons & IN_ATTACK)
				pCmd->buttons |= IN_ATTACK;
			return;
		}

		const auto iResult = CanHit(tTarget, pLocal, pWeapon);
		if (!iResult) continue;
		if (iResult == 2)
		{
			G::AimTarget = { tTarget.m_pEntity->entindex(), I::GlobalVars->tickcount, 0 };
			Aim(pCmd, tTarget.m_vAngleTo);
			break;
		}

		G::AimTarget = { tTarget.m_pEntity->entindex(), I::GlobalVars->tickcount };
		G::AimPoint = { tTarget.m_vPos, I::GlobalVars->tickcount };

		if (ShouldFire(pLocal, pWeapon, pCmd, tTarget))
		{
			switch (nWeaponID)
			{
			case TF_WEAPON_MEDIGUN:
				if (!(G::LastUserCmd->buttons & IN_ATTACK))
					pCmd->buttons |= IN_ATTACK;
				break;
			case TF_WEAPON_SNIPERRIFLE_CLASSIC:
				if (pWeapon->As<CTFSniperRifle>()->m_flChargedDamage() && pLocal->m_hGroundEntity())
					pCmd->buttons &= ~IN_ATTACK;
				break;
			case TF_WEAPON_LASER_POINTER:
				pCmd->buttons |= IN_ATTACK | IN_ATTACK2;
				break;
			default:
				pCmd->buttons |= IN_ATTACK;
			}

			if (Vars::Aimbot::Hitscan::Modifiers.Value & Vars::Aimbot::Hitscan::ModifiersEnum::Tapfire && pWeapon->GetWeaponSpread() != 0.f && !pLocal->InCond(TF_COND_RUNE_PRECISION)
				&& m_vEyePos.DistTo(tTarget.m_vPos) > Vars::Aimbot::Hitscan::TapfireDistance.Value)
			{
				const float flTimeSinceLastShot = (pLocal->m_nTickBase() * TICK_INTERVAL) - pWeapon->m_flLastFireTime();
				if (flTimeSinceLastShot <= (pWeapon->GetBulletsPerShot() > 1 ? 0.25f : 1.25f))
					pCmd->buttons &= ~IN_ATTACK;
			}
		}

		G::Attacking = SDK::IsAttacking(pLocal, pWeapon, pCmd, true);
		if (G::Attacking == 1 && nWeaponID != TF_WEAPON_LASER_POINTER)
		{
			if (tTarget.m_pEntity->IsPlayer())
				F::Resolver.HitscanRan(pLocal, tTarget.m_pEntity->As<CTFPlayer>(), pWeapon, tTarget.m_nAimedHitbox);

			if (tTarget.m_bBacktrack)
				pCmd->tick_count = TIME_TO_TICKS(tTarget.m_pRecord->m_flSimTime) + TIME_TO_TICKS(F::Backtrack.GetFakeInterp());
		}
		DrawVisuals(pLocal, tTarget, nWeaponID);

		Aim(pCmd, tTarget.m_vAngleTo);
		if (G::SilentAngles)
		{
			switch (nWeaponID)
			{
			case TF_WEAPON_MEDIGUN:
				//case TF_WEAPON_LASER_POINTER: // we can psilent with the wrangler though probably with some hacks
				G::SilentAngles = false, G::PSilentAngles = true;
			}
		}
		break;
	}
}