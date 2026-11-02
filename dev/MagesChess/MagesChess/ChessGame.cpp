#include "ChessGame.h"
#include <cctype>
#include <iostream>
#include <sstream>

ChessGame::ChessGame()
    : mWhite(PlayerColor::White), mBlack(PlayerColor::Black)
{
}

int ChessGame::ReadChoice(int minimum, int maximum)
{
    std::string input;
    int choice = 0;
    while (true)
    {
        std::cout << "  > ";
        if (!std::getline(std::cin, input))
        {
            return minimum;
        }

        std::istringstream stream(input);
        char extra = '\0';
        if ((stream >> choice) && !(stream >> extra)
            && choice >= minimum && choice <= maximum)
        {
            return choice;
        }
        std::cout << "  Invalid choice. Enter a number from "
            << minimum << " to " << maximum << ".\n";
    }
}

Element ChessGame::SelectElement(PlayerColor color)
{
    mRenderer.ClearScreen();
    mRenderer.ShowElementMenu(color);
    switch (ReadChoice(1, 3))
    {
    case 2: return Element::River;
    case 3: return Element::Zephyr;
    default: return Element::Scorch;
    }
}

bool ChessGame::ParseSquare(const std::string& input, Position& position)
{
    if (input.size() != 2) return false;
    const char file = static_cast<char>(std::tolower(static_cast<unsigned char>(input[0])));
    const char rank = input[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return false;
    position = { 8 - (rank - '0'), file - 'a' };
    return true;
}

void ChessGame::InspectBoard()
{
    std::string message = "Enter a square (e.g. e2), or 0 to return to menu.";
    std::string input;

    while (true)
    {
        mRenderer.ClearScreen();
        mRenderer.ShowBoard(mBoard, mWhite, mBlack, message);
        std::cout << "\n  Select square > ";
        if (!std::getline(std::cin, input) || input == "0") return;

        Position position;
        if (!ParseSquare(input, position))
        {
            message = "Invalid square. Use a1 through h8.";
            continue;
        }

        const Piece& piece = mBoard.GetPiece(position);
        if (piece.type == PieceType::None)
        {
            message = "That square is empty.";
            continue;
        }

        mRenderer.ShowSquareDetails(position, piece,
            piece.color == PlayerColor::White ? mWhite : mBlack);
        std::cout << "\n  Press Enter to continue...";
        if (!std::getline(std::cin, input)) return;
        message = "Enter a square (e.g. e2), or 0 to return to menu.";
    }
}

void ChessGame::StartNewGame()
{
    mBoard.Reset();
    mWhite.Reset(SelectElement(PlayerColor::White));
    mBlack.Reset(SelectElement(PlayerColor::Black));
    InspectBoard();
}

void ChessGame::Run()
{
    while (true)
    {
        mRenderer.ClearScreen();
        mRenderer.ShowMainMenu();
        std::cout << "\n  Select an option:\n";
        const int choice = ReadChoice(0, 1);
        if (choice == 0)
        {
            mRenderer.ShowMessage("Thanks for playing Mage's Chess!");
            return;
        }
        StartNewGame();
    }
}
