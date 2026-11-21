#pragma once
#include "ChessTypes.h"

// Platform-specific key handling stays out of ChessGame and ChessRenderer.
enum class InputKey { Up, Down, Left, Right, Enter, Escape, Enchant, Inspect, Help, Save, Menu, Unknown };
class InputManager {
public:
    static InputKey ReadKey();
};
