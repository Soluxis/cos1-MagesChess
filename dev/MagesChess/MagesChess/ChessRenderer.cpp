#include "ChessRenderer.h"
#include "EnchantmentManager.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {
    constexpr int Width = 112, Height = 43, SideX = 39, SideW = 70;
    int actualWidth = Width, actualHeight = Height;
    void RefreshDimensions() {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
            actualWidth = info.srWindow.Right - info.srWindow.Left + 1;
            actualHeight = info.srWindow.Bottom - info.srWindow.Top + 1;
        }
#else
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) { actualWidth = ws.ws_col; actualHeight = ws.ws_row; }
#endif
    }
    void At(int x, int y) {
#ifdef _WIN32
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{ static_cast<SHORT>(x),static_cast<SHORT>(y) });
#else
        std::cout << "\033[" << y + 1 << ';' << x + 1 << 'H';
#endif
    }
    void Color(int fg, int bg = 0) {
#ifdef _WIN32
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), static_cast<WORD>(fg | (bg << 4)));
#else
        (void)fg; (void)bg;
#endif
    }
    void ResetColor() { Color(7); }
    std::string Clip(std::string s, size_t max) { if (s.size() > max && max > 3) s = s.substr(0, max - 3) + "..."; return s; }
    void Put(int x, int y, const std::string& s, size_t max = SideW - 4) {
        if (x < 0 || y < 0 || x >= actualWidth || y >= actualHeight)return;
        const size_t count = std::min(max, static_cast<size_t>(actualWidth - x));
        if (count == 0)return;
        std::string value = Clip(s, count);
#ifdef _WIN32
        // Write directly to console cells, not through cout. This cannot wrap,
        // scroll, or leak text into the information panel on Windows Terminal.
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(h, &info))return;
        COORD dest{ static_cast<SHORT>(info.srWindow.Left + x),
                   static_cast<SHORT>(info.srWindow.Top + y) };
        DWORD written = 0;
        FillConsoleOutputCharacterA(h, ' ', static_cast<DWORD>(count), dest, &written);
        WriteConsoleOutputCharacterA(h, value.c_str(), static_cast<DWORD>(value.size()), dest, &written);
#else
        At(x, y);
        std::cout << value;
        if (value.size() < count)std::cout << std::string(count - value.size(), ' ');
#endif
    }
    void Rule(int y, const std::string& title) { Put(SideX, y, "+" + std::string(SideW - 2, '-') + "+", SideW); Put(SideX + 2, y + 1, title); Put(SideX, y + 2, "|" + std::string(SideW - 2, '-') + "|", SideW); }
    void FitConsole() {
#ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h == INVALID_HANDLE_VALUE)return;
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(h, &info))return;
        // Windows Terminal can refuse window resizing. Grow the underlying buffer first,
        // and always measure the actual dimensions instead of assuming success.
        COORD buffer{ static_cast<SHORT>(std::max<int>(Width,info.dwSize.X)),
                     static_cast<SHORT>(std::max<int>(Height,info.dwSize.Y)) };
        SetConsoleScreenBufferSize(h, buffer);
        SMALL_RECT target{ 0,0,static_cast<SHORT>(Width - 1),static_cast<SHORT>(Height - 1) };
        SetConsoleWindowInfo(h, TRUE, &target);
#endif
        RefreshDimensions();
    }
}

void ChessRenderer::ConfigureConsole() const { FitConsole(); }
void ChessRenderer::ClearScreen() const {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(h, &info))return;
    DWORD n = 0; COORD home{ 0,0 }; DWORD cells = static_cast<DWORD>(info.dwSize.X) * info.dwSize.Y;
    FillConsoleOutputCharacterA(h, ' ', cells, home, &n);
    FillConsoleOutputAttribute(h, 7, cells, home, &n); SetConsoleCursorPosition(h, home);
#else
    std::cout << "\033[2J\033[H";
#endif
}
void ChessRenderer::ShowMainMenu() const {
    std::cout << "\n  =======================================\n             MAGE'S CHESS\n  =======================================\n       Strategy meets elemental magic\n\n       [1] Start New Game\n       [2] Load Saved Game\n       [0] Exit\n\n  =======================================\n";
}
void ChessRenderer::ShowElementMenu(PlayerColor color) const {
    std::cout << "\n  " << ToString(color) << " - Choose Your Element\n  ---------------------------------------\n    [1] Scorch  - Aggression and sacrifice\n    [2] River   - Adaptation and defense\n    [3] Zephyr  - Mobility and freedom\n  ---------------------------------------\n";
}
char ChessRenderer::GetPieceSymbol(const Piece& p) const {
    char c = '.'; switch (p.type) { case PieceType::Pawn:c = 'P'; break; case PieceType::Knight:c = 'N'; break; case PieceType::Bishop:c = 'B'; break; case PieceType::Rook:c = 'R'; break; case PieceType::Queen:c = 'Q'; break; case PieceType::King:c = 'K'; break; default:break; }
                                                        return p.color == PlayerColor::Black ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : c;
}
void ChessRenderer::ShowGameScreen(const ChessBoard& board, const Player& white, const Player& black,
    PlayerColor turn, bool check, const std::string& lastMove, const std::vector<std::string>& history,
    int ownTurn, bool canEnchant, const std::string& message, bool help, bool inspect,
    Position cursor, Position selected, const std::vector<Move>& highlights) const {
    RefreshDimensions();
    ClearScreen();
    Put(2, 1, "MAGE'S CHESS  |  ELEMENTAL WARFARE", 35);
    Put(2, 3, "BLACK: " + ToString(black.GetElement()) + "  Mana: " + std::to_string(black.GetMana()) + "/100", 35);
    Put(5, 5, "a  b  c  d  e  f  g  h", 30);
    for (int row = 0; row < 8; ++row) {
        At(2, 6 + row); std::cout << (8 - row) << "  ";
        for (int col = 0; col < 8; ++col) {
            Position pos{ row,col }; const Piece& p = board.GetPiece(pos);
            const bool chosen = pos == selected;
            const bool possible = std::any_of(highlights.begin(), highlights.end(), [&](const Move& m) {return m.to == pos; });
            const bool magical = std::any_of(highlights.begin(), highlights.end(), [&](const Move& m) {return m.to == pos && m.enchanted; });
            int bg = pos == cursor ? 3 : chosen ? 6 : possible ? (magical ? 5 : 2) : ((row + col) % 2 == 0 ? 7 : 8);
            Color(p.type == PieceType::None ? bg : (p.color == PlayerColor::White ? 15 : 0), bg);
            std::cout << ' ' << GetPieceSymbol(p) << ' '; ResetColor();
        }
        At(2, 6 + row); std::cout << "   ";
    }
    Put(5, 14, "a  b  c  d  e  f  g  h", 30);
    Put(2, 17, "WHITE: " + ToString(white.GetElement()) + "  Mana: " + std::to_string(white.GetMana()) + "/100", 35);
    Put(2, 19, "Cursor: " + SquareName(cursor), 35);
    Put(2, 21, "CYAN cursor | YELLOW selected", 35);
    Put(2, 22, "GREEN normal | PURPLE magic", 35);

    Rule(2, "MATCH STATUS");
    Put(SideX + 2, 5, "TURN: " + ToString(turn) + (check ? "  [CHECK!]" : ""));
    Put(SideX + 2, 6, "LAST: " + (lastMove.empty() ? std::string("No moves yet") : lastMove));
    Put(SideX + 2, 7, "White: " + ToString(white.GetElement()) + "  " + std::to_string(white.GetMana()) + "/100 mana");
    Put(SideX + 2, 8, "Black: " + ToString(black.GetElement()) + "  " + std::to_string(black.GetMana()) + "/100 mana");
    Rule(10, "ENCHANTMENT STATUS");
    Put(SideX + 2, 13, "Turn " + std::to_string(ownTurn) + " | " + (canEnchant ? "Available (subject to mana)" : "Unavailable: first turn / cooldown"));
    Rule(15, "RECENT MOVES");
    if (history.empty())Put(SideX + 2, 18, "No moves yet.");
    else {
        size_t start = history.size() > 5 ? history.size() - 5 : 0;
        for (size_t i = start; i < history.size(); ++i)
            Put(SideX + 2, 18 + static_cast<int>(i - start), std::to_string(i + 1) + ". " + history[i]);
    }
    const int panelY = 24;
    Rule(panelY, help ? "HELP" : inspect ? "PIECE INSPECTION" : "INFORMATION");
    if (help) {
        Put(SideX + 2, panelY + 3, "Select your piece, then choose a highlighted destination.");
        Put(SideX + 2, panelY + 4, "Green highlights show legal standard moves.");
        Put(SideX + 2, panelY + 5, "Purple highlights show available enchanted moves.");
        Put(SideX + 2, panelY + 6, "Enchanted attacks cannot directly check the king.");
    }
    else if (inspect) {
        const Piece& p = board.GetPiece(cursor);
        Put(SideX + 2, panelY + 3, "Square: " + SquareName(cursor));
        if (p.type == PieceType::None)Put(SideX + 2, panelY + 4, "Empty square");
        else {
            const Player& owner = p.color == PlayerColor::White ? white : black;
            Put(SideX + 2, panelY + 4, "Piece: " + ToString(p.color) + " " + ToString(p.type));
            Put(SideX + 2, panelY + 5, "Element: " + ToString(owner.GetElement()));
            Put(SideX + 2, panelY + 6, std::string("Ability: ") + EnchantmentManager::GetAbilityName(p.type, owner.GetElement()));
            int cost = EnchantmentManager::GetCost(p.type);
            Put(SideX + 2, panelY + 7, cost < 0 ? "Enchantment unavailable" : "Cost: " + std::to_string(cost) + " mana");
        }

    }
    else Put(SideX + 2, panelY + 3, message.empty() ? "Select a piece with Enter to begin." : message);
    // Dedicated footer. Never include controls in the information panel.
    // Use viewport coordinates and direct cell writes so lines cannot wrap.
    const int footerY = actualHeight - 4;
    if (footerY >= panelY + 10) {
        Put(2, footerY, std::string(std::max(0, actualWidth - 4), '='), actualWidth - 4);
        Put(2, footerY + 1, "ARROWS Navigate  |  ENTER Select/Move  |  E Magic  |  I Inspect", actualWidth - 4);
        Put(2, footerY + 2, "H Help  |  S Save  |  ESC Cancel/Close  |  M Main Menu", actualWidth - 4);
    }
    std::cout.flush();
}
void ChessRenderer::ShowBoard(const ChessBoard& board, const Player& white, const Player& black,
    const std::string& message, PlayerColor turn, bool check, const std::string& lastMove,
    Position selected, const std::vector<Move>& highlights, Position cursor) const {
    ShowGameScreen(board, white, black, turn, check, lastMove, {}, 1, false, message, false, false, cursor, selected, highlights);
}
void ChessRenderer::ShowSquareDetails(Position pos, const Piece& p, const Player& owner) const {
    std::cout << "\n  Square " << SquareName(pos) << ": " << ToString(p.color) << ' ' << ToString(p.type) << '\n';
    int cost = EnchantmentManager::GetCost(p.type);
    if (cost >= 0)std::cout << "  Element: " << ToString(owner.GetElement()) << " | Cost: " << cost << " mana\n";
}
void ChessRenderer::ShowMessage(const std::string& message) const { std::cout << "\n  " << message << '\n'; }
void ChessRenderer::ShowMatchDetails(const Player&, int, bool) const {}
void ChessRenderer::ShowRecentMoves(const std::vector<std::string>&) const {}
void ChessRenderer::ShowInformationPanel(const std::string&, bool, bool, Position, const Piece&, const Player&) const {}
void ChessRenderer::ShowControls() const {}
