#pragma once





struct InputExitState
{
    POINT m_initialMousePosition { };
    int   m_exitThresholdPixels  { 5 };
    bool  m_keyboardTriggered    { false };
    bool  m_mouseTriggered       { false };
};
