#pragma once
#include "../../../Utils/Macros/Macros.h"
#include "../../Definitions/Classes.h"
#include "../../Vars.h"
#include "../../Definitions/Interfaces/CTFGCClientSystem.h"

Enum(Entity,
	PlayerAll, PlayerEnemy, PlayerTeam,
	BuildingAll, BuildingEnemy, BuildingTeam,
	PickupHealth, PickupAmmo, PickupMoney, PickupPowerup, PickupSpellbook, PickupGargoyle,
	WorldProjectile, WorldObjective, WorldNPC, WorldBomb,
	LocalStickies, LocalFlares, SniperDots,
	Invalid, GroupsMax
)

struct DormantData
{
	Vec3 m_vLocation;
	float m_flLastUpdate = 0.f;
	float m_flReceiveTime = 0.f;
	int m_iFlags = 0;
	bool m_bFromSound = false;
};

struct VelFixRecord
{
	Vec3 m_vecOrigin;
	float m_flSimulationTime;
};

class CEntities
{
private:
	bool ManageDormancy(CBaseEntity* pEntity);

	CTFPlayer* m_pLocal = nullptr;
	CTFWeaponBase* m_pLocalWeapon = nullptr;
	CTFPlayerResource* m_pPlayerResource = nullptr;

	std::array<std::vector<CBaseEntity*>, EntityEnum::GroupsMax> m_aGroups = {};

	std::array<float, MAX_PLAYERS> m_aDeltaTimes = {}, m_aLagTimes = {};
	std::array<int, MAX_PLAYERS> m_aChokes = {}, m_aSetTicks = {};
	std::array<Vec3, MAX_PLAYERS> m_aOldAngles = {}, m_aEyeAngles = {};
	std::array<bool, MAX_PLAYERS> m_aLagCompensation = {};
	std::array<Vec3, MAX_PLAYERS> m_aAvgVelocities = {};
	std::array<std::deque<VelFixRecord>, MAX_PLAYERS> m_aOrigins = {};
	std::array<uint32_t, MAX_EDICTS> m_aModels = {};
	std::array<bool, MAX_EDICTS> m_aDormancy = {};

	std::unordered_map<int, int> m_mIPriorities = {};
	std::unordered_map<uint32_t, int> m_mUPriorities = {};
	std::unordered_map<int, bool> m_mIFriends = {};
	std::unordered_map<uint32_t, bool> m_mUFriends = {};
	std::unordered_map<int, int> m_mIParty = {};
	std::unordered_map<uint32_t, int> m_mUParty = {};
	std::unordered_map<int, bool> m_mIF2P = {};
	std::unordered_map<uint32_t, bool> m_mUF2P = {};
	std::unordered_map<int, int> m_mILevels = {};
	std::unordered_map<uint32_t, int> m_mULevels = {};
	uint32_t m_uAccountID;
	int m_iPartyCount = 0;

	void PollActiveSounds();

public:
	void Store();
	void Clear(bool bShutdown = false);
	void ManualNetwork(const StartSoundParams_t& params);

	bool IsHealth(uint32_t uHash);
	bool IsAmmo(uint32_t uHash);
	bool IsPowerup(uint32_t uHash);
	bool IsSpellbook(uint32_t uHash);

	CTFPlayer* GetLocal();
	CTFWeaponBase* GetWeapon();
	CTFPlayerResource* GetResource();

	const std::vector<CBaseEntity*>& GetGroup(byte iGroup);

	float GetDeltaTime(byte iIndex);
	float GetLagTime(byte iIndex);
	int GetChoke(byte iIndex);
	Vec3 GetEyeAngles(byte iIndex);
	Vec3 GetDeltaAngles(byte iIndex);
	bool GetLagCompensation(byte iIndex);
	void SetLagCompensation(byte iIndex, bool bLagComp);
	Vec3* GetAvgVelocity(byte iIndex);
	void SetAvgVelocity(byte iIndex, Vec3 vAvgVelocity);
	std::deque<VelFixRecord>* GetOrigins(byte iIndex);
	uint32_t GetModel(unsigned short iIndex);
	bool GetDormancy(unsigned short iIndex);
	DormantData* GetDormancyData(unsigned short iIndex);

	int GetPriority(int iIndex);
	int GetPriority(uint32_t uAccountID);
	bool IsFriend(int iIndex);
	bool IsFriend(uint32_t uAccountID);
	bool InParty(int iIndex);
	bool InParty(uint32_t uAccountID);
	bool IsF2P(int iIndex);
	bool IsF2P(uint32_t uAccountID);
	int GetLevel(int iIndex);
	int GetLevel(uint32_t uAccountID);
	int GetParty(int iIndex);
	int GetParty(uint32_t uAccountID);
	int GetPartyCount();

	uint32_t GetAccountID(int iIndex)
	{
		auto pResource = GetResource();
		if (pResource && pResource->m_bValid(iIndex) && !pResource->IsFakePlayer(iIndex))
			return pResource->m_iAccountID(iIndex);
		return 0;
	}

	int GetLobbyPlayerTeam(uint32_t uAccountID);
	TF_GC_TEAM GetGCLobbyPlayerTeam(uint32_t uAccountID);
};

ADD_FEATURE_CUSTOM(CEntities, Entities, H);