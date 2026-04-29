#include "NoSpreadHitscan.h"

#include "../../Ticks/Ticks.h"
#include <regex>
#include <numeric>
#include <algorithm>

MAKE_SIGNATURE(CTFWeaponBaseGun_GetWeaponSpread, "client.dll", "48 89 5C 24 ? 57 48 83 EC ? 4C 63 91", 0x0);

void CNoSpreadHitscan::Reset()
{
	m_bWaitingForPlayerPerf = false;
	m_flServerTime = 0.f;
	m_vSamples.clear();
	m_dTimeDelta = 0.0;
	m_dRequestTime = 0.0;
	m_dResponseTime = 0.0;

	m_iSeed = 0;
	m_flMantissaStep = 0.f;

	m_bSynced = false;
}

bool CNoSpreadHitscan::ShouldRun(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, bool bCreateMove)
{
	if (G::PrimaryWeaponType != EWeaponType::HITSCAN
		|| (bCreateMove ? pWeapon->GetWeaponSpread() : S::CTFWeaponBaseGun_GetWeaponSpread.Call<float>(pWeapon)) <= 0.f)
		return false;

	return bCreateMove ? G::Attacking == 1 : true;
}

int CNoSpreadHitscan::GetSeed(CUserCmd* pCmd)
{
	static auto sv_usercmd_custom_random_seed = H::ConVars.FindVar("sv_usercmd_custom_random_seed");
	if (!sv_usercmd_custom_random_seed->GetBool())
		return pCmd->random_seed & 255;

	// forward latency accounting, our cmd will arrive at the server in avg latency_out seconds
	double dLatencyOut = 0.0;
	if (auto pNet = I::EngineClient->GetNetChannelInfo())
		dLatencyOut = double(pNet->GetLatency(FLOW_OUTGOING));

	double dFloatTime = SDK::PlatFloatTime() + m_dTimeDelta + dLatencyOut;
	float flTime = float(dFloatTime * 1000.0);
	return *reinterpret_cast<int*>((char*)&flTime) & 255;
}

float CNoSpreadHitscan::CalcMantissaStep(float flV)
{
	// delta to the next representable float value, in milliseconds-per-unit
	float flNextValue = std::nextafter(flV, std::numeric_limits<float>::infinity());
	float flMantissaStep = (flNextValue - flV) * 1000;

	// round up to the next power of 2
	return powf(2, ceilf(logf(flMantissaStep) / logf(2)));
}

std::string CNoSpreadHitscan::GetFormat(int iServerTime)
{
	int iHours = iServerTime / 3600;
	int iMinutes = (iServerTime % 3600) / 60;
	int iSeconds = iServerTime % 60;

	if (iHours > 0)
		return std::format("{} h {} min", iHours, iMinutes);
	else if (iMinutes > 0)
		return std::format("{} min {} sec", iMinutes, iSeconds);
	else
		return std::format("0 min {} sec", iSeconds);
}

void CNoSpreadHitscan::RecomputeDelta()
{
	if (m_vSamples.empty())
		return;

	// delta estimate is closer to the truth
	std::vector<PerfSample_t> vSorted(m_vSamples.begin(), m_vSamples.end());
	std::sort(vSorted.begin(), vSorted.end(),
		[](const PerfSample_t& a, const PerfSample_t& b) { return a.dRTT < b.dRTT; });

	const size_t nKeep = std::max<size_t>(1, vSorted.size() / 3);
	std::vector<double> vDeltas;
	vDeltas.reserve(nKeep);
	for (size_t i = 0; i < nKeep; i++)
		vDeltas.push_back(vSorted[i].dDelta);

	std::sort(vDeltas.begin(), vDeltas.end());
	m_dTimeDelta = vDeltas[vDeltas.size() / 2]; // median

	// final user tunable nudge, in ticks
	m_dTimeDelta += TICKS_TO_TIME(Vars::Aimbot::General::NoSpreadOffset.Value);
}

void CNoSpreadHitscan::AskForPlayerPerf()
{
	if (!Vars::Aimbot::General::NoSpread.Value || !I::EngineClient->IsInGame())
		return Reset();

	if (G::Choking)
		return;

	static Timer tTimer = {};
	if (!m_bWaitingForPlayerPerf ? tTimer.Run(Vars::Aimbot::General::NoSpreadInterval.Value) : tTimer.Run(Vars::Aimbot::General::NoSpreadBackupInterval.Value))
	{
		I::ClientState->SendStringCmd("playerperf");
		m_bWaitingForPlayerPerf = true;
		m_dRequestTime = SDK::PlatFloatTime();
	}
}

bool CNoSpreadHitscan::ParsePlayerPerf(const std::string& sMsg)
{
	if (!Vars::Aimbot::General::NoSpread.Value)
		return false;

	std::smatch match; std::regex_match(sMsg, match, std::regex(R"((\d+.\d+)\s\d+\s\d+\s\d+.\d+\s\d+.\d+\svel\s\d+.\d+)"));

	if (match.size() == 2)
	{
		// Lock down arrival time immediately.
		const double dNowClient = SDK::PlatFloatTime();
		m_dResponseTime = dNowClient;
		m_bWaitingForPlayerPerf = false;

		float flNewServerTime = std::stof(match[1].str());
		if (flNewServerTime < m_flServerTime)
			return true;

		m_flServerTime = flNewServerTime;

		// network time protocol average estimation
		// ugh basically server sampled Plat_FloatTime() at the midpoint of the round trip
		// correct on average when the network path is symmetric
		// on loopback we dont really care, but it
		// degenerates to serverTime - clientTime, capturing the constant process-start offset between the two Plat_FloatTime() counters
		const double dRTT = dNowClient - m_dRequestTime;
		const double dMidpoint = (m_dRequestTime + dNowClient) * 0.5;
		const double dRawDelta = double(m_flServerTime) - dMidpoint;

		m_vSamples.push_back({ dRawDelta, dRTT });
		const size_t nMax = size_t(Vars::Aimbot::General::NoSpreadAverage.Value);
		while (m_vSamples.size() > std::max<size_t>(nMax, 3))
			m_vSamples.pop_front();

		RecomputeDelta();

		float flMantissaStep = CalcMantissaStep(m_flServerTime);
		// mantissa step ~= quantization of serverTime*1000 to int32
		// need delta accuracy < step/2 to land in the right bucket
		// typical jitter limited accuracy is a few ms, so 1ms step is hopeless, 4ms borderline, 8ms+ reliable on a good connection
		m_bSynced = flMantissaStep >= 1.f;

		if (flMantissaStep > m_flMantissaStep && (m_bSynced || !m_flMantissaStep))
		{
			SDK::Output("Seed Prediction", m_bSynced ? std::format("Synced (delta={:.4f}s)", m_dTimeDelta).c_str() : "Not synced, step too low", Vars::Menu::Theme::Accent.Value);
			SDK::Output("Seed Prediction", std::format("Age {}; Step {:.3f}", GetFormat(m_flServerTime), flMantissaStep).c_str(), Vars::Menu::Theme::Accent.Value);
		}
		m_flMantissaStep = flMantissaStep;

#ifdef SEEDPRED_DEBUG
		SDK::Output("playerperf", std::format("srvTime={:.4f} raw={:.4f} RTT={:.4f} finalDelta={:.4f} samples={}",
			m_flServerTime, dRawDelta, dRTT, m_dTimeDelta, m_vSamples.size()).c_str(), { 255, 255, 0 });
#endif

		return true;
	}

	return std::regex_match(sMsg, std::regex(R"(\d+.\d+\s\d+\s\d+)"));
}

void CNoSpreadHitscan::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (!ShouldRun(pLocal, pWeapon, true))
		return;

	m_iSeed = GetSeed(pCmd);
#ifdef SEEDPRED_DEBUG
	{
		double dLatOut = I::EngineClient->GetNetChannelInfo() ? I::EngineClient->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) : 0.0;
		double dSrvTime = SDK::PlatFloatTime() + m_dTimeDelta + dLatOut;
		SDK::Output("CNoSpreadHitscan::Run", std::format("predSrv={:.4f} lat={:.4f} seed={}", dSrvTime, dLatOut, m_iSeed).c_str(), { 255, 0, 0 });
	}
#endif

	if (!m_bSynced)
		return;

	// credits to cathook for average spread stuff
	float flSpread = pWeapon->GetWeaponSpread();
	int iBulletsPerShot = pWeapon->GetBulletsPerShot();
	float flFireRate = std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL) * TICK_INTERVAL;

	// Perfect-first-shot detection: after enough idle time the server grants a nospread first bullet.
	// Single-bullet: nothing to correct, bail out entirely
	// Multi-bullet: zero out x/y for bullet 0 so it doesn't skew the averaged correction toward a spread offset the server won't apply
	bool bFirstShotPerfect = false;
	{
		int iTicks = F::Ticks.GetTicks();
		if (!iTicks || iTicks < TIME_TO_TICKS(flFireRate) * 2)
		{
			float flTimeSinceLastShot = I::GlobalVars->curtime - pWeapon->m_flLastFireTime();
			bFirstShotPerfect = flTimeSinceLastShot > (iBulletsPerShot > 1 ? 0.25f : 1.25f);
		}
	}
	if (bFirstShotPerfect && iBulletsPerShot == 1)
		return;

	// EyeAngles() + GetPunchAngle()
	Vec3 vPunchAngle = pLocal->m_vecPunchAngle();
	Vec3 vAngles = pCmd->viewangles + vPunchAngle;
	Math::ClampAngles(vAngles);

	std::vector<Vec3> vBulletCorrections = {};
	Vec3 vAverageSpread = {};
	for (int iBullet = 0; iBullet < iBulletsPerShot; iBullet++)
	{
		// mirror of server's RandomSeed/RandomFloat sequence in FireBullets
		SDK::RandomSeed(m_iSeed + iBullet);

		float x = SDK::RandomFloat(-0.5f, 0.5f) + SDK::RandomFloat(-0.5f, 0.5f);
		float y = SDK::RandomFloat(-0.5f, 0.5f) + SDK::RandomFloat(-0.5f, 0.5f);

		if (bFirstShotPerfect && iBullet == 0)
		{
			// server emits this bullet with no spread regardless of seed
			// the averaged correction must reflect that, otherwise we'd pull the representative bullet toward an offset that won't be applied
			x = 0.f;
			y = 0.f;
		}

		Vec3 vForward, vRight, vUp; Math::AngleVectors(vAngles, &vForward, &vRight, &vUp);
		Vec3 vFixedSpread = vForward + (vRight * x * flSpread) + (vUp * y * flSpread);
		vFixedSpread.Normalize();
		vAverageSpread += vFixedSpread;

		vBulletCorrections.push_back(vFixedSpread);
	}
	vAverageSpread /= static_cast<float>(iBulletsPerShot);

	const auto cFixedSpread = std::ranges::min_element(vBulletCorrections,
		[&](const Vec3& lhs, const Vec3& rhs)
		{
			return lhs.DistTo(vAverageSpread) < rhs.DistTo(vAverageSpread);
		});

	if (cFixedSpread == vBulletCorrections.end())
		return;

	// vFixedAngles = direction (with punch) the representative bullet will actually travel
	Vec3 vFixedAngles = Math::VectorAngles(*cFixedSpread) - vPunchAngle;

	pCmd->viewangles += pCmd->viewangles - vFixedAngles;
	Math::ClampAngles(pCmd->viewangles);

	G::SilentAngles = true;
}

void CNoSpreadHitscan::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::SeedPrediction) || !Vars::Aimbot::General::NoSpread.Value || !pLocal->IsAlive())
		return;

	int x = Vars::Menu::SeedPredictionDisplay.Value.x;
	int y = Vars::Menu::SeedPredictionDisplay.Value.y + 8;
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

	const auto& cColor = m_bSynced ? Vars::Menu::Theme::Active.Value : Vars::Menu::Theme::Inactive.Value;

	H::Draw.StringOutlined(fFont, x, y, cColor, Vars::Menu::Theme::Background.Value, align, std::format("Server uptime {}", GetFormat(m_flServerTime)).c_str());
	H::Draw.StringOutlined(fFont, x, y += nTall, cColor, Vars::Menu::Theme::Background.Value, align, std::format("Mantissa step {:.3f}", m_flMantissaStep).c_str());
	if (Vars::Debug::Info.Value)
	{
		H::Draw.StringOutlined(fFont, x, y += nTall, cColor, Vars::Menu::Theme::Background.Value, align, std::format("Delta {:.4f}", m_dTimeDelta).c_str());
		H::Draw.StringOutlined(fFont, x, y += nTall, cColor, Vars::Menu::Theme::Background.Value, align, std::format("Samples {}", m_vSamples.size()).c_str());
	}
}