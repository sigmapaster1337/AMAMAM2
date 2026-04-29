#pragma once
#include "../../../SDK/SDK.h"
#include <thread>
#include <mutex>

class CChatTranslator
{
private:
	// Anti-spam: track last message per entity index
	struct CachedMessage
	{
		std::string sText;
		float flTime = 0.f;
	};
	std::unordered_map<int, CachedMessage> m_mLastMessages;

	// Pending results to be printed on the main thread
	struct TranslationResult
	{
		std::string sPlayerName;
		std::string sOriginalText;
		std::string sTranslated;
		std::string sSourceLang;
		std::string sTargetLang;
		bool bOutgoing = false;
	};
	std::mutex m_mtxResults;
	std::vector<TranslationResult> m_vResults;
	std::vector<std::string> m_vErrors;

	// Language code table
	struct LangEntry
	{
		const char* szName;
		const char* szCode;
	};
	static const std::vector<LangEntry>& GetLanguages();

	void TranslateAsync(std::string sPlayerName, std::string sOriginalText, std::string sTargetLang, bool bOutgoing = false);

public:
	void Event(IGameEvent* pEvent, uint32_t uNameHash);
	bool SendOutgoing(const char* szText);
	void OnChat(int iIndex, const char* szText);

	// Call from a paint/draw hook to print completed translations
	void Update();
};

ADD_FEATURE(CChatTranslator, ChatTranslator);