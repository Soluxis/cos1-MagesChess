#pragma once

#include "ChessTypes.h"
#include "Player.h"

class EnchantmentManager
{
public:
    static int GetCost(PieceType type);
    static bool CanEnchant(const Player& player, PieceType type, int ownTurnNumber);
    static const char* GetAbilityName(PieceType type, Element element);
};
