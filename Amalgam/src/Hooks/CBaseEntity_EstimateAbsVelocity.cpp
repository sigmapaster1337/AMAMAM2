#include "../SDK/SDK.h"

#include "../Features/Resolver/Resolver.h"
#include "../Features/EnginePrediction/EnginePrediction.h"

MAKE_HOOK(CBaseEntity_EstimateAbsVelocity, S::CBaseEntity_EstimateAbsVelocity(), void,
	void* rcx, Vector& vel)
{
	DEBUG_RETURN(CBaseEntity_EstimateAbsVelocity, rcx, vel);

	auto pPlayer = reinterpret_cast<CTFPlayer*>(rcx);
	if (!pPlayer->IsPlayer())
		return CALL_ORIGINAL(rcx, vel);

	if (pPlayer->entindex() == I::EngineClient->GetLocalPlayer())
	{
		vel = pPlayer->m_vecVelocity();
		return;
	}

	// For enemies: try animation-based velocity estimation first.
	// During fakelag, m_vecVelocity is stale (last networked value),
	// but pose parameters are updated every frame by the animation system,
	// giving us a real-time velocity estimate.
	auto pAnimState = pPlayer->m_PlayerAnimState();
	if (pAnimState && !Vars::Visuals::Removals::Interpolation.Value)
	{
		// Get raw pose parameter values (range -1 to 1)
		// These encode direction * (speed / maxAnimationSpeed)
		auto& poses = pPlayer->m_flPoseParameter();
		float flMoveX = poses[pAnimState->m_PoseParameterData.m_iMoveX];
		float flMoveY = poses[pAnimState->m_PoseParameterData.m_iMoveY];

		// Get max speed from animation state movement data
		float flMaxSpeed = pAnimState->m_MovementData.m_flRunSpeed;
		if (flMaxSpeed < 1.f)
			flMaxSpeed = 320.f; // TF2 default max run speed fallback

		// Pose magnitude * max speed = estimated velocity
		float flPoseMag = sqrtf(flMoveX * flMoveX + flMoveY * flMoveY);
		float flSpeed = flPoseMag * flMaxSpeed;

		if (flSpeed > 1.f)
		{
			// Use the animation state's estimated yaw for direction
			float flYaw = pAnimState->m_PoseParameterData.m_flEstimateYaw;
			vel.x = cosf(Math::Deg2Rad(flYaw)) * flSpeed;
			vel.y = sinf(Math::Deg2Rad(flYaw)) * flSpeed;
			vel.z = pPlayer->m_vecVelocity().z; // Z from networked velocity
			return;
		}
	}

	// Fallback: original behavior

	if (!Vars::Visuals::Removals::Interpolation.Value)
	{
		CALL_ORIGINAL(rcx, vel);
		vel.z = pPlayer->m_vecVelocity().z;
	}
	else
		vel = pPlayer->m_vecVelocity();

	if (pPlayer->IsOnGround() && vel.Length2DSqr() < 2.f)
	{
		bool bMinwalk;
		if (F::Resolver.GetAngles(pPlayer, nullptr, nullptr, &bMinwalk) && bMinwalk)
			vel = { 1, 1 };
	}
}