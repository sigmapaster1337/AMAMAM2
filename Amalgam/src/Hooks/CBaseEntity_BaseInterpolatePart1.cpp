#include "../SDK/SDK.h"

#include "../Features/Ticks/Ticks.h"

MAKE_SIGNATURE(CBaseEntity_BaseInterpolatePart1, "client.dll", "48 89 5C 24 ? 56 57 41 55 41 56 41 57 48 83 EC ? 4C 8B BC 24", 0x0);

MAKE_HOOK(CBaseEntity_BaseInterpolatePart1, S::CBaseEntity_BaseInterpolatePart1(), int,
	void* rcx, float& currentTime, Vector& oldOrigin, QAngle& oldAngles, Vector& oldVel, int& bNoMoreChanges)
{
#ifdef DEBUG_HOOKS
	if (!Vars::Hooks::CBaseEntity_BaseInterpolatePart1[DEFAULT_BIND])
		return CALL_ORIGINAL(rcx, currentTime, oldOrigin, oldAngles, oldVel, bNoMoreChanges);
#endif

	auto pEntity = reinterpret_cast<CBaseEntity*>(rcx);
	if (pEntity && pEntity->GetClassID() == ETFClassID::CTFViewModel && F::Ticks.m_bRecharge)
	{
		bNoMoreChanges = 1;
		return 0;
	}

	// Disable interpolation for all non-local players.
	// This makes enemies snap to their real networked position instead of
	// smoothly interpolating between updates. At high ping + fakelag,
	// interpolation puts enemies in wrong positions that don't match
	// where the server will lag-compensate them to.
	if (pEntity && pEntity->IsPlayer())
	{
		auto pLocal = H::Entities.GetLocal();
		if (pEntity != pLocal)
		{
			bNoMoreChanges = 1;
			return 0;
		}
	}

	return CALL_ORIGINAL(rcx, currentTime, oldOrigin, oldAngles, oldVel, bNoMoreChanges);
}