#include "KeyboardInput.h"

void KeyboardInput::Update()
{
    if (GetConsoleWindow() == GetForegroundWindow())
    {
        for (int i = 0; i < 256; i++)
        {
            m_keyNewState[i] = GetAsyncKeyState(i);

            m_keys[i].pressed = false;
            m_keys[i].released = false;

            if (m_keyNewState[i] != m_keyOldState[i])
            {
                if (m_keyNewState[i] & 0x8000)
                {
                    m_keys[i].pressed = !m_keys[i].held;
                    m_keys[i].held = true;
                }
                else
                {
                    m_keys[i].released = true;
                    m_keys[i].held = false;
                }
            }

            m_keyOldState[i] = m_keyNewState[i];
        }
    }
}

const KeyState& KeyboardInput::GetKeyState(int key) const
{
    static KeyState s_dummy;
    return (key >= 0 && key <= 255 ? m_keys[key] : s_dummy);
}
