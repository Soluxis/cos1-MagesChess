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
    void Put(int x, int y, const std::string& s, size_t max = SideW - 4, int fg = 7) {
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
        FillConsoleOutputAttribute(h, static_cast<WORD>(fg), static_cast<DWORD>(count), dest, &written);
#else
        At(x, y);
        (void)fg;
        std::cout << value;
        if (value.size() < count)std::cout << std::string(count - value.size(), ' ');
#endif
    }
    // Distinct identity colors: Scorch muted amber, River royal blue, Zephyr pale aqua.
    // Board highlights use bright cyan/yellow/green/purple/orange instead.
int ElementInk(Element e) { switch (e) { case Element::Scorch:return 6; case Element::River:return 9; case Element::Zephyr:return 11; }return 7; }
                                                             void ElementLine(int x, int y, const std::string& prefix, Element element, const std::string& suffix, size_t width, int base = 7) {
                                                                 Put(x, y, prefix + ToString(element) + suffix, width, base);
                                                                 Put(x + static_cast<int>(prefix.size()), y, ToString(element), ToString(element).size(), ElementInk(element));
                                                             }
                                                             void Tile(int x, int y, char symbol, int fg, int bg) {
#ifdef _WIN32
                                                                 HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); CONSOLE_SCREEN_BUFFER_INFO info{};
                                                                 if (!GetConsoleScreenBufferInfo(h, &info))return;
                                                                 if (x < 0 || y < 0 || x + 2 >= actualWidth || y >= actualHeight)return;
                                                                 COORD dest{ static_cast<SHORT>(info.srWindow.Left + x),static_cast<SHORT>(info.srWindow.Top + y) };
                                                                 const char chars[3] = { ' ',symbol,' ' }; DWORD n = 0;
                                                                 WriteConsoleOutputCharacterA(h, chars, 3, dest, &n);
                                                                 FillConsoleOutputAttribute(h, static_cast<WORD>(fg | (bg << 4)), 3, dest, &n);
#else
                                                                 At(x, y); Color(fg, bg); std::cout << ' ' << symbol << ' '; ResetColor();
#endif
                                                             }
                                                             void Rule(int y, const std::string& title) { Put(SideX, y, "+" + std::string(SideW - 2, '-') + "+", SideW, 8); Put(SideX + 2, y + 1, title, SideW - 4, 11); Put(SideX, y + 2, "|" + std::string(SideW - 2, '-') + "|", SideW, 8); }
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
    Put(2, 1, "MAGE'S CHESS  |  ELEMENTAL WARFARE", 35, 14);
    ElementLine(2, 3, "BLACK: ", black.GetElement(), "  Mana: " + std::to_string(black.GetMana()) + "/100", 35, 11);
    Put(5, 5, "a  b  c  d  e  f  g  h", 30);
    // The last move is already recorded as "White e2-e4" (with optional annotations).
    // Parse only the two coordinate tokens; do not change the game state or save format.
    Position previousFrom{ -1,-1 }, previousTo{ -1,-1 };
    for (size_t i = 0; i + 4 < lastMove.size(); ++i) {
        if (lastMove[i] >= 'a' && lastMove[i] <= 'h' && lastMove[i + 1] >= '1' && lastMove[i + 1] <= '8' &&
            lastMove[i + 2] == '-' && lastMove[i + 3] >= 'a' && lastMove[i + 3] <= 'h' && lastMove[i + 4] >= '1' && lastMove[i + 4] <= '8') {
            previousFrom = { 8 - (lastMove[i + 1] - '0'),lastMove[i] - 'a' };
            previousTo = { 8 - (lastMove[i + 4] - '0'),lastMove[i + 3] - 'a' }; break;
        }
    }
    for (int row = 0; row < 8; ++row) {
        Put(2, 6 + row, std::to_string(8 - row) + "  ", 3);
        for (int col = 0; col < 8; ++col) {
            Position pos{ row,col }; const Piece& piece = board.GetPiece(pos);
            bool chosen = pos == selected;
            bool possible = false, magical = false, capture = false;
            for (const Move& move : highlights) {
                if (move.to != pos)continue;
                possible = true; magical = magical || move.enchanted;
                // Captures are emphasized separately, including en passant and spell captures.
                capture = capture || (!board.IsEmpty(pos) && board.GetPiece(pos).color != turn) ||
                    (!move.enchanted && board.GetPiece(move.from).type == PieceType::Pawn &&
                        move.from.column != pos.column && board.IsEmpty(pos));
            }
            bool checkedKing = check && piece.type == PieceType::King && piece.color == turn;
            int bg = ((row + col) % 2 == 0 ? 7 : 8);
            if (pos == previousFrom || pos == previousTo)bg = 1; // subtle dark blue
            if (pos == cursor)bg = 3;                         // cyan cursor
            else if (chosen)bg = 6;                         // yellow selection
            else if (checkedKing)bg = 4;                    // red checked king
            else if (possible)bg = capture ? 12 : (magical ? 5 : 2); // orange-red / purple / green
            int fg = piece.type == PieceType::None ? bg : (piece.color == PlayerColor::White ? 15 : 0);
            if (checkedKing && bg == 4)fg = 15;
            Tile(5 + col * 3, 6 + row, GetPieceSymbol(piece), fg, bg);
        }
    }
    Put(5, 14, "a  b  c  d  e  f  g  h", 30);
    ElementLine(2, 17, "WHITE: ", white.GetElement(), "  Mana: " + std::to_string(white.GetMana()) + "/100", 35, 15);
    
    Put(2, 19, "Cursor: " + SquareName(cursor), 35, 14);
    Put(2, 21, "CYAN", 4, 11); Put(6, 21, " cursor | ", 10, 15); Put(16, 21, "YELLOW", 6, 14); Put(22, 21, " selected", 9, 15);
    Put(2, 22, "GREEN", 5, 10); Put(7, 22, " normal | ", 10, 15); Put(17, 22, "PURPLE", 6, 13); Put(23, 22, " magic", 6, 15);
    
    Put(2, 23, "RED", 3, 12);                  
    Put(5, 23, " check/capture | ", 17, 15);  
    Put(22, 23, "BLUE", 4, 9);                 
    Put(26, 23, " last move", 10, 15);         

    Rule(2, "MATCH STATUS");
    Put(SideX + 2, 5, "TURN: " + ToString(turn) + (check ? "  [CHECK!]" : ""), SideW - 4, check ? 12 : 14);
    Put(SideX + 2, 6, "LAST: " + (lastMove.empty() ? std::string("No moves yet") : lastMove), SideW - 4, 7);
    ElementLine(SideX + 2, 7, "White: ", white.GetElement(), "  " + std::to_string(white.GetMana()) + "/100 mana", SideW - 4, 11);
    ElementLine(SideX + 2, 8, "Black: ", black.GetElement(), "  " + std::to_string(black.GetMana()) + "/100 mana", SideW - 4, 11);
    Rule(10, "ENCHANTMENT STATUS");
    Put(SideX + 2, 13, "Turn " + std::to_string(ownTurn) + " | " + (canEnchant ? "Available (subject to mana)" : "Unavailable: first turn / cooldown"), SideW - 4, canEnchant ? 10 : 8);
    Rule(15, "RECENT MOVES");
    if (history.empty())Put(SideX + 2, 18, "No moves yet.", SideW - 4, 8);
    else {
        size_t start = history.size() > 5 ? history.size() - 5 : 0;
        for (size_t i = start; i < history.size(); ++i)
            Put(SideX + 2, 18 + static_cast<int>(i - start), std::to_string(i + 1) + ". " + history[i], SideW - 4, 7);
    }
    const int panelY = 24;
    Rule(panelY, help ? "HELP" : inspect ? "PIECE INSPECTION" : "INFORMATION");
    if (help) {
        Put(SideX + 2, panelY + 3, "Select your piece, then choose a highlighted destination.", SideW - 4, 7);
        Put(SideX + 2, panelY + 4, "Green highlights show legal standard moves.", SideW - 4, 10);
        Put(SideX + 2, panelY + 5, "Purple highlights show available enchanted moves.", SideW - 4, 13);
        Put(SideX + 2, panelY + 6, "Enchanted attacks cannot directly check the king.", SideW - 4, 8);
    }
    else if (inspect) {
        const Piece& p = board.GetPiece(cursor);
        Put(SideX + 2, panelY + 3, "Square: " + SquareName(cursor), SideW - 4, 14);
        if (p.type == PieceType::None)Put(SideX + 2, panelY + 4, "Empty square", SideW - 4, 8);
        else {
            const Player& owner = p.color == PlayerColor::White ? white : black;
            Put(SideX + 2, panelY + 4, "Piece: " + ToString(p.color) + " " + ToString(p.type), SideW - 4, 15);
            ElementLine(SideX + 2, panelY + 5, "Element: ", owner.GetElement(), "", SideW - 4, 11);
            Put(SideX + 2, panelY + 6, std::string("Ability: ") + EnchantmentManager::GetAbilityName(p.type, owner.GetElement()), SideW - 4, 13);
            int cost = EnchantmentManager::GetCost(p.type);
            Put(SideX + 2, panelY + 7, cost < 0 ? "Enchantment unavailable" : "Cost: " + std::to_string(cost) + " mana", SideW - 4, cost < 0 ? 8 : 14);
        }

    }
    else {
        int messageColor = 11;
        if (message.find("Game saved") != std::string::npos || message.find("Move accepted") != std::string::npos)messageColor = 10;
        else if (message.find("failed") != std::string::npos || message.find("Illegal") != std::string::npos || message.find("unavailable") != std::string::npos)messageColor = 12;
        Put(SideX + 2, panelY + 3, message.empty() ? "Select a piece with Enter to begin." : message, SideW - 4, messageColor);
    }
    // Dedicated footer. Never include controls in the information panel.
    // Use viewport coordinates and direct cell writes so lines cannot wrap.
    const int footerY = actualHeight - 4;
    if (footerY >= panelY + 10) {
        Put(2, footerY, std::string(std::max(0, actualWidth - 4), '='), actualWidth - 4, 8);
        Put(2, footerY + 1, "ARROWS Navigate  |  ENTER Select/Move  |  E Magic  |  I Inspect", actualWidth - 4, 11);
        Put(2, footerY + 2, "H Help  |  S Save  |  ESC Cancel/Close  |  M Main Menu", actualWidth - 4, 11);
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

void ChessRenderer::ShowGameOver(PlayerColor turn, bool check, size_t moves) const {
    ClearScreen();
    Put(24, 5, "===============================================", 47, 8);
    Put(24, 7, "                 MAGE'S CHESS                  ", 47, 14);
    Put(24, 9, check ? "                   CHECKMATE                   " : "                   STALEMATE                   ", 47, check ? 12 : 11);
    Put(24, 11, check ? (ToString(Opponent(turn)) + " wins the match!") : "The match ends in a draw.", 47, check ? 14 : 11);
    Put(24, 13, "Moves played: " + std::to_string(moves), 47, 15);
    Put(24, 16, "Press ENTER to return to the main menu", 47, 10);
    Put(24, 18, "===============================================", 47, 8);
    std::cout.flush();
}
