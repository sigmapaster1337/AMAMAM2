#include "OffscreenArrows.h"

#include "../Groups/Groups.h"

void COffscreenArrows::DrawArrowTo(const Vec3& vFromPos, const Vec3& vToPos, Color_t tColor, int iOffset, float flMaxDistance)
{
	float flMap = Math::RemapVal(vFromPos.DistTo(vToPos), flMaxDistance, flMaxDistance * 0.9f, 0.f, 1.f);
	tColor.a = byte(flMap * 255.f);
	if (!tColor.a)
		return;

	Vec2 vCenter = { H::Draw.m_nScreenW / 2.f, H::Draw.m_nScreenH / 2.f };
	Vec3 vScreen;
	if (SDK::W2S(vToPos, vScreen, true))
	{
		float flMin = std::min(vCenter.x, vCenter.y), flMax = std::max(vCenter.x, vCenter.y);
		float flDist = sqrt(powf(vScreen.x - vCenter.x, 2) + powf(vScreen.y - vCenter.y, 2));
		tColor.a *= std::clamp((flDist - flMin) / (flMin != flMax ? flMax - flMin : 1), 0.f, 1.f);
		if (!tColor.a)
			return;
	}

	Vec3 vAngle = Math::VectorAngles({ vCenter.x - vScreen.x, vCenter.y - vScreen.y, 0 });
	const float flDeg = Math::Deg2Rad(vAngle.y);
	const float flCos = cos(flDeg);
	const float flSin = sin(flDeg);

	float flOffset = -iOffset;
	float flBase = H::Draw.Scale(20);
	float flHeight = H::Draw.Scale(22);

	Vec2 vPos = { flOffset * flCos, flOffset * flSin };
	if (fabs(vPos.x) > vCenter.x - flHeight || fabs(vPos.y) > vCenter.y - flHeight)
	{
		Vec2 a = { -(vCenter.x - flHeight) / vPos.x, -(vCenter.y - flHeight) / vPos.y };
		Vec2 b = { (vCenter.x - flHeight) / vPos.x, (vCenter.y - flHeight) / vPos.y };
		Vec2 c = { std::min(a.x, b.x), std::min(a.y, b.y) };
		vPos *= fabsf(std::max(c.x, c.y));
	}
	vPos += vCenter;

	Vec2 v1 = { 0, flBase / 2.f },
		v2 = { 0, -flBase / 2.f },
		v3 = { -flHeight, 0 };

	H::Draw.FillPolygon(
		{
			{ { vPos.x + v1.x * flCos - v1.y * flSin, vPos.y + v1.y * flCos + v1.x * flSin } },
			{ { vPos.x + v2.x * flCos - v2.y * flSin, vPos.y + v2.y * flCos + v2.x * flSin } },
			{ { vPos.x + v3.x * flCos - v3.y * flSin, vPos.y + v3.y * flCos + v3.x * flSin } }
		}, tColor
	);
}

void COffscreenArrows::Store()
{
	m_mCache.clear();
	if (!F::Groups.GroupsActive())
		return;

	for (auto& [pEntity, pGroup] : F::Groups.GetGroup(false))
	{
		if (!pGroup->m_bOffscreenArrows
			|| pEntity->entindex() == I::EngineClient->GetLocalPlayer())
			continue;

		ArrowCache_t& tCache = m_mCache[pEntity];
		tCache.m_tColor = F::Groups.GetColor(pEntity, pGroup);
		tCache.m_iOffset = pGroup->m_iOffscreenArrowsOffset;
		tCache.m_flMaxDistance = pGroup->m_flOffscreenArrowsMaxDistance;
	}
}

void COffscreenArrows::Draw(CTFPlayer* pLocal)
{
	if (m_mCache.empty())
		return;

	Vec3 vLocalPos = pLocal->GetEyePosition();
	for (auto& [pEntity, tCache] : m_mCache)
		DrawArrowTo(vLocalPos, pEntity->GetCenter(), tCache.m_tColor, tCache.m_iOffset, tCache.m_flMaxDistance);
}