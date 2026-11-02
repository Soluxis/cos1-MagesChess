#include "ChessRenderer.h"
#include "EnchantmentManager.h"
#include <cctype>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    void SetConsoleColor(int foreground, int background = 0)
    {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
            static_cast<WORD>(foreground | (background << 4)));
#else
        (void)foreground;
        (void)background;
#endif
    }

    void ResetConsoleColor()
    {
        SetConsoleColor(7);
    }
}

void ChessRenderer::ClearScreen() const
{
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(output, &info)) return;

    const DWORD cells = static_cast<DWORD>(info.dwSize.X) * info.dwSize.Y;
    DWORD written = 0;
    const COORD home{ 0, 0 };
    FillConsoleOutputCharacterA(output, ' ', cells, home, &written);
    FillConsoleOutputAttribute(output, info.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(output, home);
#else
    std::cout << "\033[2J\033[H";
#endif
}

void ChessRenderer::ShowMainMenu() const
{
    std::cout << "\n"
        << "  =======================================\n"
        << "             MAGE'S CHESS\n"
        << "  =======================================\n"
        << "       Strategy meets elemental magic\n\n"
        << "       [1] Start New Game\n"
        << "       [0] Exit\n\n"
        << "  =======================================\n";
}

void ChessRenderer::ShowElementMenu(PlayerColor color) const
{
    std::cout << "\n  " << ToString(color) << " - Choose Your Element\n"
        << "  ---------------------------------------\n"
        << "    [1] Scorch  - Aggression and sacrifice\n"
        << "    [2] River   - Adaptation and defense\n"
        << "    [3] Zephyr  - Mobility and freedom\n"
        << "  ---------------------------------------\n";
}

char ChessRenderer::GetPieceSymbol(const Piece& piece) const
{
    char symbol = '.';
    switch (piece.type)
    {
    case PieceType::Pawn: symbol = 'P'; break;
    case PieceType::Knight: symbol = 'N'; break;
    case PieceType::Bishop: symbol = 'B'; break;
    case PieceType::Rook: symbol = 'R'; break;
    case PieceType::Queen: symbol = 'Q'; break;
    case PieceType::King: symbol = 'K'; break;
    default: break;
    }

    return piece.color == PlayerColor::Black
        ? static_cast<char>(std::tolower(static_cast<unsigned char>(symbol))) : symbol;
}

void ChessRenderer::ShowBoard(const ChessBoard& board, const Player& white,
    const Player& black, const std::string& message) const
{
    std::cout << "\n  MAGE'S CHESS\n"
        << "  ---------------------------------------\n"
        << "  BLACK: " << ToString(black.GetElement())
        << " | Mana: " << black.GetMana() << "/100\n\n"
        << "       a  b  c  d  e  f  g  h\n";

    for (int row = 0; row < ChessBoard::Size; ++row)
    {
        std::cout << "    " << (8 - row) << " ";
        for (int column = 0; column < ChessBoard::Size; ++column)
        {
            const Piece& piece = board.GetPiece({ row, column });
            const bool light = (row + column) % 2 == 0;
            const int background = light ? 7 : 8;
            const int foreground = piece.type == PieceType::None ? background
                : (piece.color == PlayerColor::White ? 15 : 0);
            SetConsoleColor(foreground, background);
            std::cout << ' ' << GetPieceSymbol(piece) << ' ';
            ResetConsoleColor();
        }
        std::cout << ' ' << (8 - row) << '\n';
    }

    std::cout << "       a  b  c  d  e  f  g  h\n\n"
        << "  WHITE: " << ToString(white.GetElement())
        << " | Mana: " << white.GetMana() << "/100\n"
        << "  ---------------------------------------\n"
        << "  Uppercase = White | Lowercase = Black\n"
        << "  " << message << '\n';
}

void ChessRenderer::ShowSquareDetails(Position position, const Piece& piece,
    const Player& owner) const
{
    const std::string square = std::string(1, static_cast<char>('a' + position.column))
        + std::to_string(8 - position.row);

    std::cout << "\n  Square " << square << ": " << ToString(owner.GetColor())
        << ' ' << ToString(piece.type) << '\n';

    const int cost = EnchantmentManager::GetCost(piece.type);
    if (cost >= 0)
    {
        std::cout << "  Element: " << ToString(owner.GetElement()) << '\n'
            << "  Ability: " << EnchantmentManager::GetAbilityName(piece.type, owner.GetElement()) << '\n'
            << "  Enchantment cost: " << cost << " mana\n";
    }
    else
    {
        std::cout << "  Enchantment: unavailable\n";
    }
}

void ChessRenderer::ShowMessage(const std::string& message) const
{
    std::cout << "\n  " << message << '\n';
}
