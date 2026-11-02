#include "ChessBoard.h"
#include <stdexcept>

ChessBoard::ChessBoard()
{
    Reset();
}

void ChessBoard::Reset()
{
    mSquares.assign(Size, std::vector<Piece>(Size));

    PlaceBackRank(0, PlayerColor::Black);
    PlaceBackRank(7, PlayerColor::White);

    for (int column = 0; column < Size; ++column)
    {
        mSquares[1][column] = { PieceType::Pawn, PlayerColor::Black, false };
        mSquares[6][column] = { PieceType::Pawn, PlayerColor::White, false };
    }
}

void ChessBoard::PlaceBackRank(int row, PlayerColor color)
{
    const PieceType order[Size] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop,
        PieceType::Queen, PieceType::King, PieceType::Bishop,
        PieceType::Knight, PieceType::Rook
    };

    for (int column = 0; column < Size; ++column)
    {
        mSquares[row][column] = { order[column], color, false };
    }
}

bool ChessBoard::IsInside(Position position) const
{
    return position.row >= 0 && position.row < Size
        && position.column >= 0 && position.column < Size;
}

const Piece& ChessBoard::GetPiece(Position position) const
{
    if (!IsInside(position))
    {
        throw std::out_of_range("Chessboard position is out of bounds.");
    }

    return mSquares[position.row][position.column];
}

bool ChessBoard::IsEmpty(Position position) const
{
    return GetPiece(position).type == PieceType::None;
}
