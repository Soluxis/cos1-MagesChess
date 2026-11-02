#pragma once

#include "ChessTypes.h"

class Player
{
public:
    explicit Player(PlayerColor color);

    void Reset(Element element);
    PlayerColor GetColor() const;
    Element GetElement() const;
    int GetMana() const;
    bool CanEnchant(int ownTurnNumber) const;
    bool SpendMana(int amount);
    void GainMana(int amount);
    void RecordEnchantment(int ownTurnNumber);

private:
    PlayerColor mColor;
    Element mElement = Element::Scorch;
    int mMana = 50;
    int mLastEnchantedTurn = -1;

    static constexpr int MaxMana = 100;
};
