#include "ChessGame.h"
#include <cctype>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>

ChessGame::ChessGame() : mWhite(PlayerColor::White), mBlack(PlayerColor::Black) {}

int ChessGame::ReadChoice(int minimum, int maximum) {
    std::string input;
    while (true) {
        std::cout << "  > ";
        if (!std::getline(std::cin, input)) return minimum;
        std::istringstream stream(input);
        int choice; char extra;
        if ((stream >> choice) && !(stream >> extra) && choice >= minimum && choice <= maximum) return choice;
        std::cout << "  Enter a number from " << minimum << " to " << maximum << ".\n";
    }
}

Element ChessGame::SelectElement(PlayerColor color) {
    mRenderer.ClearScreen(); mRenderer.ShowElementMenu(color);
    switch (ReadChoice(1, 3)) { case 2:return Element::River; case 3:return Element::Zephyr; default:return Element::Scorch; }
}

bool ChessGame::ParseSquare(const std::string& s, Position& p) {
    if (s.size() != 2) return false;
    char file = char(std::tolower(static_cast<unsigned char>(s[0]))), rank = s[1];
    if (file < 'a' || file>'h' || rank < '1' || rank>'8')return false;
    p = { 8 - (rank - '0'),file - 'a' }; return true;
}

bool ChessGame::ParseMove(const std::string& raw, Move& move) {
    std::string s;
    for (char c : raw) if (!std::isspace(static_cast<unsigned char>(c)) && c != '-') s += char(std::tolower(static_cast<unsigned char>(c)));
    if (s.size() != 4 && s.size() != 5) return false;
    if (!ParseSquare(s.substr(0, 2), move.from) || !ParseSquare(s.substr(2, 2), move.to)) return false;
    move.promotion = PieceType::None; move.enchanted = false;
    if (s.size() == 5) {
        switch (s[4]) {
        case 'q':move.promotion = PieceType::Queen; break;
        case 'r':move.promotion = PieceType::Rook; break;
        case 'b':move.promotion = PieceType::Bishop; break;
        case 'n':move.promotion = PieceType::Knight; break;
        default:return false;
        }
    }
    return true;
}

void ChessGame::PlayMatch() {
    std::string message = "Enter e2e4 for normal moves, or enchant e2e3 for magic. Type help.";
    std::string input;
    Position highlighted{ -1,-1 };
    std::vector<Move> highlights;
    while (true) {
        Player& active = mTurn == PlayerColor::White ? mWhite : mBlack;
        Player& other = mTurn == PlayerColor::White ? mBlack : mWhite;
        int ownTurn = (mTurn == PlayerColor::White ? mWhiteTurns : mBlackTurns) + 1;
        const bool check = MoveValidator::IsInCheck(mBoard, mTurn);
        const auto normal = MoveValidator::LegalMoves(mBoard, mTurn, mEnPassantTarget);
        const auto spells = EnchantmentManager::LegalMoves(mBoard, active, ownTurn);
        mRenderer.ClearScreen();
        mRenderer.ShowBoard(mBoard, mWhite, mBlack, message, mTurn, check, mLastMove, highlighted, highlights);
        std::cout << "  Enchant: " << (active.CanEnchant(ownTurn) ? "Ready (if mana allows)" : "Cooldown / first turn") << "\n";
        std::cout << "  Recent moves:\n";
        for (size_t i = mHistory.size() > 5 ? mHistory.size() - 5 : 0; i < mHistory.size(); ++i)
            std::cout << "    " << mHistory[i] << "\n";
        if (normal.empty() && spells.empty()) {
            std::cout << "\n  " << (check ? "CHECKMATE! " : "STALEMATE! ");
            if (check)std::cout << ToString(Opponent(mTurn)) << " wins!\n";
            else std::cout << "Draw.\n";
            std::cout << "  Press Enter to return to menu...";
            std::getline(std::cin, input); return;
        }
        std::cout << "\n  " << ToString(mTurn) << " move > ";
        if (!std::getline(std::cin, input))return;
        if (input == "menu" || input == "0" || input == "quit")return;
        if (input == "help") {
            message = "e2e4 | enchant e2e3 | moves e2 | spells e2 | inspect e2 | menu";
            continue;
        }
        bool enchanted = false;
        if (input.rfind("enchant ", 0) == 0) { enchanted = true; input = input.substr(8); }
        else if (input.rfind("e ", 0) == 0) { enchanted = true; input = input.substr(2); }
        if (input.rfind("moves ", 0) == 0 || input.rfind("spells ", 0) == 0) {
            bool magical = input.rfind("spells ", 0) == 0;
            Position from;
            if (!ParseSquare(input.substr(magical ? 7 : 6), from)) { message = "Invalid square."; continue; }
            highlighted = from; highlights.clear();
            for (const auto& candidate : (magical ? spells : normal))if (candidate.from == from)highlights.push_back(candidate);
            message = (magical ? "Enchanted" : "Normal") + std::string(" moves from ") + SquareName(from) + ": ";
            int count = 0;
            for (const auto& m : (magical ? spells : normal))if (m.from == from) {
                if (count++)message += ", ";
                message += SquareName(m.to);
                if (m.promotion != PieceType::None)
                    message += m.promotion == PieceType::Queen ? 'q' : m.promotion == PieceType::Rook ? 'r' : m.promotion == PieceType::Bishop ? 'b' : 'n';
            }
            if (!count)message += "none (or insufficient mana/cooldown)";
            continue;
        }
        if (input.rfind("inspect ", 0) == 0) {
            Position p;
            if (!ParseSquare(input.substr(8), p)) { message = "Invalid square."; continue; }
            Piece piece = mBoard.GetPiece(p);
            if (piece.type == PieceType::None) { message = "That square is empty."; continue; }
            const Player& owner = piece.color == PlayerColor::White ? mWhite : mBlack;
            mRenderer.ShowSquareDetails(p, piece, owner);
            std::cout << "\n  Press Enter to continue...";
            if (!std::getline(std::cin, input))return;
            message = "Select your next move."; continue;
        }
        Move move;
        if (!ParseMove(input, move)) { message = "Invalid move. Example: e2e4 or enchant e2e3."; continue; }
        move.enchanted = enchanted;
        const Piece moving = mBoard.GetPiece(move.from);
        if (moving.type == PieceType::None || moving.color != mTurn) { message = "Select one of your own pieces."; continue; }
        if (moving.type == PieceType::Pawn && (move.to.row == 0 || move.to.row == 7) && move.promotion == PieceType::None) {
            message = "Promotion requires q, r, b, or n appended to the move."; continue;
        }
        int enemies = 0, friendlies = 0;
        bool capture = false, castle = false;
        if (enchanted) {
            if (!EnchantmentManager::CanEnchant(active, moving.type, ownTurn)) {
                message = "Enchantment unavailable: mana, cooldown, or unsupported piece."; continue;
            }
            SpellResult outcome;
            if (!EnchantmentManager::TryApply(mBoard, move, mTurn, active.GetElement(), outcome)) {
                message = "Illegal enchanted move or king left in check. Use spells <square>."; continue;
            }
            active.SpendMana(EnchantmentManager::GetCost(moving.type));
            active.RecordEnchantment(ownTurn);
            mBoard = outcome.board;
            enemies = outcome.enemyLost; friendlies = outcome.friendlyLost;
        }
        else {
            if (!MoveValidator::IsLegal(mBoard, mTurn, move, mEnPassantTarget)) {
                message = "Illegal move: blocked, invalid pattern, or leaves your king in check."; continue;
            }
            bool enPassant = moving.type == PieceType::Pawn && move.to == mEnPassantTarget &&
                mBoard.IsEmpty(move.to) && move.from.column != move.to.column;
            capture = enPassant || !mBoard.IsEmpty(move.to);
            castle = moving.type == PieceType::King && std::abs(move.to.column - move.from.column) == 2;
            MoveValidator::Apply(mBoard, move, mEnPassantTarget);
            enemies = capture ? 1 : 0;
        }
        for (int i = 0; i < enemies; ++i) { active.GainMana(5); other.GainMana(15); }
        for (int i = 0; i < friendlies; ++i) other.GainMana(15);
        mEnPassantTarget = { -1,-1 };
        if (!enchanted && moving.type == PieceType::Pawn && std::abs(move.to.row - move.from.row) == 2)
            mEnPassantTarget = { (move.from.row + move.to.row) / 2,move.from.column };
        mLastMove = ToString(mTurn) + " " + SquareName(move.from) + "-" + SquareName(move.to);
        if (enchanted)mLastMove += " [" + ToString(active.GetElement()) + " " + EnchantmentManager::GetAbilityName(moving.type, active.GetElement()) + "]";
        if (enemies || friendlies)mLastMove += " (enemy lost: " + std::to_string(enemies) + ", friendly lost: " + std::to_string(friendlies) + ")";
        if (castle)mLastMove += " (castle)";
        if (move.promotion != PieceType::None)mLastMove += " (promoted to " + ToString(move.promotion) + ")";
        mHistory.push_back(mLastMove);
        highlighted = { -1,-1 }; highlights.clear();
        if (mTurn == PlayerColor::White)++mWhiteTurns; else ++mBlackTurns;
        mTurn = Opponent(mTurn);
        if (mTurn == PlayerColor::White)++mFullMoveNumber;
        message = "Move accepted.";
    }
}

void ChessGame::StartNewGame() {
    mBoard.Reset(); mWhite.Reset(SelectElement(PlayerColor::White)); mBlack.Reset(SelectElement(PlayerColor::Black));
    mTurn = PlayerColor::White; mEnPassantTarget = { -1,-1 }; mFullMoveNumber = 1; mLastMove = ""; mHistory.clear(); mWhiteTurns = 0; mBlackTurns = 0;
    PlayMatch();
}

void ChessGame::Run() {
    while (true) {
        mRenderer.ClearScreen(); mRenderer.ShowMainMenu();
        std::cout << "\n  Select an option:\n";
        int choice = ReadChoice(0, 1);
        if (choice == 0) { mRenderer.ShowMessage("Thanks for playing Mage's Chess!"); return; }
        StartNewGame();
    }
}
