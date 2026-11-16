#pragma once
#include "ChessTypes.h"
#include "ChessBoard.h"
#include "Player.h"
#include <vector>

struct SpellResult {
    ChessBoard board;
    int enemyLost = 0;
    int friendlyLost = 0;
};

class EnchantmentManager {
public:
    static int GetCost(PieceType type);
    static bool CanEnchant(const Player& player, PieceType type, int ownTurnNumber);
    static const char* GetAbilityName(PieceType type, Element element);
    static std::vector<Move> LegalMoves(const ChessBoard& board, const Player& player, int ownTurnNumber);
    static bool TryApply(const ChessBoard& board, const Move& move, PlayerColor side, Element element, SpellResult& result);
};
