#pragma once

#include "matrixrain/ScreenSaverMode.h"





struct ScreenSaverModeContext
{
    ScreenSaverMode m_mode              { ScreenSaverMode::Normal };
    HWND            m_previewParentHwnd { nullptr                 };
    std::wstring    m_commandLine;
    bool            m_enableHotkeys     { true                    };
    bool            m_hideCursor        { false                   };
    bool            m_exitOnInput       { false                   };
    bool            m_suppressDebug     { false                   };
    bool            m_spanAllDisplays   { false                   };
};
