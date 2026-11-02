#include "EnchantmentManager.h"

int EnchantmentManager::GetCost(PieceType type)
{
    switch (type)
    {
    case PieceType::Pawn: return 10;
    case PieceType::Bishop: return 20;
    case PieceType::Knight: return 25;
    case PieceType::Rook: return 40;
    default: return -1; // Queen spells not designed; kings cannot enchant.
    }
}

bool EnchantmentManager::CanEnchant(const Player& player, PieceType type, int ownTurnNumber)
{
    const int cost = GetCost(type);
    return cost >= 0 && player.GetMana() >= cost && player.CanEnchant(ownTurnNumber);
}

const char* EnchantmentManager::GetAbilityName(PieceType type, Element element)
{
    if (GetCost(type) < 0)
    {
        return "No enchantment available";
    }

    switch (element)
    {
    case Element::Scorch:
        switch (type)
        {
        case PieceType::Pawn: return "Forward Capture";
        case PieceType::Bishop: return "Sacrificial Line Capture";
        case PieceType::Knight: return "Sacrificial Path Capture";
        case PieceType::Rook: return "Destructive Passage";
        default: break;
        }
        break;
    case Element::River:
        switch (type)
        {
        case PieceType::Pawn: return "Backward Diagonal";
        case PieceType::Bishop: return "Rightward Capture";
        case PieceType::Knight: return "Flowing Movement";
        case PieceType::Rook: return "Adjacent Swap";
        default: break;
        }
        break;
    case Element::Zephyr:
        switch (type)
        {
        case PieceType::Pawn: return "Sideways Movement";
        case PieceType::Bishop: return "Single-Piece Jump";
        case PieceType::Knight: return "Extended Leap";
        case PieceType::Rook: return "Friendly-Piece Jump";
        default: break;
        }
        break;
    }

    return "No enchantment available";
}
