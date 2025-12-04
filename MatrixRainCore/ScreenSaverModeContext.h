#pragma once

#include "ScreenSaverMode.h"





struct ScreenSaverModeContext
{
    ScreenSaverMode  m_mode              { ScreenSaverMode::Normal };
    HWND             m_previewParentHwnd { nullptr                 };
    bool             m_enableHotkeys     { false                   };
    bool             m_hideCursor        { false                   };
    bool             m_exitOnInput       { false                   };
    bool             m_suppressDebug     { false                   };
    bool             m_spanAllDisplays   { false                   };
    std::wstring     m_errorMessage;
};
