#include "Fonts.h"
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include "../../Definitions/Interfaces.h"
#include "../../SDK.h"

void CFonts::Reload(float flDPI)
{
    Shutdown();

    //SDK::Output("Amalgam", std::format("Reloading fonts with DPI scale: {:.1f}", flDPI).c_str(), Vars::Menu::Theme::Accent.Value, OUTPUT_CONSOLE);

    m_mFonts[FONT_ESP] = {
        "Tahoma",
        int(13.f * flDPI),
        FONTFLAG_OUTLINE,
        0,
        0
    };

    m_mFonts[FONT_INDICATORS] = {
        "Tahoma",
        int(13.f * flDPI),
        FONTFLAG_OUTLINE,
        0,
        0
    };

    // Create fonts
    for (auto& [eFont, fFont] : m_mFonts)
    {
        const char* szType = (eFont == FONT_ESP) ? "ESP" : "INDICATORS";

        fFont.m_dwFont = I::MatSystemSurface->CreateFont();
        if (!fFont.m_dwFont) {
            SDK::Output("Amalgam", std::format("{}: CreateFont FAILED", szType).c_str(),
                Vars::Menu::Theme::Accent.Value, OUTPUT_CONSOLE);
            continue;
        }

        bool bSuccess = I::MatSystemSurface->SetFontGlyphSet(
            fFont.m_dwFont,
            fFont.m_szName,
            fFont.m_nTall,
            fFont.m_nWeight,
            0, 0,
            fFont.m_nFlags
        );

        if (!bSuccess) {
            SDK::Output("Amalgam", std::format("{}: SetFontGlyphSet FAILED", szType).c_str(),
                Vars::Menu::Theme::Accent.Value, OUTPUT_CONSOLE);
        }
        else {
            int w, h;
            I::MatSystemSurface->GetTextSize(fFont.m_dwFont, L"A", w, h);
        }
    }
}

void CFonts::Shutdown()
{
    for (auto& [eFont, fFont] : m_mFonts)
    {
        if (fFont.m_dwFont)
            fFont.m_dwFont = 0;
    }
    m_mFonts.clear();
}

const Font_t& CFonts::GetFont(EFonts eFont)
{
    static Font_t defaultFont = { "Tahoma", 13, FONTFLAG_OUTLINE, 500, 0 };

    auto it = m_mFonts.find(eFont);
    if (it != m_mFonts.end() && it->second.m_dwFont) {
        return it->second;
    }

    return defaultFont;
}