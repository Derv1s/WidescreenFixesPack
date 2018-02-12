#include "stdafx.h"
#include "dxsdk\d3d8.h"
#include "dxsdk\d3dvtbl.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fDynamicScreenFieldOfViewScale;
    float fFieldOfView;
    float fAspectRatio;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fHudOffsetWide;
    float fWidescreenHudOffset;
    float fBorderOffset;
    float fWidthScale;
    float fHalfWidthScale;
    float f1_fWidthScale;
    float fDouble1_fWidthScale;
    float fHalf1_fWidthScale;
    bool bDrawBorders;
    bool bDrawBordersToFillGap;
} Screen;

struct TextCoords
{
    float a;
    float b;
    float c;
    float d;
};

enum P_D3D_RenderState
{
    GRAPHIC_NOVEL = 0x00FFFFFF,
    GAMEPLAY = 0x00202020
};

typedef HRESULT(STDMETHODCALLTYPE* EndScene_t)(LPDIRECT3DDEVICE8);
EndScene_t RealEndScene = NULL;

void DrawRect(LPDIRECT3DDEVICE8 pDevice, int32_t x, int32_t y, int32_t w, int32_t h, D3DCOLOR color = D3DCOLOR_XRGB(0, 0, 0))
{
    D3DRECT BarRect = { x, y, x + w, y + h };
    pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, color, 0, 0);
}

HRESULT WINAPI EndScene(LPDIRECT3DDEVICE8 pDevice)
{
    if (Screen.bDrawBordersToFillGap)
    {
        //hiding top/left 1px gap
        DrawRect(pDevice, 0, 0, Screen.nWidth, 1);
        DrawRect(pDevice, 0, 0, 1, Screen.nHeight);
        Screen.bDrawBordersToFillGap = false;
    }

    if (Screen.bDrawBorders)
    {
        auto x = Screen.fHudOffsetReal * Screen.fFieldOfView;
        auto y = (x - Screen.fHudOffsetReal) / Screen.fFieldOfView;

        DrawRect(pDevice, 0, 0, static_cast<int32_t>(x), Screen.nHeight);
        DrawRect(pDevice, static_cast<int32_t>(Screen.fWidth - x), 0, static_cast<int32_t>(Screen.fHudOffsetReal + x), Screen.nHeight);

        DrawRect(pDevice, 0, 0, Screen.nWidth, static_cast<int32_t>(y));
        DrawRect(pDevice, 0, static_cast<int32_t>(Screen.fHeight - y), Screen.nWidth, static_cast<int32_t>(y));
    }

    return RealEndScene(pDevice);
}

uint32_t* pRenderState;
uint8_t* bIsInMenu;
bool bGraphicNovelMode;
bool* bIsInCutscene;

DWORD WINAPI GraphicNovelsHandler(LPVOID)
{
    static bool bPatched;
    CIniReader iniReader("");
    int32_t nGraphicNovelModeKey = iniReader.ReadInteger("MAIN", "GraphicNovelModeKey", VK_F2);

    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D E4 D9 45 EC");
    uintptr_t e2mfc_49E00 = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D E0 E8");
    uintptr_t e2mfc_49DFC = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? D8 0D");
    uintptr_t e2mfc_1565E = (uintptr_t)pattern.get_first(6);

    while (true)
    {
        Sleep(0);

        if ((GetAsyncKeyState(nGraphicNovelModeKey) & 1) && (*pRenderState == P_D3D_RenderState::GRAPHIC_NOVEL))
        {
            bGraphicNovelMode = !bGraphicNovelMode;
            bPatched = !bPatched;
            iniReader.WriteInteger("MAIN", "GraphicNovelMode", bGraphicNovelMode);
            while ((GetAsyncKeyState(nGraphicNovelModeKey) & 0x8000) > 0) { Sleep(0); }
        }

        if (bGraphicNovelMode)
        {
            if (*bIsInMenu && *pRenderState == P_D3D_RenderState::GRAPHIC_NOVEL) //+main menu check
            {
                if (!bPatched)
                {
                    injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / (480.0f * Screen.fAspectRatio)) / 2.0f), true);
                    injector::WriteMemory<float>(e2mfc_49DFC, (1.0f / (640.0f / (4.0f / 3.0f)) / 2.0f), true);
                    injector::WriteMemory<float>(e2mfc_1565E, 0.0f, true);
                    bPatched = true;
                }
            }
            else
            {
                if (bPatched)
                {
                    injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / 640.0f) / 2.0f), true);
                    injector::WriteMemory<float>(e2mfc_49DFC, Screen.fHalf1_fWidthScale, true);
                    injector::WriteMemory<float>(e2mfc_1565E, 0.0f, true);
                    bPatched = false;
                }
            }
        }
        else
        {
            if (*bIsInMenu && *pRenderState == P_D3D_RenderState::GRAPHIC_NOVEL) //+main menu check
            {
                if (!bPatched)
                {
                    injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / (480.0f * Screen.fAspectRatio)) / 2.0f) * 1.27f, true);
                    injector::WriteMemory<float>(e2mfc_49DFC, (1.0f / (640.0f / (4.0f / 3.0f)) / 2.0f) * 1.27f, true);
                    injector::WriteMemory<float>(e2mfc_1565E, -0.39f, true);
                    bPatched = true;
                }
            }
            else
            {
                if (bPatched)
                {
                    injector::WriteMemory<float>(e2mfc_49E00, ((1.0f / 640.0f) / 2.0f), true);
                    injector::WriteMemory<float>(e2mfc_49DFC, Screen.fHalf1_fWidthScale, true);
                    injector::WriteMemory<float>(e2mfc_1565E, 0.0f, true);
                    bPatched = false;
                }
            }
        }
    }
    return 0;
}

DWORD WINAPI InitWF(LPVOID)
{
    while (!Screen.nWidth || !Screen.nHeight) { Sleep(0); };

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.fWidthScale = 640.0f / Screen.fAspectRatio;
    Screen.fHalfWidthScale = Screen.fWidthScale / 2.0f;
    Screen.f1_fWidthScale = 1.0f / Screen.fWidthScale;
    Screen.fDouble1_fWidthScale = Screen.f1_fWidthScale * 2.0f;
    Screen.fHalf1_fWidthScale = Screen.f1_fWidthScale / 2.0f;
    Screen.fHudOffset = ((-1.0f / Screen.fAspectRatio) * (4.0f / 3.0f));
    Screen.fHudScale = (1.0f / (480.0f * Screen.fAspectRatio)) * 2.0f;
    Screen.fBorderOffset = (1.0f / Screen.fHudScale) - (640.0f / 2.0f);
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    #undef SCREEN_FOV_HORIZONTAL
    #undef SCREEN_FOV_VERTICAL
    #define SCREEN_FOV_HORIZONTAL 65.0f
    #define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))
    Screen.fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

    if (Screen.fAspectRatio <= ((4.0f / 3.0f) + 0.03f))
        return 0;

    CIniReader iniReader("");
    bool bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    static bool bWidescreenHud = iniReader.ReadInteger("MAIN", "WidescreenHud", 1) != 0;
    Screen.fWidescreenHudOffset = iniReader.ReadFloat("MAIN", "WidescreenHudOffset", 100.0f);
    bGraphicNovelMode = iniReader.ReadInteger("MAIN", "GraphicNovelMode", 1) != 0;
    if (!Screen.fWidescreenHudOffset) { Screen.fWidescreenHudOffset = 100.0f; }
    float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    if (!fFOVFactor) { fFOVFactor = 1.0f; }
    Screen.fDynamicScreenFieldOfViewScale *= fFOVFactor;

    //fix aspect ratio
    //injector::WriteMemory((DWORD)e2mfc + 0x148ED + 0x2, &f1_480, true); //doors fix ???
    //CPatch::SetFloat((DWORD)h_e2mfc_dll + 0x49DE4, height_multipl); //D3DERR_INVALIDCALL
    static uintptr_t e2mfc_14775, e2mfc_1566C, e2mfc_146FA;
    static uintptr_t e2mfc_49DEC, e2mfc_49DFC;

    uintptr_t dword_40B3B2 = (uintptr_t)hook::get_pattern("C7 86 F5 00 00 00 00 00 00 00 5E");
    struct DelayedHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esi + 0xF5) = 0;

            injector::WriteMemory(e2mfc_146FA, &Screen.f1_fWidthScale, true); // D3DERR_INVALIDCALL 5
            injector::WriteMemory(e2mfc_14775, &Screen.f1_fWidthScale, true); // D3DERR_INVALIDCALL 6
            injector::WriteMemory(e2mfc_1566C, &Screen.f1_fWidthScale, true); // D3DERR_INVALIDCALL 9

            injector::WriteMemory<float>(e2mfc_49DEC, Screen.fDouble1_fWidthScale, true);
            injector::WriteMemory<float>(e2mfc_49DFC, Screen.fHalf1_fWidthScale, true);
        }
    }; injector::MakeInline<DelayedHook>(dword_40B3B2, dword_40B3B2 + 10);

    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "BB 00 0C 00 00 25 00 0C 00 00 8B 4D F8");
    auto flt_10049DE4 = *pattern.count(10).get(5).get<uintptr_t>(-15);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), pattern_str(0xD8, 0x0D, to_bytes(flt_10049DE4)));
    while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(9).empty()) { Sleep(0); };
    for (size_t i = 0; i < pattern.size(); i++)
    {
        if (i != 4 && i != 5 && i != 8)
            injector::WriteMemory(pattern.get(i).get<uintptr_t>(2), &Screen.f1_fWidthScale, true);
    }
    e2mfc_146FA = (uintptr_t)pattern.get(4).get<uintptr_t>(2);
    e2mfc_14775 = (uintptr_t)pattern.get(5).get<uintptr_t>(2);
    e2mfc_1566C = (uintptr_t)pattern.get(8).get<uintptr_t>(2);
    
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 1D ? ? ? ? 8A 86 E0");
    e2mfc_49DEC = *pattern.get_first<uintptr_t>(2);
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D E0 E8");
    e2mfc_49DFC = *pattern.get_first<uintptr_t>(2);

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), pattern_str(to_bytes(480.0f))); //0x5ECD00
    while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(6).empty()) { Sleep(0); };
    for (size_t i = 0; i < pattern.size(); i++)
        injector::WriteMemory<float>(pattern.get(i).get<float*>(0), Screen.fWidthScale, true);

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 3D ? ? ? ? D8 6D 0C D9 5D 0C");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fWidthScale, true); //0x10049DDC 480.0f

    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 25 ? ? ? ? D9 86 DC 01 00 00");
    injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHalfWidthScale, true); //0x10049DF0 240.0

    //corrupted graphic in tunnel (DisableSubViewport)
    pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "55 8B EC 83 EC 24 56 8B F1 D9 86 DC 01");
    injector::MakeRET(pattern.get_first(), 0x10, true); //10013FB0 ret 10h
  
    // Hud
    if (bFixHud)
    {
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? 8B 4D F4");
        injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHudOffset, true); //100495C8
        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D8 0D ? ? ? ? D9 5D D8 B9 ? ? ? ? 8B 45 D8 8B 55 D4");
        injector::WriteMemory<float>(*pattern.get_first<float**>(2), Screen.fHudScale, true); //100495D0

        pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? D9 E0 D9 45 FC D8 25");
        static float* pHudElementPosX = *pattern.count(2).get(1).get<float*>(2); //0x10065190
        static float* pHudElementPosY = *pattern.count(2).get(1).get<float*>(22); //0x10065194
        struct P_HudPosHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float ElementPosX = *pHudElementPosX;
                float ElementPosY = *pHudElementPosY;
                float ElementNewPosX1 = ElementPosX;
                float ElementNewPosY1 = ElementPosY;
                float ElementNewPosX2 = ElementPosX;
                float ElementNewPosY2 = ElementPosY;
                if (!*bIsInCutscene)
                    Screen.bDrawBorders = false;

                if (bWidescreenHud)
                {
                    if (ElementPosX == 7.0f) // bullet time meter
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }

                    if (ElementPosX == 8.0f && regs.eax != 8) // bullet time overlay()
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }

                    if (ElementPosX == 12.0f) // painkillers
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }

                    if (ElementPosX == 22.5f) //health bar and overlay
                    {
                        ElementNewPosX1 = ElementPosX + Screen.fHudOffsetWide;
                    }

                    if (ElementPosX == 95.0f) // other weapons name
                    {
                        ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                    }

                    if (ElementPosX == 190.0f) //molotovs/grenades name pos
                    {
                        ElementNewPosX1 = ElementPosX - Screen.fHudOffsetWide;
                    }
                }

                ElementNewPosX2 = ElementNewPosX1;

                if (ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == 2 && *(float*)&regs.edx == 640.0f) // fading
                {
                    ElementNewPosX1 = ElementPosX + 640.0f;
                    ElementNewPosX2 = ElementPosX - 640.0f;
                    //ElementNewPosY1 = ElementPosY + 48.0f;
                    //ElementNewPosY2 = ElementPosY - 48.0f;
                }
                else if ((pRenderState && *pRenderState == P_D3D_RenderState::GRAPHIC_NOVEL) && *bIsInMenu) //+main menu check
                {
                    if (ElementPosX == 0.0f && ElementPosY == 100.0f /*&& regs.eax == 2*/ /*&& *(float*)&regs.edx == 80.0f*/) // graphic novels controls and background
                    {
                        if (bGraphicNovelMode)
                        {
                            ElementNewPosY1 = ElementPosY - 90.0f;
                            ElementNewPosY2 = ElementPosY - 90.0f;
                        }
                        else
                        {
                            ElementNewPosY1 = ElementPosY - 160.0f;
                            ElementNewPosY2 = ElementPosY - 160.0f;
                        }
                    }
                
                    if (ElementPosX == 0.0f && ElementPosY == 0.0f && regs.eax == 2 && (*(float*)&regs.edx == 80.0f || *(float*)&regs.edx == 60.0f)) // graphic novels controls and background
                    {
                        if (bGraphicNovelMode)
                        {
                            ElementNewPosY1 = ElementPosY - 60.0f;
                            ElementNewPosY2 = ElementPosY - 60.0f;
                        }
                        else
                        {
                            ElementNewPosY1 = ElementPosY - 160.0f;
                            ElementNewPosY2 = ElementPosY - 160.0f;
                        }
                    }
                }
                else if (ElementPosX == 100.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f)) //sniper scope borders left side
                {
                    Screen.bDrawBordersToFillGap = true;
                    ElementNewPosX1 += Screen.fBorderOffset;
                }
                else if (ElementPosX == 0.0f && (ElementPosY == 0.0f || ElementPosY == 20.0f || ElementPosY == 220.0f) && *(float*)&regs.edx == 100.0f) //sniper scope borders right side
                {
                    Screen.bDrawBordersToFillGap = true;
                    ElementNewPosX2 -= Screen.fBorderOffset;
                }
                
                *(float *)(regs.ebp - 4) -= ElementNewPosX2;
                *(float *)(regs.ebp - 8) -= ElementNewPosY2;

                _asm
                {
                    fld     dword ptr[ElementNewPosX1]
                    fchs
                    fld     dword ptr[ElementNewPosY1]
                    fchs
                }
            }
        }; injector::MakeInline<P_HudPosHook>(pattern.count(2).get(1).get<uintptr_t>(0), pattern.count(2).get(1).get<uintptr_t>(40)); //1000856C


        if (bWidescreenHud)
        {
            Screen.fHudOffsetWide = Screen.fWidescreenHudOffset;
        
            if (Screen.fAspectRatio < (16.0f / 9.0f))
            {
                Screen.fHudOffsetWide = Screen.fWidescreenHudOffset / (((16.0f / 9.0f) / (Screen.fAspectRatio)) * 1.5f);
            }
        
            pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "D9 05 ? ? ? ? D8 8E 74 01 00 00");
            static auto pTextElementPosX = *pattern.get_first<TextCoords*>(2); //0x100647D0
            struct P_TextPosHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    auto TextPosX = pTextElementPosX->a;
                    auto TextNewPosX = TextPosX;

                    if ((pTextElementPosX->a == 0.0f || pTextElementPosX->a == -8.0f || pTextElementPosX->a == -16.0f || pTextElementPosX->a == -24.0f || pTextElementPosX->a == -32.0f) && pTextElementPosX->b == -10.5f && (pTextElementPosX->c == 8.0f || pTextElementPosX->c == 16.0f || pTextElementPosX->c == 24.0f || pTextElementPosX->c == 32.0f) && pTextElementPosX->d == 21) //ammo numbers(position depends on digits amount)
                        TextNewPosX = TextPosX + Screen.fHudOffsetWide;

                    _asm fld    dword ptr[TextNewPosX]
                }
            }; injector::MakeInline<P_TextPosHook>(pattern.get_first(0), pattern.get_first(6));

            static float TextPosX1, TextPosX2, TextPosY1;
            pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "C7 45 D0 00 00 00 00 D9 5D"); //100045FC
            struct P_TextPosHook2
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(float*)(regs.ebp - 0x30) = 0.0f;
                    TextPosX1 = *(float*)(regs.ebp - 0x28);
                    TextPosY1 = *(float*)(regs.ebp - 0x2C);
                }
            }; injector::MakeInline<P_TextPosHook2>(pattern.get_first(0), pattern.get_first(7));

            pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "89 41 08 D9 45 E4 D8 0D"); //0x10004693
            struct P_TextPosHook3
            {
                void operator()(injector::reg_pack& regs)
                {
                    TextPosX2 = *(float*)(regs.ebp - 0x1C);

                    if (TextPosX1 == (69.0f + Screen.fHudOffsetWide) && TextPosY1 == 457.0f) // painkillers amount number
                        *(float*)(regs.ebp - 0x1C) += (24.0f * Screen.fHudOffsetWide);

                    *(uint32_t*)(regs.ecx + 8) = regs.eax;
                    auto ebp1C = *(float*)(regs.ebp - 0x1C);
                    _asm fld  dword ptr[ebp1C]
                }
            }; injector::MakeInline<P_TextPosHook3>(pattern.get_first(0), pattern.get_first(6));
        }
    }

    return 0;
}

DWORD WINAPI InitE2MFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("e2mfc"), "E8 ? ? ? ? 8B 0D ? ? ? ? 89 45 DC 89 5D E0");

    if (pattern.count_hint(2).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitE2MFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("e2mfc")).count_hint(2).empty()) { Sleep(0); };

    auto PDriverGetWidth = [](uintptr_t P_Driver__m_initializedDriver, uintptr_t edx) -> int32_t
    {
        if (*(uintptr_t*)(P_Driver__m_initializedDriver + 48))
            Screen.nWidth = *(int32_t*)(P_Driver__m_initializedDriver + 52);
        else
            Screen.nWidth = *(int32_t*)(P_Driver__m_initializedDriver + 4);

        return Screen.nWidth;
    };

    auto PDriverGetHeight = [](uintptr_t P_Driver__m_initializedDriver, uintptr_t edx) -> int32_t
    {
        if (*(uintptr_t*)(P_Driver__m_initializedDriver + 48))
            Screen.nHeight = *(int32_t *)(P_Driver__m_initializedDriver + 56);
        else
            Screen.nHeight = *(int32_t *)(P_Driver__m_initializedDriver + 8);

        return Screen.nHeight;
    };

    //get resolution
    injector::MakeCALL(pattern.get(0).get<uintptr_t>(0), static_cast<int32_t(__fastcall *)(uintptr_t, uintptr_t)>(PDriverGetWidth), true);   //e2mfc + 0x15582
    injector::MakeCALL(pattern.get(0).get<uintptr_t>(41), static_cast<int32_t(__fastcall *)(uintptr_t, uintptr_t)>(PDriverGetHeight), true); //e2mfc + 0x155AB

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitWF, NULL, 0, NULL);
    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    bool bUseGameFolderForSavegames = iniReader.ReadInteger("MAIN", "UseGameFolderForSavegames", 0) != 0;
    if (bUseGameFolderForSavegames)
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0x85, true); //0x40FCAB
    
    bool bAltTab = iniReader.ReadInteger("MAIN", "AllowAltTabbingWithoutPausing", 0) != 0;
    if (bAltTab)
    {
        pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 5E C2 08 00");
        injector::MakeNOP(pattern.count(2).get(1).get<uintptr_t>(0), 5, true); //0x40D29B
    }
    
    static int32_t nCutsceneBorders = iniReader.ReadInteger("MAIN", "CutsceneBorders", 1);
    if (nCutsceneBorders)
    {
        auto f = [](uintptr_t _this, uintptr_t edx) -> float
        {
            if (nCutsceneBorders > 1)
                return *(float *)(_this + 12) * (1.0f / ((4.0f / 3.0f) / Screen.fAspectRatio));
            else
                return 1.0f;
        };
        pattern = hook::pattern("E8 ? ? ? ? EB ? D9 05 ? ? ? ? 8B CF");
        injector::MakeCALL(pattern.get_first(), static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(f), true); //0x4565B8
    }

    pattern = hook::pattern("8B 0D ? ? ? ? 8B 3D ? ? ? ? 41 89 0D ? ? ? ? 6A 18 8B CE E8");
    bIsInMenu = *pattern.count(8).get(1).get<uint8_t*>(2); //0x100845E8
    bIsInCutscene = *hook::get_pattern<bool*>("A0 ? ? ? ? 84 C0 0F 85 ? ? ? ? 8B 86 ? ? ? ? 85 C0 0F 84", 1);

    //FOV
    static auto FOVHook = [](uintptr_t _this, uintptr_t edx) -> float
    {
        Screen.fFieldOfView = *(float*)(_this + 88) * Screen.fDynamicScreenFieldOfViewScale;

        if (Screen.fFieldOfView > 2.0f)
            Screen.fFieldOfView = 2.0f;

        return Screen.fFieldOfView;
    };

    pattern = hook::pattern("E8 ? ? ? ? D8 8B 3C 12"); //0x50B9E0
    auto sub_50B9E0 = injector::GetBranchDestination(pattern.get_first(), true);
    pattern = hook::pattern("E8 ? ? ? ?");
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        auto addr = pattern.get(i).get<uint32_t>(0);
        auto dest = injector::GetBranchDestination(addr, true);
        if (dest == sub_50B9E0)
            injector::MakeCALL(addr, static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(FOVHook), true);
    }
    pattern = hook::pattern("E8 ? ? ? ? D9 5C 24 14 8B CF E8"); // 0x45650D
    injector::MakeCALL(pattern.get_first(0), sub_50B9E0, true); // restoring cutscene FOV

    bool bD3DHookBorders = iniReader.ReadInteger("MAIN", "D3DHookBorders", 1) != 0;
    if (bD3DHookBorders)
    {
        auto CutsceneFOVHook = [](uintptr_t _this, uintptr_t edx) -> float
        {
            if (Screen.fAspectRatio > (16.0f / 9.0f))
                return *(float*)(_this + 88);
            else
                return FOVHook(_this, edx);
        };
        injector::MakeCALL(pattern.get_first(0), static_cast<float(__fastcall *)(uintptr_t, uintptr_t)>(CutsceneFOVHook), true);

        pattern = hook::pattern("8B 4D 08 8B 16 83 C4 14 51 8B CE 8B F8 FF 52 10");
        struct CameraOverlayHook
        {
            void operator()(injector::reg_pack& regs)
            {
                Screen.bDrawBorders = false;
                regs.ecx = *(uint32_t*)(regs.ebp + 0x08);
                regs.edx = *(uint32_t*)(regs.esi);

                if (*(uint32_t*)(regs.esp + 0x2C) == 191 && (Screen.fAspectRatio <= (16.0f / 9.0f))) // looks like cutscene id or idk
                    Screen.bDrawBorders = true;
            }
        }; injector::MakeInline<CameraOverlayHook>(pattern.count(201).get(168).get<void>(0)); // 0x703861
    }

    return 0;
}

DWORD WINAPI InitE2_D3D8_DRIVER_MFC(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "8B 45 0C 53 56 33 F6 2B C6 57");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&InitE2_D3D8_DRIVER_MFC, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("e2_d3d8_driver_mfc")).count_hint(1).empty()) { Sleep(0); };

    pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "8B 96 ? ? ? ? 85 D2 74 ? 39 9E");
    pRenderState = (uint32_t*)(*pattern.get_first<uintptr_t>(2) + 0x680); //0x100845E8 (0x10083F68 + 0x680)

    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&GraphicNovelsHandler, NULL, 0, NULL);

    //D3D Hook for borders
    CIniReader iniReader("");
    bool bD3DHookBorders = iniReader.ReadInteger("MAIN", "D3DHookBorders", 1) != 0;
    if (bD3DHookBorders)
    {
        pattern = hook::module_pattern(GetModuleHandle("e2_d3d8_driver_mfc"), "8B 86 ? ? ? ? 8B 08 50 FF 91");
        struct EndSceneHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.esi + 0xD0);

                if (!RealEndScene)
                {
                    auto addr = *(uint32_t*)(regs.eax) + (IDirect3DDevice8VTBL::EndScene * 4);
                    RealEndScene = *(EndScene_t*)addr;
                    injector::WriteMemory(addr, &EndScene, true);
                }
            }
        }; injector::MakeInline<EndSceneHook>(pattern.get_first(0), pattern.get_first(6)); //0x1002856D 
    }

    return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
        InitE2MFC(NULL);
        InitE2_D3D8_DRIVER_MFC(NULL);
    }
    return TRUE;
}