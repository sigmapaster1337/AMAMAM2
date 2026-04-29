#include "../SDK/SDK.h"

#include "../Features/Visuals/Visuals.h"

MAKE_SIGNATURE(CTFPlayer_FireBullet, "client.dll", "48 89 74 24 ? 55 57 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? F3 41 0F 10 58", 0x0);

MAKE_HOOK(CTFPlayer_FireBullet, S::CTFPlayer_FireBullet(), void,
	void* rcx, CBaseCombatWeapon* pWeapon, const FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
{
	DEBUG_RETURN(CTFPlayer_FireBullet, rcx, pWeapon, info, nDamageType, nDamageType, nCustomDamageType);

	auto pLocal = reinterpret_cast<CTFPlayer*>(rcx);
	if (pLocal != H::Entities.GetLocal() || !pWeapon)
		return CALL_ORIGINAL(rcx, pWeapon, info, bDoEffects, nDamageType, nCustomDamageType);

	auto& sString = nDamageType & DMG_CRITICAL ? Vars::Visuals::Effects::CritTracer.Value : Vars::Visuals::Effects::BulletTracer.Value;
	auto uHash = FNV1A::Hash32(sString.c_str());
	if (uHash == FNV1A::Hash32Const("Default"))
		return CALL_ORIGINAL(rcx, pWeapon, info, bDoEffects, nDamageType, nCustomDamageType);
	else if (uHash == FNV1A::Hash32Const("None"))
		return;

	const Vec3 vStart = info.m_vecSrc;
	const Vec3 vEnd = vStart + info.m_vecDirShooting * info.m_flDistance;
	CGameTrace trace = {};
	CTraceFilterHitscan filter = {};
	filter.pSkip = pLocal;
	SDK::Trace(vStart, vEnd, MASK_SHOT | CONTENTS_GRATE, &filter, &trace);

	int iIndex = I::EngineClient->GetLocalPlayer();
	int iTeam = pLocal->m_iTeamNum();
	int iAttachment = pWeapon->LookupAttachment("muzzle");
	pWeapon->GetAttachment(iAttachment, trace.startpos);

	switch (uHash)
	{
	case FNV1A::Hash32Const("Big nasty"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "bullet_bignasty_tracer01_blue" : "bullet_bignasty_tracer01_red", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Distortion trail"):
		H::Particles.ParticleTracer("tfc_sniper_distortion_trail", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Machina"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "dxhr_sniper_rail_red" : "dxhr_sniper_rail_blue", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Sniper rail"):
		H::Particles.ParticleTracer("dxhr_sniper_rail", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Short circuit"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "dxhr_lightningball_hit_zap_red" : "dxhr_lightningball_hit_zap_blue", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("C.A.P.P.E.R"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "bullet_tracer_raygun_red" : "bullet_tracer_raygun_blue", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Merasmus ZAP"):
		H::Particles.ParticleTracer("merasmus_zap", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Merasmus ZAP 2"):
		H::Particles.ParticleTracer("merasmus_zap_beam02", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Black ink"):
		H::Particles.ParticleTracer("merasmus_zap_beam01", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Line"):
	case FNV1A::Hash32Const("Line ignore Z"):
	{
		const float flDrawDuration = Vars::Visuals::Line::DrawDuration.Value;
		const float flFadeout = Vars::Visuals::Line::Fadeout.Value;
		const float flEndTime = I::GlobalVars->curtime + flDrawDuration + flFadeout;

		Color_t tLineColor = (uHash == FNV1A::Hash32Const("Line"))
			? Vars::Colors::Line.Value
			: Vars::Colors::LineIgnoreZ.Value;

		bool bZBuffer = (uHash == FNV1A::Hash32Const("Line"));

		// Store in our fading vector
		F::Visuals.m_vLines.emplace_back(
			std::pair<Vec3, Vec3>(trace.startpos, trace.endpos),
			flEndTime,
			tLineColor,
			bZBuffer
		);

		// === Box at impact point (exact same size as projectile path) ===
		if (Vars::Colors::LineBox.Value)
		{
			const float flSize = 1.f;                    // exact same as projectile path boxes
			const Vec3 vMins = { -flSize, -flSize, -flSize };
			const Vec3 vMaxs = { flSize,  flSize,  flSize };

			Color_t tEdge = bZBuffer ? Vars::Colors::LineBoxEdge.Value : Vars::Colors::LineBoxEdgeIgnoreZ.Value;
			Color_t tFace = bZBuffer ? Vars::Colors::LineBoxFace.Value : Vars::Colors::LineBoxFaceIgnoreZ.Value;

			if (tEdge.a || tFace.a)
			{
				DrawBoxFadeBullet_t tBox;
				tBox.m_vOrigin = trace.endpos;
				tBox.m_vMins = vMins;
				tBox.m_vMaxs = vMaxs;
				tBox.m_vAngles = Vec3();
				tBox.m_flTime = flEndTime;           // same lifetime as the line
				tBox.m_tColorEdge = tEdge;
				tBox.m_tColorFace = tFace;
				tBox.m_bZBuffer = bZBuffer;

				F::Visuals.m_vBulletBoxes.emplace_back(tBox);
			}
		}

		break;
	}
	case FNV1A::Hash32Const("Beam"):
	{
		BeamInfo_t beamInfo;
		beamInfo.m_nType = 0;
		beamInfo.m_pszModelName = !Vars::Visuals::Beams::Model.Value.empty() ? Vars::Visuals::Beams::Model.Value.c_str() : "sprites/physbeam.vmt";
		beamInfo.m_nModelIndex = -1; // will be set by CreateBeamPoints if its -1
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = Vars::Visuals::Beams::Life.Value;
		beamInfo.m_flWidth = Vars::Visuals::Beams::Width.Value;
		beamInfo.m_flEndWidth = Vars::Visuals::Beams::EndWidth.Value;
		beamInfo.m_flFadeLength = Vars::Visuals::Beams::FadeLength.Value;
		beamInfo.m_flAmplitude = Vars::Visuals::Beams::Amplitude.Value;
		beamInfo.m_flBrightness = Vars::Visuals::Beams::Brightness.Value;
		beamInfo.m_flSpeed = Vars::Visuals::Beams::Speed.Value;
		beamInfo.m_nStartFrame = 0;
		beamInfo.m_flFrameRate = 0;
		beamInfo.m_flRed = Vars::Visuals::Beams::Color.Value.r;
		beamInfo.m_flGreen = Vars::Visuals::Beams::Color.Value.g;
		beamInfo.m_flBlue = Vars::Visuals::Beams::Color.Value.b;
		beamInfo.m_nSegments = Vars::Visuals::Beams::Segments.Value;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = Vars::Visuals::Beams::Flags.Value;
		beamInfo.m_vecStart = trace.startpos;
		beamInfo.m_vecEnd = trace.endpos;

		if (auto pBeam = I::ViewRenderBeams->CreateBeamPoints(beamInfo))
			I::ViewRenderBeams->DrawBeam(pBeam);

		break;
	}
	default:
		H::Particles.ParticleTracer(sString.c_str(), trace.startpos, trace.endpos, iIndex, iAttachment, true);
	}
}