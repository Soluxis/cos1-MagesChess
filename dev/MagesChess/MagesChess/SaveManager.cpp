#include "SaveManager.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <limits>
#include <utility>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
    constexpr const char* Magic = "MAGES_CHESS_V1";
    bool ValidElement(int v) { return v >= 0 && v <= 2; }
    bool ValidColor(int v) { return v == 0 || v == 1; }
    bool ValidPiece(int v) { return v >= 0 && v <= 6; }
    bool ReadLine(std::istream& in, std::string& out, size_t limit = 512) {
        if (!std::getline(in, out) || out.size() > limit) return false;
        return true;
    }
    bool ParseInts(const std::string& s, std::initializer_list<int*> fields) {
        std::istringstream in(s); char extra;
        for (int* field : fields) if (!(in >> *field)) return false;
        return !(in >> extra);
    }
}
bool SaveManager::Save(const SavedGame& game, const std::string& filename, std::string& error) {
    if (filename.empty()) { error = "Invalid save filename."; return false; }
    const std::string temporary = filename + ".tmp";
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) { error = "Cannot open save file for writing."; return false; }
    out << Magic << '\n';
    out << static_cast<int>(game.turn) << ' ' << game.fullMove << ' ' << game.whiteTurns << ' ' << game.blackTurns << '\n';
    out << static_cast<int>(game.whiteElement) << ' ' << game.whiteMana << ' ' << game.whiteLastEnchant << '\n';
    out << static_cast<int>(game.blackElement) << ' ' << game.blackMana << ' ' << game.blackLastEnchant << '\n';
    out << game.enPassant.row << ' ' << game.enPassant.column << '\n';
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& x = game.board.GetPiece({ r,c });
            out << static_cast<int>(x.type) << ' ' << static_cast<int>(x.color) << ' ' << static_cast<int>(x.hasMoved) << ' ';
        }
        out << '\n';
    }
    if (game.history.size() > 10000) { error = "Too many moves to save."; out.close(); std::remove(temporary.c_str()); return false; }
    out << game.history.size() << '\n';
    for (const auto& line : game.history) {
        if (line.size() > 512 || line.find('\n') != std::string::npos || line.find('\r') != std::string::npos) {
            error = "Invalid move history."; out.close(); std::remove(temporary.c_str()); return false;
        }
        out << line << '\n';
    }
    out.flush();
    if (!out.good()) { error = "Failed while writing save file."; out.close(); std::remove(temporary.c_str()); return false; }
    out.close();
    // Replace only after the entire temporary file has been written.
#ifdef _WIN32
    if (!MoveFileExA(temporary.c_str(), filename.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Unable to replace the existing save file."; std::remove(temporary.c_str()); return false;
    }
#else
    if (std::rename(temporary.c_str(), filename.c_str()) != 0) {
        error = "Unable to replace the existing save file."; std::remove(temporary.c_str()); return false;
    }
#endif
    error.clear(); return true;
}
bool SaveManager::Load(const std::string& filename, SavedGame& game, std::string& error) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) { error = "Save file not found."; return false; }
    SavedGame candidate;
    std::string line;
    if (!ReadLine(in, line) || line != Magic) { error = "Unknown or damaged save format."; return false; }
    int turn, we, be;
    if (!ReadLine(in, line) || !ParseInts(line, { &turn,&candidate.fullMove,&candidate.whiteTurns,&candidate.blackTurns }) ||
        !ValidColor(turn) || candidate.fullMove < 1 || candidate.fullMove>1000000 || candidate.whiteTurns < 0 || candidate.blackTurns < 0 ||
        candidate.whiteTurns>1000000 || candidate.blackTurns>1000000 ||
        candidate.whiteTurns + candidate.blackTurns != static_cast<long long>(candidate.fullMove - 1) * 2 + (turn == 1 ? 1 : 0)) {
        error = "Invalid turn counters."; return false;
    }
    candidate.turn = static_cast<PlayerColor>(turn);
    if (!ReadLine(in, line) || !ParseInts(line, { &we,&candidate.whiteMana,&candidate.whiteLastEnchant }) || !ValidElement(we) ||
        candidate.whiteMana < 0 || candidate.whiteMana>100 || candidate.whiteLastEnchant< -1 || candidate.whiteLastEnchant>candidate.whiteTurns) {
        error = "Invalid White player data."; return false;
    }
    candidate.whiteElement = static_cast<Element>(we);
    if (!ReadLine(in, line) || !ParseInts(line, { &be,&candidate.blackMana,&candidate.blackLastEnchant }) || !ValidElement(be) ||
        candidate.blackMana < 0 || candidate.blackMana>100 || candidate.blackLastEnchant< -1 || candidate.blackLastEnchant>candidate.blackTurns) {
        error = "Invalid Black player data."; return false;
    }
    candidate.blackElement = static_cast<Element>(be);
    if (!ReadLine(in, line) || !ParseInts(line, { &candidate.enPassant.row,&candidate.enPassant.column }) ||
        !(candidate.enPassant == Position{ -1,-1 } ||
            ((candidate.enPassant.row == 2 || candidate.enPassant.row == 5) && candidate.enPassant.column >= 0 && candidate.enPassant.column < 8))) {
        error = "Invalid en passant square."; return false;
    }
    int whiteKings = 0, blackKings = 0;
    for (int r = 0; r < 8; ++r) {
        if (!ReadLine(in, line, 1024)) { error = "Incomplete board data."; return false; }
        std::istringstream row(line); int t, c, m; char extra;
        for (int col = 0; col < 8; ++col) {
            if (!(row >> t >> c >> m) || !ValidPiece(t) || !ValidColor(c) || (m != 0 && m != 1)) { error = "Invalid piece data."; return false; }
            if (t == static_cast<int>(PieceType::King)) { if (c == 0)++whiteKings; else ++blackKings; }
            candidate.board.SetPiece({ r,col }, { static_cast<PieceType>(t),static_cast<PlayerColor>(c),m != 0 });
        }
        if (row >> extra) { error = "Unexpected board data."; return false; }
    }
    if (whiteKings != 1 || blackKings != 1) { error = "Save must contain exactly one king per player."; return false; }
    if (!ReadLine(in, line)) { error = "Missing move history."; return false; }
    size_t count = 0; { std::istringstream s(line); char extra; if (!(s >> count) || (s >> extra) || count > 10000) { error = "Invalid history count."; return false; } }
    for (size_t i = 0; i < count; ++i) {
        if (!ReadLine(in, line)) { error = "Incomplete move history."; return false; }
        candidate.history.push_back(line);
    }
    if (candidate.history.size() != static_cast<size_t>(candidate.whiteTurns + candidate.blackTurns)) { error = "Move count does not match turns."; return false; }
    candidate.lastMove = candidate.history.empty() ? "" : candidate.history.back();
    if (in.peek() != std::char_traits<char>::eof()) { error = "Unexpected trailing save data."; return false; }
    game = std::move(candidate); error.clear(); return true;
}
