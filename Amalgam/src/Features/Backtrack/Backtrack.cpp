#include "Backtrack.h"

#include "../PacketManip/FakeLag/FakeLag.h"
#include "../Ticks/Ticks.h"
#include "../Aimbot/Aimbot.h"

void CBacktrack::Reset()
{
	m_mRecords.clear();
	m_dSequences.clear();
	m_iLastInSequence = 0;
}

float CBacktrack::GetReal(int iFlow, bool bNoFake)
{
	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (!pNetChan)
		return 0.f;

	if (iFlow != MAX_FLOWS)
		return pNetChan->GetLatency(iFlow) - (bNoFake && iFlow == FLOW_INCOMING ? GetFakeLatency() : 0.f);
	return pNetChan->GetLatency(FLOW_INCOMING) + pNetChan->GetLatency(FLOW_OUTGOING) - (bNoFake ? GetFakeLatency() : 0.f);
}

float CBacktrack::GetWishFake()
{
	return std::clamp(Vars::Backtrack::Latency.Value / 1000.f, 0.f, m_flMaxUnlag);
}

float CBacktrack::GetWishLerp()
{
	if (Vars::Misc::Game::AntiCheatCompatibility.Value)
		return std::clamp(Vars::Backtrack::Interp.Value / 1000.f, G::Lerp, 0.1f);

	return std::clamp(Vars::Backtrack::Interp.Value / 1000.f, G::Lerp, m_flMaxUnlag);
}

float CBacktrack::GetFakeLatency()
{
	return m_flFakeLatency;
}

float CBacktrack::GetFakeInterp()
{
	if (Vars::Misc::Game::AntiCheatCompatibility.Value)
		return std::min(m_flFakeInterp, 0.1f);

	return m_flFakeInterp;
}

float CBacktrack::GetWindow()
{
	return Vars::Backtrack::Window.Value / 1000.f;
}

int CBacktrack::GetAnticipatedChoke(int iMethod)
{
	int iAnticipatedChoke = 0;
	if (F::Ticks.CanChoke() && G::PrimaryWeaponType != EWeaponType::HITSCAN && Vars::Aimbot::General::AimType.Value == Vars::Aimbot::General::AimTypeEnum::Silent)
		iAnticipatedChoke = 1;
	if (F::FakeLag.m_iGoal && !Vars::Fakelag::UnchokeOnAttack.Value && F::Ticks.m_iShiftedTicks == F::Ticks.m_iShiftedGoal && !F::Ticks.m_bDoubletap)
		iAnticipatedChoke = F::FakeLag.m_iGoal - I::ClientState->chokedcommands;
	return iAnticipatedChoke;
}

float CBacktrack::GetServerCorrect()
{
	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (!pNetChan)
		return 0.f;

	float flCorrect = pNetChan->GetLatency(FLOW_INCOMING);
	int iLerpTicks = TIME_TO_TICKS(GetFakeInterp());
	flCorrect += TICKS_TO_TIME(iLerpTicks);
	flCorrect = std::clamp(flCorrect, 0.f, m_flMaxUnlag);

	return flCorrect;
}

int CBacktrack::GetServerTickCount()
{
	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (!pNetChan)
		return GetClientTickCount();

	float flOutgoing = pNetChan->GetLatency(FLOW_OUTGOING);

	// Server receives packet and processes it in the NEXT server frame.
	// This adds ~1 tick of processing delay.
	int iProcessingTicks = 1;

	return GetClientTickCount() + TIME_TO_TICKS(flOutgoing) + GetAnticipatedChoke() + iProcessingTicks;
}

bool CBacktrack::IsTickValid(float flSimTime)
{
	float flCorrect = GetServerCorrect();
	int iLerpTicks = TIME_TO_TICKS(GetFakeInterp());
	int iTickCount = TIME_TO_TICKS(flSimTime) + iLerpTicks;
	int iTargetTick = iTickCount - iLerpTicks;
	int iServerTick = GetServerTickCount();
	float flDeltaTime = flCorrect - TICKS_TO_TIME(iServerTick - iTargetTick);
	return std::fabs(flDeltaTime) <= 0.2f;
}

float CBacktrack::GetIdealSimTime()
{
	float flCorrect = GetServerCorrect();
	int iServerTick = GetServerTickCount();
	return TICKS_TO_TIME(iServerTick) - flCorrect;
}

std::vector<TickRecord*> CBacktrack::GetValidRecords(std::vector<TickRecord*>& vRecords, CTFPlayer* pLocal, bool bDistance, float flTimeMod)
{
	if (vRecords.empty())
		return {};

	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (!pNetChan)
		return {};

	std::vector<TickRecord*> vReturn = {};
	float flCorrect = std::clamp(GetReal(MAX_FLOWS, false) + ROUND_TO_TICKS(GetFakeInterp()), 0.f, m_flMaxUnlag);
	int iServerTick = GetClientTickCount() + GetAnticipatedChoke() + Vars::Backtrack::Offset.Value + TIME_TO_TICKS(GetReal(FLOW_OUTGOING));

	if (!Vars::Misc::Game::AntiCheatCompatibility.Value && GetWindow())
	{
		for (auto pRecord : vRecords)
		{
			float flDelta = fabsf(flCorrect - TICKS_TO_TIME(iServerTick - TIME_TO_TICKS(pRecord->m_flSimTime + flTimeMod)));
			if (flDelta > GetWindow())
				continue;

			vReturn.push_back(pRecord);
		}
	}

	if (vReturn.empty())
	{	// make sure there is at least 1 record
		float flMinDelta = 0.2f;
		for (auto pRecord : vRecords)
		{
			float flDelta = fabsf(flCorrect - TICKS_TO_TIME(iServerTick - TIME_TO_TICKS(pRecord->m_flSimTime + flTimeMod)));
			if (flDelta > flMinDelta)
				continue;

			flMinDelta = flDelta;
			vReturn = { pRecord };
		}
	}
	else if (pLocal && vReturn.size() > 1)
	{
		if (bDistance)
			std::sort(vReturn.begin(), vReturn.end(), [&](const TickRecord* a, const TickRecord* b) -> bool
				{
					if (Vars::Backtrack::PreferOnShot.Value && a->m_bOnShot != b->m_bOnShot)
						return a->m_bOnShot > b->m_bOnShot;

					return pLocal->m_vecOrigin().DistTo(a->m_vOrigin) < pLocal->m_vecOrigin().DistTo(b->m_vOrigin);
				});
		else
		{
			std::sort(vReturn.begin(), vReturn.end(), [&](const TickRecord* a, const TickRecord* b) -> bool
				{
					if (Vars::Backtrack::PreferOnShot.Value && a->m_bOnShot != b->m_bOnShot)
						return a->m_bOnShot > b->m_bOnShot;

					const float flADelta = flCorrect - TICKS_TO_TIME(iServerTick - TIME_TO_TICKS(a->m_flSimTime + flTimeMod));
					const float flBDelta = flCorrect - TICKS_TO_TIME(iServerTick - TIME_TO_TICKS(b->m_flSimTime + flTimeMod));
					return fabsf(flADelta) < fabsf(flBDelta);
				});
		}
	}

	return vReturn;
}

void CBacktrack::CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (F::Aimbot.m_bRan || Vars::Misc::Game::AntiCheatCompatibility.Value)
		return;

	int iLerpTicks = TIME_TO_TICKS(GetFakeInterp());
	int iDefaultLerpTicks = TIME_TO_TICKS(G::Lerp);

	pCmd->tick_count += iLerpTicks;
	if (!Vars::Visuals::Removals::Lerp.Value && !Vars::Visuals::Removals::Interpolation.Value)
		pCmd->tick_count -= iDefaultLerpTicks;

	BacktrackToCrosshair(pLocal, pWeapon, pCmd);
}

void CBacktrack::SendLerp()
{
	static Timer tTimer = {};
	if (!tTimer.Run(0.1f))
		return;

	// With cl_interpolate=0, server sets m_fLerpTime = 0.
	// We must reflect this in our calculations.
	m_flSentInterp = 0.f;
	m_flFakeInterp = 0.f;

	auto pNetChan = reinterpret_cast<CNetChannel*>(I::EngineClient->GetNetChannelInfo());
	if (pNetChan && I::EngineClient->IsConnected())
	{
		NET_SetConVar tConvar1 = { "cl_interp", "0" };
		pNetChan->SendNetMsg(tConvar1);
		NET_SetConVar tConvar2 = { "cl_interp_ratio", "1" };
		pNetChan->SendNetMsg(tConvar2);
		NET_SetConVar tConvar3 = { "cl_interpolate", "0" };
		pNetChan->SendNetMsg(tConvar3);
	}
}

void CBacktrack::SetLerp(IGameEvent* pEvent)
{
	// With cl_interpolate=0, server always uses m_fLerpTime=0.
	// No need to wait for player_spawn event.
	m_flFakeInterp = 0.f;
}

void CBacktrack::UpdateDatagram()
{
	auto pNetChan = reinterpret_cast<CNetChannel*>(I::EngineClient->GetNetChannelInfo());
	if (!pNetChan)
		return;

	if (auto pLocal = H::Entities.GetLocal())
		m_nOldTickBase = pLocal->m_nTickBase();

	if (pNetChan->m_nInSequenceNr > m_iLastInSequence)
	{
		m_iLastInSequence = pNetChan->m_nInSequenceNr;
		m_dSequences.emplace_front(pNetChan->m_nInReliableState, pNetChan->m_nInSequenceNr, I::GlobalVars->realtime);
	}

	if (m_dSequences.size() > 67)
		m_dSequences.pop_back();
}

bool CBacktrack::GetRecords(CBaseEntity* pEntity, std::vector<TickRecord*>& vReturn)
{
	if (m_mRecords.find(pEntity) == m_mRecords.end())
		return false;

	auto& vRecords = m_mRecords[pEntity];
	for (auto& tRecord : vRecords)
		vReturn.push_back(&tRecord);
	return true;
}

matrix3x4* CBacktrack::GetBones(CBaseEntity* pEntity)
{
	std::vector<TickRecord*> vRecords = {};
	if (F::Backtrack.GetRecords(pEntity, vRecords) && !vRecords.empty())
		return vRecords.front()->m_aBones;
	return nullptr;
}

std::vector<HitboxInfo_t>* CBacktrack::GetHitboxInfos(CBaseEntity* pEntity)
{
	std::vector<TickRecord*> vRecords = {};
	if (F::Backtrack.GetRecords(pEntity, vRecords) && !vRecords.empty() && !vRecords.front()->m_vHitboxInfos.empty())
		return &vRecords.front()->m_vHitboxInfos;
	return nullptr;
}

void CBacktrack::MakeRecords()
{
	for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (pPlayer->entindex() == I::EngineClient->GetLocalPlayer() || !pPlayer->IsAlive() || pPlayer->IsAGhost()
			|| !H::Entities.GetDeltaTime(pPlayer->entindex()))
			continue;

		auto& vRecords = m_mRecords[pPlayer];
		TickRecord* pLastRecord = !vRecords.empty() ? &vRecords.front() : nullptr;

		vRecords.push_front({});
		TickRecord& tCurRecord = vRecords.front();
		tCurRecord.m_flSimTime = pPlayer->m_flSimulationTime();
		tCurRecord.m_flArriveTime = I::GlobalVars->curtime;
		tCurRecord.m_vOrigin = pPlayer->m_vecOrigin();
		tCurRecord.m_vMins = pPlayer->m_vecMins();
		tCurRecord.m_vMaxs = pPlayer->m_vecMaxs();
		tCurRecord.m_bOnShot = m_mDidShoot[pPlayer->entindex()];

		// Velocity from position delta with blending for high ping
		if (pLastRecord)
		{
			float flTimeDelta = tCurRecord.m_flSimTime - pLastRecord->m_flSimTime;
			if (flTimeDelta > 0.f)
			{
				Vec3 vDeltaVelocity = (tCurRecord.m_vOrigin - pLastRecord->m_vOrigin) / flTimeDelta;

				// At high ping, blend delta velocity with previous velocity for stability
				int iTicks = TIME_TO_TICKS(flTimeDelta);
				if (iTicks > 3)
				{
					float flBlendFactor = std::clamp(3.f / iTicks, 0.3f, 1.f);
					tCurRecord.m_vVelocity = vDeltaVelocity * flBlendFactor + pLastRecord->m_vVelocity * (1.f - flBlendFactor);
				}
				else
				{
					tCurRecord.m_vVelocity = vDeltaVelocity;
				}
			}
			else
			{
				tCurRecord.m_vVelocity = pLastRecord->m_vVelocity;
			}
		}
		else
		{
			tCurRecord.m_vVelocity = pPlayer->m_vecVelocity();
		}

		pPlayer->InvalidateBoneCache();
		m_bSettingUpBones = true;
		bool bSetup = pPlayer->SetupBones(tCurRecord.m_aBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, tCurRecord.m_flSimTime);
		m_bSettingUpBones = false;
		if (!bSetup)
		{
			vRecords.pop_front();
			continue;
		}

		bool bLagComp = false;
		if (pLastRecord)
		{
			const Vec3 vDelta = tCurRecord.m_vOrigin - pLastRecord->m_vOrigin;
			static auto sv_lagcompensation_teleport_dist = H::ConVars.FindVar("sv_lagcompensation_teleport_dist");
			const float flDist = powf(sv_lagcompensation_teleport_dist->GetFloat(), 2.f);
			if (vDelta.Length2DSqr() > flDist)
			{
				bLagComp = true;
				if (!H::Entities.GetLagCompensation(pPlayer->entindex()))
					vRecords.resize(1);
				for (size_t i = 1; i < vRecords.size(); i++)
					vRecords[i].m_bInvalid = true;
			}

			// Copy new record data to invalid records so there's always usable bone data
			for (auto& tRecord : vRecords)
			{
				if (!tRecord.m_bInvalid)
					continue;

				tRecord.m_vOrigin = tCurRecord.m_vOrigin;
				tRecord.m_vMins = tCurRecord.m_vMins;
				tRecord.m_vMaxs = tCurRecord.m_vMaxs;
				tRecord.m_bOnShot = tCurRecord.m_bOnShot;
				memcpy(tRecord.m_aBones, tCurRecord.m_aBones, sizeof(tRecord.m_aBones));
			}
		}

		while (vRecords.size() > 72)
			vRecords.pop_back();

		H::Entities.SetLagCompensation(pPlayer->entindex(), bLagComp);
		m_mDidShoot[pPlayer->entindex()] = false;
	}
}

void CBacktrack::CleanRecords()
{
	for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (pPlayer->entindex() == I::EngineClient->GetLocalPlayer())
			continue;

		auto& vRecords = m_mRecords[pPlayer];
		if (!pPlayer->IsAlive() || pPlayer->IsAGhost())
		{
			vRecords.clear();
			continue;
		}

		const float flDeadtime = I::GlobalVars->curtime + GetReal() - m_flMaxUnlag;
		while (!vRecords.empty())
		{
			if (vRecords.back().m_flSimTime < flDeadtime)
				vRecords.pop_back();
			else
				break;
		}
	}
}

void CBacktrack::Store()
{
	UpdateDatagram();
	if (!I::EngineClient->IsInGame())
		return;

	static auto sv_maxunlag = H::ConVars.FindVar("sv_maxunlag");
	m_flMaxUnlag = sv_maxunlag->GetFloat();

	MakeRecords();
	CleanRecords();
}

void CBacktrack::ResolverUpdate(CBaseEntity* pEntity) {}

void CBacktrack::ReportShot(int iIndex)
{
	if (!Vars::Backtrack::PreferOnShot.Value)
		return;

	auto pEntity = I::ClientEntityList->GetClientEntity(iIndex);
	if (!pEntity || SDK::GetWeaponType(pEntity->As<CTFPlayer>()->m_hActiveWeapon()->As<CTFWeaponBase>()) != EWeaponType::HITSCAN)
		return;

	m_mDidShoot[pEntity->entindex()] = true;
}

void CBacktrack::AdjustPing(CNetChannel* pNetChan)
{
	m_nOldInSequenceNr = pNetChan->m_nInSequenceNr, m_nOldInReliableState = pNetChan->m_nInReliableState;

	auto Set = [&]()
		{
			if (!Vars::Backtrack::Latency.Value)
				return 0.f;

			auto pLocal = H::Entities.GetLocal();
			if (!pLocal || !pLocal->m_iClass())
				return 0.f;

			static auto host_timescale = H::ConVars.FindVar("host_timescale");
			float flTimescale = host_timescale->GetFloat();

			static float flStaticReal = 0.f;
			float flFake = GetWishFake(), flReal = TICKS_TO_TIME(pLocal->m_nTickBase() - m_nOldTickBase);
			flStaticReal += (flReal + 5 * TICK_INTERVAL - flStaticReal) * 0.1f;

			int nInReliableState = pNetChan->m_nInReliableState, nInSequenceNr = pNetChan->m_nInSequenceNr; float flLatency = 0.f;
			for (auto& cSequence : m_dSequences)
			{
				nInReliableState = cSequence.m_nInReliableState;
				nInSequenceNr = cSequence.m_nSequenceNr;
				flLatency = (I::GlobalVars->realtime - cSequence.m_flTime) * flTimescale - TICK_INTERVAL;

				if (flLatency > flFake || m_nLastInSequenceNr >= cSequence.m_nSequenceNr || flLatency > m_flMaxUnlag - flStaticReal)
					break;
			}
			if (flLatency > 1.f)
				return 0.f;

			pNetChan->m_nInReliableState = nInReliableState;
			pNetChan->m_nInSequenceNr = nInSequenceNr;
			return flLatency;
		};

	auto flLatency = Set();
	m_nLastInSequenceNr = pNetChan->m_nInSequenceNr;

	if (Vars::Backtrack::Latency.Value || m_flFakeLatency)
	{
		m_flFakeLatency = std::clamp(
			m_flFakeLatency + (flLatency - m_flFakeLatency) * 0.3f,
			m_flFakeLatency - TICK_INTERVAL,
			m_flFakeLatency + TICK_INTERVAL);
		if (!flLatency && m_flFakeLatency < TICK_INTERVAL)
			m_flFakeLatency = 0.f;
	}
}

void CBacktrack::RestorePing(CNetChannel* pNetChan)
{
	pNetChan->m_nInSequenceNr = m_nOldInSequenceNr, pNetChan->m_nInReliableState = m_nOldInReliableState;
}

void CBacktrack::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Ping) || !pLocal->IsAlive())
		return;

	auto pResource = H::Entities.GetResource();
	auto pNetChan = I::EngineClient->GetNetChannelInfo();
	if (!pResource || !pNetChan)
		return;

	static float flFakeLatency = 0.f;
	{
		static Timer tTimer = {};
		if (tTimer.Run(0.5f))
			flFakeLatency = GetFakeLatency();
	}
	float flFakeLerp = GetFakeInterp() > G::Lerp ? GetFakeInterp() : 0.f;

	float flFake = std::min(flFakeLatency + flFakeLerp, m_flMaxUnlag) * 1000;
	float flLatency = std::max(pNetChan->GetLatency(FLOW_INCOMING) + pNetChan->GetLatency(FLOW_OUTGOING) - flFakeLatency, 0.f) * 1000;
	int iLatencyScoreboard = pResource->m_iPing(I::EngineClient->GetLocalPlayer());

	int x = Vars::Menu::PingDisplay.Value.x;
	int y = Vars::Menu::PingDisplay.Value.y + 8;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
	const int nTall = fFont.m_nTall + H::Draw.Scale(1);

	EAlign align = ALIGN_TOP;
	if (x <= 100 + H::Draw.Scale(50, Scale_Round))
	{
		x -= H::Draw.Scale(42, Scale_Round);
		align = ALIGN_TOPLEFT;
	}
	else if (x >= H::Draw.m_nScreenW - 100 - H::Draw.Scale(50, Scale_Round))
	{
		x += H::Draw.Scale(42, Scale_Round);
		align = ALIGN_TOPRIGHT;
	}

	float flCorrect = GetServerCorrect();
	float flIdeal = GetIdealSimTime();
	float flWindowMin = flIdeal - 0.2f;
	float flWindowMax = flIdeal + 0.2f;

	if (flFake || Vars::Backtrack::Interp.Value > G::Lerp * 1000)
	{
		char szText[128];
		snprintf(szText, sizeof(szText), "Real %.0f (+ %.0f) ms",
			flLatency, flFake);
		H::Draw.StringOutlined(fFont, x, y, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, szText);
	}
	else
	{
		char szText[64];
		snprintf(szText, sizeof(szText), "Real %.0f ms", flLatency);
		H::Draw.StringOutlined(fFont, x, y, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, szText);
	}
	{
		char szText[64];
		snprintf(szText, sizeof(szText), "Scoreboard %d ms", iLatencyScoreboard);
		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, szText);
	}
}

std::optional<TickRecord> CBacktrack::GetHitRecord(CBaseEntity* pEntity, CTFWeaponBase* pWeapon, CUserCmd* pCmd, CrosshairRecordInfo_t& InfoOut, const Vec3 vAngles, const Vec3 vPos)
{
	std::optional<TickRecord> pReturnRecord{};
	float flMinFov = 45.f;
	float flMinDist = 50.f;
	bool bInsideRecord = false;
	std::vector<TickRecord*> vRecords;
	if (GetRecords(pEntity, vRecords))
	{
		vRecords = F::Backtrack.GetValidRecords(vRecords);
		for (auto pRecord : vRecords)
		{
			for (int n = 0; n < pRecord->m_vHitboxInfos.size(); n++)
			{
				auto sHitboxInfo = pRecord->m_vHitboxInfos[n];
				if (sHitboxInfo.m_iBone == -1)
					continue;

				const auto vAngleTo = Math::CalcAngle(vPos, sHitboxInfo.m_vCenter);
				const float flFOVTo = Math::CalcFov(vAngles, vAngleTo);
				const float flDistTo = vPos.DistTo(sHitboxInfo.m_vCenter);

				if (flDistTo < flMinDist)
				{
					pReturnRecord = *pRecord;
					flMinDist = flDistTo;
					bInsideRecord = true;
				}
				else if (!bInsideRecord && flFOVTo < flMinFov)
				{
					pReturnRecord = *pRecord;
					flMinFov = flFOVTo;
				}
			}
		}
	}

	InfoOut = CrosshairRecordInfo_t{ flMinDist, flMinFov, bInsideRecord };
	return pReturnRecord;
}

void CBacktrack::BacktrackToCrosshair(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (G::Attacking != 1 || !pLocal->IsAlive() || pLocal->IsAGhost() || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return;

	const Vec3 vShootPos = F::Ticks.GetShootPos();
	const Vec3 vAngles = pCmd->viewangles;

	std::vector<std::pair<TickRecord, CrosshairRecordInfo_t>> vValidRecords;
	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerEnemy))
	{
		if (!pEntity) continue;
		if (!pEntity->As<CTFPlayer>()->IsAlive() || pEntity->As<CTFPlayer>()->IsAGhost() || pEntity->As<CTFPlayer>()->IsInvulnerable())
			continue;

		CrosshairRecordInfo_t sRecordInfo{};
		if (auto pCheckRec = GetHitRecord(pEntity, pWeapon, pCmd, sRecordInfo, vAngles, vShootPos))
			vValidRecords.push_back({ *pCheckRec, sRecordInfo });
	}

	std::optional<TickRecord> pReturnTick{};
	if (!vValidRecords.empty())
	{
		auto pFinalTick = std::min_element(vValidRecords.begin(), vValidRecords.end(), [&](const std::pair<TickRecord, CrosshairRecordInfo_t>& a, const std::pair<TickRecord, CrosshairRecordInfo_t>& b)
			{
				const bool bInsideBoth = a.second.m_bInsideThisRecord && b.second.m_bInsideThisRecord;
				const bool bNotInsideRecords = !a.second.m_bInsideThisRecord && !b.second.m_bInsideThisRecord;
				const bool bResult = { bInsideBoth ? a.second.m_flMinDist < b.second.m_flMinDist : bNotInsideRecords ? a.second.m_flFov < b.second.m_flFov : a.second.m_bInsideThisRecord };
				return bResult;
			});
		pReturnTick = pFinalTick->first;
	}

	if (pReturnTick)
		pCmd->tick_count = TIME_TO_TICKS(pReturnTick->m_flSimTime) + TIME_TO_TICKS(m_flFakeInterp);
}