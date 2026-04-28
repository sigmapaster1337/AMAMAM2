#pragma once
#include "../Misc/CUtlVector.h"
#include "../Types.h"

struct SndInfo_t
{
	int m_nGuid;
	void* m_filenameHandle;
	int m_nSoundSource;
	int m_nChannel;
	int m_nSpeakerEntity;
	float m_flVolume;
	float m_flLastSpatializedVolume;
	float m_flRadius;
	int m_nPitch;
	Vec3* m_pOrigin;
	Vec3* m_pDirection;
	bool m_bUpdatePositions;
	bool m_bIsSentence;
	bool m_bDryMix;
	bool m_bSpeaker;
	bool m_bSpecialDSP;
	bool m_bFromServer;
};

class IEngineSound
{
public:
	virtual bool PrecacheSound(const char* pSample, bool bPreload = false, bool bIsUISound = false) = 0;
	virtual bool IsSoundPrecached(const char* pSample) = 0;
	virtual void PrefetchSound(const char* pSample) = 0;
	virtual float GetSoundDuration(const char* pSample) = 0;
	virtual void EmitSound(void* filter, int iEntIndex, int iChannel, const char* pSample, float flVolume, float flAttenuation, int iFlags = 0, int iPitch = 100, int iSpecialDSP = 0, const Vec3* pOrigin = nullptr, const Vec3* pDirection = nullptr, void* pUtlVecOrigins = nullptr, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;
	virtual void EmitSound(void* filter, int iEntIndex, int iChannel, const char* pSample, float flVolume, int iSoundlevel, int iFlags = 0, int iPitch = 100, int iSpecialDSP = 0, const Vec3* pOrigin = nullptr, const Vec3* pDirection = nullptr, void* pUtlVecOrigins = nullptr, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;
	virtual void EmitSentenceByIndex(void* filter, int iEntIndex, int iChannel, int iSentenceIndex, float flVolume, int iSoundlevel, int iFlags = 0, int iPitch = 100, int iSpecialDSP = 0, const Vec3* pOrigin = nullptr, const Vec3* pDirection = nullptr, void* pUtlVecOrigins = nullptr, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;
	virtual void StopSound(int iEntIndex, int iChannel, const char* pSample) = 0;
	virtual void StopAllSounds(bool bClearBuffers) = 0;
	virtual void SetRoomType(void* filter, int roomType) = 0;
	virtual void SetPlayerDSP(void* filter, int dspType, bool fastReset) = 0;
	virtual void EmitAmbientSound(const char* pSample, float flVolume, int iPitch = 100, int flags = 0, float soundtime = 0.0f) = 0;
	virtual float GetDistGainFromSoundLevel(int soundlevel, float dist) = 0;
	virtual int GetGuidForLastSoundEmitted() = 0;
	virtual bool IsSoundStillPlaying(int guid) = 0;
	virtual void StopSoundByGuid(int guid) = 0;
	virtual void SetVolumeByGuid(int guid, float fvol) = 0;
	virtual void GetActiveSounds(CUtlVector<SndInfo_t>& sndlist) = 0;
	virtual void PrecacheSentenceGroup(const char* pGroupName) = 0;
	virtual void NotifyBeginMoviePlayback() = 0;
	virtual void NotifyEndMoviePlayback() = 0;
};

MAKE_INTERFACE_VERSION(IEngineSound, EngineSound, "engine.dll", "IEngineSoundClient003");