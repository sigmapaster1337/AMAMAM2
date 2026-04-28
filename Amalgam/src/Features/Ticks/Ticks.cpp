#include "Ticks.h"

#include "../PacketManip/AntiAim/AntiAim.h"
#include "../EnginePrediction/EnginePrediction.h"
#include "../Aimbot/AutoRocketJump/AutoRocketJump.h"
#include "../Backtrack/Backtrack.h"

void CTicks::Reset()
{
	m_bDoubletap = m_bRecharge = m_bWarp = m_bAutoRecharge = false;
	m_iShiftedTicks = m_iShiftedGoal = 0;
}

const int RESERVED_TICKS = 2; // Ticks reserved for anti-aim and other features

void CTicks::Recharge(CTFPlayer* pLocal)
{
	if (!m_bGoalReached)
		return;

	m_bRecharge = false;

	if (m_iDeficit)
	{
		m_iDeficit--, m_iShiftedTicks--;
	}

	// Start auto-recharge if bind is pressed and we're not already doing it
	if (Vars::Doubletap::RechargeTicks.Value && !m_bAutoRecharge && m_iShiftedTicks < m_iMaxShift)
		m_bAutoRecharge = true;

	// Stop auto-recharge if we reach max or if other operations are active
	if (m_bAutoRecharge && (m_iShiftedTicks == m_iMaxShift || m_bDoubletap || m_bWarp))
		m_bAutoRecharge = false;

	if ((!Vars::Doubletap::RechargeTicks.Value && !m_bAutoRecharge)
		|| m_iShiftedTicks == m_iMaxShift)
		return;

	m_bRecharge = true;
	m_iShiftedGoal = m_iMaxShift; // Charge all the way to max
}

void CTicks::Warp()
{
	if (!m_bGoalReached)
		return;

	m_bWarp = false;
	// Allow warp even during auto-recharge (but not during manual recharge)
	if (!Vars::Doubletap::Warp.Value
		|| !m_iShiftedTicks || m_bDoubletap)
		return;

	m_bWarp = true;
	// Stop auto-recharge when warp is used
	m_bAutoRecharge = false;
	m_iShiftedGoal = std::max(m_iShiftedTicks - Vars::Doubletap::WarpRate.Value + 1, 0);
}

void CTicks::Doubletap(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!m_bGoalReached)
		return;

	if (!Vars::Doubletap::Doubletap.Value
		|| m_iWait || m_bWarp)
		return;

	int iTicks = std::min(m_iShiftedTicks + 1, m_iMaxShift + RESERVED_TICKS);
	auto pWeapon = H::Entities.GetWeapon();
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		return;

	bool bAttacking = G::PrimaryWeaponType == EWeaponType::MELEE ? pCmd->buttons & IN_ATTACK : G::Attacking;
	if (!G::CanPrimaryAttack && !G::Reloading || !bAttacking && !m_bDoubletap || F::AutoRocketJump.IsRunning())
		return;

	m_bDoubletap = true;
	// Stop auto-recharge when doubletap is used
	m_bAutoRecharge = false;
	m_iShiftedGoal = std::max(m_iShiftedTicks - Vars::Doubletap::TickLimit.Value + 1, 0);
	if (Vars::Doubletap::AntiWarp.Value)
		m_bAntiWarp = pLocal->m_hGroundEntity();
}



static Vec3 s_vVelocity = {};
static int s_iMaxTicks = 0;
void CTicks::AntiWarp(CTFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove, int iTicks)
{
	s_iMaxTicks = std::max(iTicks + 1, s_iMaxTicks);

	Vec3 vAngles; Math::VectorAngles(s_vVelocity, vAngles);
	vAngles.y = flYaw - vAngles.y;
	Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
	vForward *= s_vVelocity.Length2D();

	if (iTicks > std::max(s_iMaxTicks - 8, 3))
		flForwardMove = -vForward.x, flSideMove = -vForward.y;
	else if (iTicks > 3)
		flForwardMove = flSideMove = 0.f;
	else
		flForwardMove = vForward.x, flSideMove = vForward.y;
}
void CTicks::AntiWarp(CTFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove)
{
	AntiWarp(pLocal, flYaw, flForwardMove, flSideMove, GetTicks());
}
void CTicks::AntiWarp(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bAntiWarp)
		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	else
	{
		s_vVelocity = pLocal->m_vecVelocity();
		s_iMaxTicks = 0;
	}
}

bool CTicks::ValidWeapon(CTFWeaponBase* pWeapon)
{
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_PDA_SPY_BUILD:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_GRAPPLINGHOOK:
	case TF_WEAPON_JAR_MILK:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_BUFF_ITEM:
	case TF_WEAPON_ROCKETPACK:
	case TF_WEAPON_JAR_GAS:
	case TF_WEAPON_LASER_POINTER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_JAR:
		return false;
	}

	return true;
}

void CTicks::MoveFunc(float accumulated_extra_samples, bool bFinalTick)
{
	m_iShiftedTicks--;
	if (m_iWait > 0)
		m_iWait--;

	int iTicks = std::min(m_iShiftedTicks + 1, m_iMaxShift + RESERVED_TICKS);
	auto pWeapon = H::Entities.GetWeapon();
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		m_iWait = -1;

	m_bGoalReached = bFinalTick && m_iShiftedTicks == m_iShiftedGoal;

	static auto CL_Move = U::Hooks.m_mHooks["CL_Move"];
	CL_Move->Call<void>(accumulated_extra_samples, bFinalTick);
}

void CTicks::Move(float accumulated_extra_samples, bool bFinalTick)
{
	MoveManage();

	while (m_iShiftedTicks > m_iMaxShift)
		MoveFunc(accumulated_extra_samples, false);
	m_iShiftedTicks = std::max(m_iShiftedTicks, 0) + 1;



	m_iShiftedGoal = std::clamp(m_iShiftedGoal, 0, m_iMaxShift);
	if (m_iShiftedTicks > m_iShiftedGoal) // normal use/doubletap/teleport
	{
		m_iShiftStart = m_iShiftedTicks - 1;
		m_bShifted = false;

		while (m_iShiftedTicks > m_iShiftedGoal)
		{
			m_bShifting = m_bShifted = m_bShifted || m_iShiftedTicks - 1 != m_iShiftedGoal;
			MoveFunc(accumulated_extra_samples, m_iShiftedTicks - 1 == m_iShiftedGoal);
		}

		m_bShifting = m_bAntiWarp = m_bTimingUnsure = false;
		if (m_bWarp)
			m_iDeficit = 0;

		m_bDoubletap = m_bWarp = false;
	}
	else // else recharge, run once if we have any choked ticks
	{
		if (I::ClientState->chokedcommands)
			MoveFunc(accumulated_extra_samples, bFinalTick);
	}
}

void CTicks::MoveManage()
{
	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return;

	Recharge(pLocal);
	Warp();

	if (!m_bRecharge)
		m_iWait = std::max(m_iWait, 0);
	if (auto pWeapon = H::Entities.GetWeapon())
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_CANNON:
			if (!G::CanSecondaryAttack)
				m_iWait = Vars::Doubletap::TickLimit.Value;
			break;
		default:
			if (!ValidWeapon(pWeapon))
				m_iWait = -1;
			else if (G::Attacking || !G::CanPrimaryAttack && !G::Reloading)
				m_iWait = Vars::Doubletap::TickLimit.Value;
		}
	}
	else
		m_iWait = -1;

	static auto sv_maxusrcmdprocessticks = H::ConVars.FindVar("sv_maxusrcmdprocessticks");
	m_iMaxUsrCmdProcessTicks = sv_maxusrcmdprocessticks->GetInt();
	if (Vars::Misc::Game::AntiCheatCompatibility.Value)
		m_iMaxUsrCmdProcessTicks = std::min(m_iMaxUsrCmdProcessTicks, 8);

	m_iMaxShift = m_iMaxUsrCmdProcessTicks - std::max(m_iMaxUsrCmdProcessTicks - Vars::Doubletap::RechargeLimit.Value, 0) - RESERVED_TICKS;
	m_iMaxShift = std::max(m_iMaxShift, 1);
}

void CTicks::CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, bool* pSendPacket)
{
	Doubletap(pLocal, pCmd);
	AntiWarp(pLocal, pCmd);
	ManagePacket(pCmd, pSendPacket);

	SaveShootPos(pLocal);
	SaveShootAngle(pCmd, *pSendPacket);

	if (m_bDoubletap && m_iShiftedTicks == m_iShiftStart && pWeapon && pWeapon->IsInReload())
		m_bTimingUnsure = true;
}

void CTicks::ManagePacket(CUserCmd* pCmd, bool* pSendPacket)
{
	if (!m_bDoubletap && !m_bWarp)
	{
		static bool bWasSet = false;
		bool bCanChoke = CanChoke(true); // failsafe
		if (G::PSilentAngles && bCanChoke)
			*pSendPacket = false, bWasSet = true;
		else if (bWasSet || !bCanChoke)
			*pSendPacket = true, bWasSet = false;

		bool bShouldShift = m_iShiftedTicks && m_iShiftedTicks + I::ClientState->chokedcommands >= m_iMaxUsrCmdProcessTicks;
		if (!*pSendPacket && bShouldShift)
			m_iShiftedGoal = std::max(m_iShiftedGoal - 1, 0);
	}
	else
	{
		if (m_bWarp && G::Attacking == 1)
		{
			*pSendPacket = true;
			return;
		}

		*pSendPacket = m_iShiftedGoal == m_iShiftedTicks;
		if (I::ClientState->chokedcommands >= 21) // prevent overchoking
			*pSendPacket = true;
	}
}

void CTicks::CorrectTickbase(CTFPlayer* pLocal)
{
	// Only needed during/after tick shifting
	if (!m_bShifted && !m_bDoubletap && !m_bWarp)
		return;

	static auto sv_clockcorrection_msecs = H::ConVars.FindVar("sv_clockcorrection_msecs");
	float flCorrectionSeconds = std::clamp(
		sv_clockcorrection_msecs->GetFloat() / 1000.f, 0.f, 1.f);
	int nCorrectionTicks = TIME_TO_TICKS(flCorrectionSeconds);

	// Server's ideal tick = server_tick + correction
	// We estimate server tick from backtrack's tickcount + incoming latency
	int nigger = I::GlobalVars->tickcount;
	int nServerTick = nigger + TIME_TO_TICKS(F::Backtrack.GetReal(FLOW_INCOMING));
	int nIdealFinalTick = nServerTick + nCorrectionTicks;

	// Our estimated final tick after simulation
	int nSimulationTicks = I::ClientState->chokedcommands + 1;
	int nEstimatedFinalTick = pLocal->m_nTickBase() + nSimulationTicks;

	int too_fast = nIdealFinalTick + nCorrectionTicks;
	int too_slow = nIdealFinalTick - nCorrectionTicks;

	if (nEstimatedFinalTick > too_fast || nEstimatedFinalTick < too_slow)
	{
		auto pNetChan = I::EngineClient->GetNetChannelInfo();
		if (!pNetChan) return;

		int nProcess = I::GlobalVars->simTicksThisFrame
			+ TIME_TO_TICKS(pNetChan->GetLatency(FLOW_OUTGOING)
				+ pNetChan->GetLatency(FLOW_INCOMING));

		int nCorrectedTick = nIdealFinalTick - nSimulationTicks + nProcess;
		pLocal->m_nTickBase() = nCorrectedTick;
	}
}

void CTicks::Start(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	Vec2 vOriginalMove; int iOriginalButtons;
	if (m_bPredictAntiwarp = m_bAntiWarp || GetTicks(H::Entities.GetWeapon()) && Vars::Doubletap::AntiWarp.Value && pLocal->m_hGroundEntity())
	{
		vOriginalMove = { pCmd->forwardmove, pCmd->sidemove };
		iOriginalButtons = pCmd->buttons;

		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	}

	F::EnginePrediction.Start(pLocal, pCmd);

	// Correct tickbase to match server's expectation after DT/warp
	CorrectTickbase(pLocal);

	if (m_bPredictAntiwarp)
	{
		pCmd->forwardmove = vOriginalMove.x, pCmd->sidemove = vOriginalMove.y;
		pCmd->buttons = iOriginalButtons;
	}
}

void CTicks::End(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bPredictAntiwarp && !m_bAntiWarp && !G::Attacking)
	{
		F::EnginePrediction.End(pLocal, pCmd);
		F::EnginePrediction.Start(pLocal, pCmd);
	}
}

bool CTicks::CanChoke(bool bCanShift, int iMaxTicks)
{
	bool bCanChoke = I::ClientState->chokedcommands < 21;
	if (bCanChoke && !bCanShift)
		bCanChoke = m_iShiftedTicks + I::ClientState->chokedcommands < iMaxTicks;
	return bCanChoke;
}
bool CTicks::CanChoke(bool bCanShift)
{
	return CanChoke(bCanShift, m_iMaxUsrCmdProcessTicks);
}

int CTicks::GetTicks(CTFWeaponBase* pWeapon)
{
	if (m_bDoubletap && m_iShiftedGoal < m_iShiftedTicks)
		return m_iShiftedTicks - m_iShiftedGoal;

	if (!Vars::Doubletap::Doubletap.Value
		|| m_iWait || m_bWarp || m_bRecharge || F::AutoRocketJump.IsRunning())
		return 0;

	int iTicks = std::min(m_iShiftedTicks + 1, m_iMaxShift + RESERVED_TICKS);
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		return 0;

	return std::min(Vars::Doubletap::TickLimit.Value - 1, m_iMaxShift);
}

int CTicks::GetShotsWithinPacket(CTFWeaponBase* pWeapon, int iTicks)
{
	iTicks = std::min(m_iMaxShift + 1, iTicks);

	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return 1 + (iTicks - iDelay) / std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL);
}

int CTicks::GetMinimumTicksNeeded(CTFWeaponBase* pWeapon)
{
	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return (GetShotsWithinPacket(pWeapon) - 1) * std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL) + iDelay;
}

void CTicks::SaveShootPos(CTFPlayer* pLocal)
{
	if (m_iShiftedTicks == m_iShiftStart)
	{
		// Predict where we'll be when the shot actually fires on server.
		// During DT/warp the server processes multiple ticks, moving us each tick.
		// The shot fires after (m_iShiftStart - m_iShiftedGoal) ticks of movement.
		int iTicksUntilShot = m_iShiftStart - m_iShiftedGoal;
		if (iTicksUntilShot > 0 && m_bDoubletap)
		{
			Vec3 vVelocity = pLocal->m_vecVelocity();
			Vec3 vPredicted = pLocal->GetShootPos();
			// Server runs RunCommand for each shifted tick, moving us by velocity * tick_interval
			vPredicted += vVelocity * TICKS_TO_TIME(iTicksUntilShot);
			m_vShootPos = vPredicted;
		}
		else
		{
			m_vShootPos = pLocal->GetShootPos();
		}
	}
}
Vec3 CTicks::GetShootPos()
{
	return m_vShootPos;
}

void CTicks::SaveShootAngle(CUserCmd* pCmd, bool bSendPacket)
{
	static auto sv_maxusrcmdprocessticks_holdaim = H::ConVars.FindVar("sv_maxusrcmdprocessticks_holdaim");

	if (bSendPacket)
		m_bShootAngle = false;
	else if (!m_bShootAngle && G::Attacking == 1 && sv_maxusrcmdprocessticks_holdaim->GetBool())
		m_vShootAngle = pCmd->viewangles, m_bShootAngle = true;
}
Vec3* CTicks::GetShootAngle()
{
	if (m_bShootAngle && I::ClientState->chokedcommands)
		return &m_vShootAngle;
	return nullptr;
}

bool CTicks::IsTimingUnsure()
{	// actually knowing when we'll shoot would be better than this, but this is fine for now
	return m_bTimingUnsure /*|| m_bWarp*/;
}

void CTicks::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Ticks) || !pLocal->IsAlive())
		return;

	const DragBox_t dtPos = Vars::Menu::TicksDisplay.Value;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);

	int iAntiAimTicks = F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0;
	int iChoke = std::max(I::ClientState->chokedcommands - iAntiAimTicks, 0);

	// Use m_iMaxShift which already has reserved ticks subtracted
	int iTicks = std::clamp(m_iShiftedTicks + iChoke, 0, m_iMaxShift);
	float flRatio = float(iTicks) / m_iMaxShift;

	int iSizeX = H::Draw.Scale(80, Scale_Round);
	int iSizeY = H::Draw.Scale(8, Scale_Round);
	int iPosX = dtPos.x - iSizeX / 2;
	int iPosY = dtPos.y + 25;

	H::Draw.StringOutlined(fFont, dtPos.x, iPosY - H::Draw.Scale(8, Scale_Round) - fFont.m_nTall,
		Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOP,
		std::format("{} / {}", iTicks, m_iMaxShift).c_str());

	H::Draw.LineRect(iPosX, iPosY, iSizeX, iSizeY, Vars::Menu::Theme::Background.Value);

	if (flRatio)
	{
		int iFillSizeX = iSizeX - 2;
		int iFillSizeY = iSizeY - 2;
		int iFillPosX = iPosX + 1;
		int iFillPosY = iPosY + 1;

		bool bWeaponCooldown = m_iWait > 0 && iTicks >= Vars::Doubletap::TickLimit.Value;

		Color_t fillColor = bWeaponCooldown ? Color_t(159, 156, 156, 255) : Vars::Menu::Theme::Accent.Value;
		H::Draw.FillRect(iFillPosX, iFillPosY, iFillSizeX * flRatio, iFillSizeY, fillColor);
	}
}