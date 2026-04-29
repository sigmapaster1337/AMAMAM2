#include "ChatTranslator.h"
#include <wininet.h>
#pragma comment(lib, "Wininet.lib")

// Language table
const std::vector<CChatTranslator::LangEntry>& CChatTranslator::GetLanguages()
{
	static const std::vector<LangEntry> vLanguages = {
		{ "English", "en" }, { "Russian", "ru" }, { "German", "de" }, { "French", "fr" },
		{ "Spanish", "es" }, { "Portuguese", "pt" }, { "Chinese (Simplified)", "zh" },
		{ "Chinese (Traditional)", "zh-tw" }, { "Japanese", "ja" }, { "Korean", "ko" },
		{ "Turkish", "tr" }, { "Polish", "pl" }, { "Italian", "it" }, { "Dutch", "nl" },
		{ "Swedish", "sv" }, { "Norwegian", "no" }, { "Danish", "da" }, { "Finnish", "fi" },
		{ "Czech", "cs" }, { "Romanian", "ro" }, { "Hungarian", "hu" }, { "Ukrainian", "uk" },
		{ "Arabic", "ar" }, { "Thai", "th" }, { "Vietnamese", "vi" }, { "Indonesian", "id" },
		{ "Hindi", "hi" }, { "Bulgarian", "bg" }, { "Greek", "el" }, { "Croatian", "hr" },
		{ "Serbian", "sr" }, { "Slovak", "sk" }, { "Slovenian", "sl" }, { "Estonian", "et" },
		{ "Latvian", "lv" }, { "Lithuanian", "lt" }
	};
	return vLanguages;
}

static std::string UrlEncode(const std::string& sValue)
{
	std::string sResult;
	sResult.reserve(sValue.size() * 3);
	for (unsigned char c : sValue)
	{
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
			sResult += c;
		else
		{
			char hex[4];
			sprintf_s(hex, "%%%02X", c);
			sResult += hex;
		}
	}
	return sResult;
}

void CChatTranslator::Event(IGameEvent* pEvent, uint32_t uNameHash) {}

void CChatTranslator::OnChat(int iIndex, const char* szText)
{
	if (!Vars::Misc::ChatTranslator::Enabled.Value || !szText)
		return;

	if (iIndex == 0 || iIndex == I::EngineClient->GetLocalPlayer())
		return;

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return;

	std::string sFullText = szText;
	std::string sPlayerName = pResource->GetName(iIndex);

	size_t iNamePos = sFullText.find(sPlayerName);
	if (iNamePos == std::string::npos)
		return;

	size_t iColonPos = sFullText.find(" : ", iNamePos + sPlayerName.length() - 1);
	if (iColonPos == std::string::npos)
		return;

	std::string sOriginalText = sFullText.substr(iColonPos + 3);
	if (sOriginalText.empty())
		return;

	float flNow = I::GlobalVars->realtime;
	auto it = m_mLastMessages.find(iIndex);
	if (it != m_mLastMessages.end())
	{
		if (it->second.sText == sOriginalText && flNow < it->second.flTime + 0.5f)
			return;
	}
	m_mLastMessages[iIndex] = { sOriginalText, flNow };

	const auto& vLangs = GetLanguages();
	int iLangIdx = Vars::Misc::ChatTranslator::TargetLanguage.Value;
	if (iLangIdx < 0 || iLangIdx >= static_cast<int>(vLangs.size()))
		iLangIdx = 0;
	std::string sTargetLang = vLangs[iLangIdx].szCode;

	std::thread(&CChatTranslator::TranslateAsync, this, sPlayerName, sOriginalText, sTargetLang, false).detach();
}

bool CChatTranslator::SendOutgoing(const char* szText)
{
	// Log the attempt to console for debugging
	I::CVar->ConsoleColorPrintf({ 150, 200, 255, 255 }, "[ChatTranslator] Checking outgoing: %s\n", szText);

	if (strncmp(szText, ".t ", 3) != 0)
		return false;

	std::string sCmd = szText;
	size_t iFirstSpace = sCmd.find(' ', 3);
	if (iFirstSpace == std::string::npos)
	{
		I::CVar->ConsoleColorPrintf({ 255, 150, 150, 255 }, "[ChatTranslator] Usage: .t <langcode> <message>\n");
		return true; // Mark as handled so the bad command doesn't send
	}

	std::string sTargetLang = sCmd.substr(3, iFirstSpace - 3);
	std::string sOriginalText = sCmd.substr(iFirstSpace + 1);

	if (sOriginalText.empty())
		return false;

	I::CVar->ConsoleColorPrintf({ 150, 255, 150, 255 }, "[ChatTranslator] Translating outgoing to [%s]: %s\n", sTargetLang.c_str(), sOriginalText.c_str());

	std::thread(&CChatTranslator::TranslateAsync, this, "", sOriginalText, sTargetLang, true).detach();

	return true;
}

void CChatTranslator::TranslateAsync(std::string sPlayerName, std::string sOriginalText, std::string sTargetLang, bool bOutgoing)
{
	std::string sPath = std::format(
		"/translate_a/t?client=dict-chrome-ex&sl=auto&tl={}&ie=UTF-8&oe=UTF-8&dt=t&q={}",
		sTargetLang,
		UrlEncode(sOriginalText)
	);

	HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!hInternet)
	{
		std::lock_guard<std::mutex> lock(m_mtxResults);
		m_vErrors.push_back("ChatTranslator: InternetOpen failed");
		return;
	}

	HINTERNET hConnect = InternetConnectA(hInternet, "clients5.google.com", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
	if (!hConnect)
	{
		InternetCloseHandle(hInternet);
		std::lock_guard<std::mutex> lock(m_mtxResults);
		m_vErrors.push_back("ChatTranslator: InternetConnect failed");
		return;
	}

	DWORD dwFlags = INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD;
	HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", sPath.c_str(), nullptr, nullptr, nullptr, dwFlags, 0);
	if (!hRequest)
	{
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		std::lock_guard<std::mutex> lock(m_mtxResults);
		m_vErrors.push_back("ChatTranslator: HttpOpenRequest failed");
		return;
	}

	if (!HttpSendRequestA(hRequest, nullptr, 0, nullptr, 0))
	{
		DWORD dwErr = GetLastError();
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		std::lock_guard<std::mutex> lock(m_mtxResults);
		m_vErrors.push_back(std::format("ChatTranslator: HttpSendRequest failed (err {})", dwErr));
		return;
	}

	std::string sBody;
	char szBuffer[4096];
	DWORD dwBytesRead = 0;
	while (InternetReadFile(hRequest, szBuffer, sizeof(szBuffer) - 1, &dwBytesRead) && dwBytesRead > 0)
	{
		szBuffer[dwBytesRead] = '\0';
		sBody += szBuffer;
		dwBytesRead = 0;
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	if (sBody.empty()) return;

	std::string sTranslated;
	std::string sSourceLang;

	size_t iStart = sBody.find("[[\"");
	if (iStart != std::string::npos)
	{
		iStart += 3;
		size_t iEnd = sBody.find("\"", iStart);
		if (iEnd != std::string::npos)
		{
			sTranslated = sBody.substr(iStart, iEnd - iStart);
			size_t iLangStart = sBody.find("\",\"", iEnd);
			if (iLangStart != std::string::npos)
			{
				iLangStart += 3;
				size_t iLangEnd = sBody.find("\"", iLangStart);
				if (iLangEnd != std::string::npos)
					sSourceLang = sBody.substr(iLangStart, iLangEnd - iLangStart);
			}
		}
	}
	else
	{
		iStart = sBody.find("[\"");
		if (iStart != std::string::npos)
		{
			iStart += 2;
			size_t iEnd = sBody.find("\"", iStart);
			if (iEnd != std::string::npos)
				sTranslated = sBody.substr(iStart, iEnd - iStart);
		}
	}

	if (sTranslated.empty()) return;

	std::string sUnescaped;
	sUnescaped.reserve(sTranslated.size());
	for (size_t i = 0; i < sTranslated.size(); i++)
	{
		if (sTranslated[i] == '\\' && i + 1 < sTranslated.size())
		{
			switch (sTranslated[i + 1])
			{
			case '"': sUnescaped += '"'; i++; break;
			case '\\': sUnescaped += '\\'; i++; break;
			case 'n': sUnescaped += '\n'; i++; break;
			case 't': sUnescaped += '\t'; i++; break;
			case '/': sUnescaped += '/'; i++; break;
			default: sUnescaped += sTranslated[i]; break;
			}
		}
		else sUnescaped += sTranslated[i];
	}

	{
		std::lock_guard<std::mutex> lock(m_mtxResults);
		m_vResults.push_back({
			sPlayerName, sOriginalText, sUnescaped,
			sSourceLang, sTargetLang, bOutgoing
			});
	}
}

void CChatTranslator::Update()
{
	if (!Vars::Misc::ChatTranslator::Enabled.Value)
		return;

	{
		std::lock_guard<std::mutex> lock(m_mtxResults);
		for (const auto& sError : m_vErrors)
		{
			I::CVar->ConsoleColorPrintf({ 255, 200, 100, 255 }, "[ChatTranslator] %s\n", sError.c_str());
		}
		m_vErrors.clear();
	}

	std::vector<TranslationResult> vResults;
	{
		std::lock_guard<std::mutex> lock(m_mtxResults);
		if (m_vResults.empty()) return;
		vResults = std::move(m_vResults);
		m_vResults.clear();
	}

	for (const auto& result : vResults)
	{
		if (result.bOutgoing)
		{
			I::EngineClient->ClientCmd(std::format("say {}", result.sTranslated).c_str());
		}
		else
		{
			// Skip if source language matches target language (no translation needed)
			std::string sSrcLower = result.sSourceLang;
			std::string sTgtLower = result.sTargetLang;
			std::transform(sSrcLower.begin(), sSrcLower.end(), sSrcLower.begin(), ::tolower);
			std::transform(sTgtLower.begin(), sTgtLower.end(), sTgtLower.begin(), ::tolower);
			if (sSrcLower == sTgtLower)
				continue;

			std::string sLangTag;
			if (!result.sSourceLang.empty())
			{
				std::string sSrcUpper = result.sSourceLang;
				std::string sTgtUpper = result.sTargetLang;
				std::transform(sSrcUpper.begin(), sSrcUpper.end(), sSrcUpper.begin(), ::toupper);
				std::transform(sTgtUpper.begin(), sTgtUpper.end(), sTgtUpper.begin(), ::toupper);
				sLangTag = std::format("\x04[{} -> {}] ", sSrcUpper, sTgtUpper);
			}

			std::string sChatMsg = std::format(
				"\x01{}\x03{}\x01: {}",
				sLangTag, result.sPlayerName, result.sTranslated
			);

			I::CVar->ConsoleColorPrintf({ 100, 255, 100, 255 }, "[Translated] %s: %s\n",
				result.sPlayerName.c_str(), result.sTranslated.c_str());

			I::ClientModeShared->m_pChatElement->ChatPrintf(0, sChatMsg.c_str());
		}
	}
}