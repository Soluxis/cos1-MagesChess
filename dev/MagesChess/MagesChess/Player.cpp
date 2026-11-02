#include "Player.h"
#include <algorithm>

Player::Player(PlayerColor color) : mColor(color) {}

void Player::Reset(Element element)
{
    mElement = element;
    mMana = 50;
    mLastEnchantedTurn = -1;
}

PlayerColor Player::GetColor() const { return mColor; }
Element Player::GetElement() const { return mElement; }
int Player::GetMana() const { return mMana; }

bool Player::CanEnchant(int ownTurnNumber) const
{
    return ownTurnNumber > 1 && mLastEnchantedTurn != ownTurnNumber - 1;
}

bool Player::SpendMana(int amount)
{
    if (amount < 0 || amount > mMana)
    {
        return false;
    }

    mMana -= amount;
    return true;
}

void Player::GainMana(int amount)
{
    if (amount > 0)
    {
        mMana = std::min(MaxMana, mMana + amount);
    }
}

void Player::RecordEnchantment(int ownTurnNumber)
{
    mLastEnchantedTurn = ownTurnNumber;
}
