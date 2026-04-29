#pragma once
#include "../../SDK/SDK.h"

//#define DEBUG_TEXT

struct Projectile_t
{
	std::vector<Vec3> m_vPath = {};
	float m_flTime = 0.f;
	float m_flRadius = 0.f;
	Vec3 m_vNormal = { 0, 0, 1 };
	Color_t m_tColor = {};
	int m_iFlags = 0b0;

	float m_flSpawnTime = 0.f;
	bool m_bCritical = false;
};

struct DrawBoxFade_t : public DrawBox_t
{
	float m_flDrawTime = 0.f;
	float m_flFadeStartTime = 0.f;

	DrawBoxFade_t() = default;
	DrawBoxFade_t(const Vec3& vOrigin, const Vec3& vMins, const Vec3& vMaxs, const Vec3& vAngles, float flTime, const Color_t& tEdge, const Color_t& tFace, bool bZBuffer = true)
		: DrawBox_t(vOrigin, vMins, vMaxs, vAngles, flTime, tEdge, tFace, bZBuffer)
		, m_flFadeStartTime(flTime)
		, m_flDrawTime(flTime)
	{
	}
};

struct DrawLineFade_t
{
	std::pair<Vec3, Vec3> m_paOrigin = {};
	float m_flTime = 0.f;        // end time
	Color_t m_tColor = {};
	bool m_bZBuffer = false;
};

struct DrawBoxFadeBullet_t : public DrawBox_t
{
	float m_flTime = 0.f;        // end time (same as line)
};

struct Sightline_t
{
	Vec3 m_vStart = {};
	Vec3 m_vEnd = {};
	Color_t m_tColor = {};
	bool m_bZBuffer = false;
};

struct PendingHitData_t
{
	matrix3x4 m_aBones[MAXSTUDIOBONES] = {};
	int m_nAimedHitbox = -1;
	std::vector<int> m_vAimedHitboxes = {};
	float m_flTime = 0.f; // curtime when registered, for stale-entry expiry
};

struct PickupData_t
{
	int m_iType = 0;
	float m_flTime = 0.f;
	Vec3 m_vLocation;
};

#ifdef DEBUG_TEXT
struct DebugText_t
{
	std::string m_sText = "";
	Color_t m_tColor = {};
	std::optional<Vec2> vPosition2D = std::nullopt;
	std::optional<Vec3> vPosition3D = std::nullopt;
};
#endif

class CVisuals
{
private:
	std::unordered_map<CBaseEntity*, Projectile_t> m_mProjectiles = {};
	std::vector<Sightline_t> m_vSightLines = {};
	std::vector<PickupData_t> m_vPickups = {};

	std::unordered_map<int, PendingHitData_t> m_mPendingHits = {};

	std::unordered_map<int, Vec3> m_mItemPositions;

#ifdef DEBUG_TEXT
	std::vector<DebugText_t> m_vDebugText = {};
#endif

public:
	void Event(IGameEvent* pEvent, uint32_t uHash);
	void Store();
	void Tick();

	void ProjectileTrace(CTFPlayer* pPlayer, CTFWeaponBase* pWeapon, const bool bInterp = true);
	void DrawPickupTimers();
	void DrawAntiAim(CTFPlayer* pLocal);
	void DrawDebugInfo(CTFPlayer* pLocal);

#ifdef DEBUG_TEXT
	void AddDebugText(const DebugText_t& sText);
	void AddDebugText(const std::string& sString, Color_t tColor = Vars::Menu::Theme::Active.Value);
	void AddDebugText(const std::string& sString, const Vec2& vPosition, Color_t tColor = Vars::Menu::Theme::Active.Value);
	void AddDebugText(const std::string& sString, const Vec3& vPosition, Color_t tColor = Vars::Menu::Theme::Active.Value);
	void ClearDebugText();
#endif

	std::vector<DrawBox_t> GetHitboxes(matrix3x4* aBones, CBaseAnimating* pEntity, std::vector<int> vHitboxes = {}, int iTarget = -1);
	std::vector<DrawBox_t> GetHitboxes(matrix3x4* aBones, CBaseAnimating* pEntity, std::vector<int> vHitboxes, std::vector<int> vTargets);
	void DrawEffects();
	void DrawHitboxes(int iStore = 0);

	void FOV(CTFPlayer* pLocal, CViewSetup* pView);
	void ThirdPerson(CTFPlayer* pLocal, CViewSetup* pView);

	void OverrideWorldTextures();
	void Modulate();
	void RestoreWorldModulation();

	void CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon);

	void RegisterPendingHit(int iEntIndex, matrix3x4* aBones, int nAimedHitbox, std::vector<int> vAimedHitboxes);

	void ClearFadeHitboxes();
	void AddFadeHitbox(const Vec3& vOrigin, const Vec3& vMins, const Vec3& vMaxs, const Vec3& vAngles, float flTime, const Color_t& tEdge, const Color_t& tFace, bool bZBuffer = true);

	std::vector<DrawBoxFadeBullet_t> m_vBulletBoxes = {};   // Only for bullet tracer boxes

	std::vector<DrawLineFade_t> m_vLines = {};
};

ADD_FEATURE(CVisuals, Visuals);