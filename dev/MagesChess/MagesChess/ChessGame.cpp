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
    std::string message = "Select one of your pieces with Enter to begin.";
    Position cursor{ 6,4 };
    Position selected{ -1,-1 };
    bool enchanted = false;
    enum class InfoMode { Status, Help, Inspect };
    InfoMode infoMode = InfoMode::Status;
    std::vector<Move> highlights;
    while (true) {
        Player& active = mTurn == PlayerColor::White ? mWhite : mBlack;
        Player& other = mTurn == PlayerColor::White ? mBlack : mWhite;
        int ownTurn = (mTurn == PlayerColor::White ? mWhiteTurns : mBlackTurns) + 1;
        const bool check = MoveValidator::IsInCheck(mBoard, mTurn);
        const auto normal = MoveValidator::LegalMoves(mBoard, mTurn, mEnPassantTarget);
        const auto spells = EnchantmentManager::LegalMoves(mBoard, active, ownTurn);
        mRenderer.ShowGameScreen(mBoard, mWhite, mBlack, mTurn, check, mLastMove, mHistory,
            ownTurn, active.CanEnchant(ownTurn), message, infoMode == InfoMode::Help,
            infoMode == InfoMode::Inspect, cursor, selected, highlights);
        if (normal.empty() && spells.empty()) {
            std::cout << "\n  " << (check ? "CHECKMATE! " : "STALEMATE! ");
            if (check)std::cout << ToString(Opponent(mTurn)) << " wins!\n";
            else std::cout << "Draw.\n";
            std::cout << "  Press Enter to return to menu...";
            while (InputManager::ReadKey() != InputKey::Enter) {}
            return;
        }
        mRenderer.ShowControls();
        const InputKey key = InputManager::ReadKey();
        if (key == InputKey::Menu)return;
        if (key == InputKey::Up) { infoMode = InfoMode::Status; cursor.row = (cursor.row + 7) % 8; continue; }
        if (key == InputKey::Down) { infoMode = InfoMode::Status; cursor.row = (cursor.row + 1) % 8; continue; }
        if (key == InputKey::Left) { infoMode = InfoMode::Status; cursor.column = (cursor.column + 7) % 8; continue; }
        if (key == InputKey::Right) { infoMode = InfoMode::Status; cursor.column = (cursor.column + 1) % 8; continue; }
        if (key == InputKey::Help) { infoMode = infoMode == InfoMode::Help ? InfoMode::Status : InfoMode::Help; continue; }
        if (key == InputKey::Escape && infoMode != InfoMode::Status) { infoMode = InfoMode::Status; continue; }
        if (key == InputKey::Escape) { selected = { -1,-1 }; enchanted = false; highlights.clear(); message = "Selection cancelled."; continue; }
        if (key == InputKey::Inspect) {
            infoMode = infoMode == InfoMode::Inspect ? InfoMode::Status : InfoMode::Inspect;
            continue;
        }
        infoMode = InfoMode::Status;
        if (key == InputKey::Enchant) {
            if (selected.row < 0) { message = "Select one of your pieces first."; continue; }
            if (!enchanted && !EnchantmentManager::CanEnchant(active, mBoard.GetPiece(selected).type, ownTurn)) {
                message = "Enchantment unavailable: mana, cooldown, or unsupported piece."; continue;
            }
            enchanted = !enchanted;
            highlights.clear();
            for (const auto& m : (enchanted ? spells : normal))if (m.from == selected)highlights.push_back(m);
            message = enchanted ? "Enchanted destinations highlighted." : "Normal destinations highlighted.";
            continue;
        }
        if (key != InputKey::Enter)continue;
        if (selected.row < 0) {
            const Piece piece = mBoard.GetPiece(cursor);
            if (piece.type == PieceType::None || piece.color != mTurn) { message = "Select one of your own pieces."; continue; }
            selected = cursor; enchanted = false; highlights.clear();
            for (const auto& m : normal)if (m.from == selected)highlights.push_back(m);
            message = "Selected " + ToString(piece.type) + " on " + SquareName(selected) + ". Press E for magic.";
            continue;
        }
        if (cursor == selected) { selected = { -1,-1 }; enchanted = false; highlights.clear(); message = "Selection cancelled."; continue; }
        const auto& candidates = enchanted ? spells : normal;
        auto it = std::find_if(candidates.begin(), candidates.end(), [&](const Move& m) {return m.from == selected && m.to == cursor; });
        if (it == candidates.end()) {
            const Piece piece = mBoard.GetPiece(cursor);
            if (piece.type != PieceType::None && piece.color == mTurn) {
                selected = cursor; enchanted = false; highlights.clear();
                for (const auto& m : normal)if (m.from == selected)highlights.push_back(m);
                message = "Selected " + ToString(piece.type) + " on " + SquareName(selected); continue;
            }
            message = "Illegal destination. Choose a highlighted square."; continue;
        }
        Move move = *it;
        // Promotion choice uses the existing MoveValidator legality rules.
        const Piece moving = mBoard.GetPiece(move.from);
        if (moving.type == PieceType::Pawn && (move.to.row == 0 || move.to.row == 7)) {
            message = "Promotion: Q queen, R rook, B bishop, N knight (default queen).";
            mRenderer.ClearScreen();
            std::cout << "\n  MAGE'S CHESS - PAWN PROMOTION\n";
            std::cout << "  Choose promotion [Q/R/B/N]: ";
            char choice = 'q';
            std::string entry;
            if (!std::getline(std::cin, entry))return;
            if (!entry.empty())choice = char(std::tolower(static_cast<unsigned char>(entry[0])));
            switch (choice) {
            case 'r':move.promotion = PieceType::Rook; break;
            case 'b':move.promotion = PieceType::Bishop; break;
            case 'n':move.promotion = PieceType::Knight; break;
            default:move.promotion = PieceType::Queen; break;
            }
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
        highlights.clear();
        if (mTurn == PlayerColor::White)++mWhiteTurns; else ++mBlackTurns;
        mTurn = Opponent(mTurn);
        if (mTurn == PlayerColor::White)++mFullMoveNumber;
        message = "Move accepted.";
        selected = { -1,-1 }; enchanted = false;
        cursor = move.to;
    }
}

void ChessGame::StartNewGame() {
    mBoard.Reset(); mWhite.Reset(SelectElement(PlayerColor::White)); mBlack.Reset(SelectElement(PlayerColor::Black));
    mTurn = PlayerColor::White; mEnPassantTarget = { -1,-1 }; mFullMoveNumber = 1; mLastMove = ""; mHistory.clear(); mWhiteTurns = 0; mBlackTurns = 0;
    PlayMatch();
}

void ChessGame::Run() {
    mRenderer.ConfigureConsole();
    while (true) {
        mRenderer.ClearScreen(); mRenderer.ShowMainMenu();
        std::cout << "\n  Select an option:\n";
        int choice = ReadChoice(0, 1);
        if (choice == 0) { mRenderer.ShowMessage("Thanks for playing Mage's Chess!"); return; }
        StartNewGame();
    }
}
