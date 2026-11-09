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
    std::string message = "Enter a move such as e2e4, or type help, moves e2, inspect e2, or menu.";
    std::string input;
    while (true) {
        const bool check = MoveValidator::IsInCheck(mBoard, mTurn);
        const auto legal = MoveValidator::LegalMoves(mBoard, mTurn, mEnPassantTarget);
        mRenderer.ClearScreen();
        mRenderer.ShowBoard(mBoard, mWhite, mBlack, message, mTurn, check, mLastMove);
        if (legal.empty()) {
            std::cout << "\n  " << (check ? "CHECKMATE! " : "STALEMATE! ");
            if (check) std::cout << ToString(Opponent(mTurn)) << " wins!\n";
            else std::cout << "The game is a draw.\n";
            std::cout << "  Press Enter to return to the menu...";
            std::getline(std::cin, input); return;
        }
        std::cout << "\n  " << ToString(mTurn) << " move > ";
        if (!std::getline(std::cin, input))return;
        if (input == "menu" || input == "0" || input == "quit")return;
        if (input == "help") {
            message = "Moves: e2e4 | Promote: e7e8q | moves e2 | inspect e2 | menu";
            continue;
        }
        if (input.size() >= 6 && input.substr(0, 6) == "moves ") {
            Position from;
            if (!ParseSquare(input.substr(6), from)) { message = "Invalid square."; continue; }
            message = "Legal moves from " + SquareName(from) + ": ";
            int count = 0;
            for (const auto& m : legal) if (m.from == from) {
                if (count++)message += ", ";
                message += SquareName(m.to);
                if (m.promotion != PieceType::None) {
                    char p = m.promotion == PieceType::Queen ? 'q' : m.promotion == PieceType::Rook ? 'r' : m.promotion == PieceType::Bishop ? 'b' : 'n';
                    message += p;
                }
            }
            if (!count)message += "none";
            continue;
        }
        if (input.size() >= 8 && input.substr(0, 8) == "inspect ") {
            Position p;
            if (!ParseSquare(input.substr(8), p)) { message = "Invalid square."; continue; }
            Piece piece = mBoard.GetPiece(p);
            if (piece.type == PieceType::None) { message = "That square is empty."; continue; }
            const Player& owner = piece.color == PlayerColor::White ? mWhite : mBlack;
            mRenderer.ShowSquareDetails(p, piece, owner);
            std::cout << "\n  Press Enter to continue...";
            if (!std::getline(std::cin, input))return;
            message = "Select your next move.";
            continue;
        }
        Move move;
        if (!ParseMove(input, move)) { message = "Invalid format. Example: e2e4 or e7e8q."; continue; }
        const Piece moving = mBoard.GetPiece(move.from);
        if (moving.type == PieceType::None || moving.color != mTurn) { message = "Select one of your own pieces."; continue; }
        if (moving.type == PieceType::Pawn && (move.to.row == 0 || move.to.row == 7) && move.promotion == PieceType::None) {
            message = "Promotion requires a piece: q (queen), r (rook), b (bishop), or n (knight)."; continue;
        }
        if (!MoveValidator::IsLegal(mBoard, mTurn, move, mEnPassantTarget)) {
            message = "Illegal move: blocked, invalid pattern, or leaves your king in check."; continue;
        }
        const bool enPassant = moving.type == PieceType::Pawn && move.to == mEnPassantTarget &&
            mBoard.IsEmpty(move.to) && move.from.column != move.to.column;
        const bool capture = enPassant || !mBoard.IsEmpty(move.to);
        const bool castle = moving.type == PieceType::King && std::abs(move.to.column - move.from.column) == 2;
        MoveValidator::Apply(mBoard, move, mEnPassantTarget);
        if (capture) {
            Player& capturer = mTurn == PlayerColor::White ? mWhite : mBlack;
            Player& victim = mTurn == PlayerColor::White ? mBlack : mWhite;
            capturer.GainMana(5); victim.GainMana(15);
        }
        mEnPassantTarget = { -1,-1 };
        if (moving.type == PieceType::Pawn && std::abs(move.to.row - move.from.row) == 2)
            mEnPassantTarget = { (move.from.row + move.to.row) / 2,move.from.column };
        mLastMove = ToString(mTurn) + " " + SquareName(move.from) + "-" + SquareName(move.to);
        if (capture)mLastMove += " (capture: +5/+15 mana)";
        if (castle)mLastMove += " (castle)";
        if (move.promotion != PieceType::None)mLastMove += " (promoted to " + ToString(move.promotion) + ")";
        mTurn = Opponent(mTurn);
        if (mTurn == PlayerColor::White)++mFullMoveNumber;
        message = "Move accepted.";
    }
}

void ChessGame::StartNewGame() {
    mBoard.Reset(); mWhite.Reset(SelectElement(PlayerColor::White)); mBlack.Reset(SelectElement(PlayerColor::Black));
    mTurn = PlayerColor::White; mEnPassantTarget = { -1,-1 }; mFullMoveNumber = 1; mLastMove = "";
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
