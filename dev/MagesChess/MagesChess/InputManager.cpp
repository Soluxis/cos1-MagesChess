#include "InputManager.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <cstdio>
#endif

InputKey InputManager::ReadKey() {
#ifdef _WIN32
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD original = 0;
    if (handle == INVALID_HANDLE_VALUE || !GetConsoleMode(handle, &original)) return InputKey::Menu;
    SetConsoleMode(handle, (original | ENABLE_EXTENDED_FLAGS) & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_QUICK_EDIT_MODE));
    INPUT_RECORD event{}; DWORD count = 0;
    for (;;) {
        if (!ReadConsoleInputW(handle, &event, 1, &count)) { SetConsoleMode(handle, original); return InputKey::Menu; }
        if (event.EventType != KEY_EVENT || !event.Event.KeyEvent.bKeyDown) continue;
        const auto& key = event.Event.KeyEvent;
        InputKey result = InputKey::Unknown;
        switch (key.wVirtualKeyCode) {
        case VK_UP: result = InputKey::Up; break;
        case VK_DOWN: result = InputKey::Down; break;
        case VK_LEFT: result = InputKey::Left; break;
        case VK_RIGHT: result = InputKey::Right; break;
        case VK_RETURN: result = InputKey::Enter; break;
        case VK_ESCAPE: result = InputKey::Escape; break;
        case 'E': result = InputKey::Enchant; break;
        case 'I': result = InputKey::Inspect; break;
        case 'H': result = InputKey::Help; break;
        case 'M': result = InputKey::Menu; break;
        }
        if (result != InputKey::Unknown) { SetConsoleMode(handle, original); return result; }
    }
#else
    termios old{};
    if (tcgetattr(STDIN_FILENO, &old) != 0) return InputKey::Menu;
    termios raw = old; raw.c_lflag &= ~(ICANON | ECHO); raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    int c = std::getchar();
    InputKey result = InputKey::Unknown;
    if (c == 27) {
        termios wait = raw; wait.c_cc[VMIN] = 0; wait.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &wait);
        int next = std::getchar();
        if (next == '[') {
            switch (std::getchar()) { case 'A':result = InputKey::Up; break; case 'B':result = InputKey::Down; break; case 'C':result = InputKey::Right; break; case 'D':result = InputKey::Left; break; }
        }
        else result = InputKey::Escape;
    }
    else if (c == '\r' || c == '\n')result = InputKey::Enter;
    else if (c == 'e' || c == 'E')result = InputKey::Enchant;
    else if (c == 'i' || c == 'I')result = InputKey::Inspect;
    else if (c == 'h' || c == 'H')result = InputKey::Help;
    else if (c == 'm' || c == 'M')result = InputKey::Menu;
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return result;
#endif
}
