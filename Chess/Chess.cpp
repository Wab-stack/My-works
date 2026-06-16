#include <iostream>
#include <string>
#include <cstdint> //uint64_t
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <fcntl.h> //Для Фигур
#include <io.h> //Для широких символов
#include <fstream> 

/*
Полноценные шахматы с консольным интерфейсом, сохранением и загрузкой партий. 
В основе лежит битбордная модель – позиция хранится в 64‑битных целых числах (Bitboard = uint64_t), 
каждый бит которых соответствует одной клетке доски. 
Собиралось на MVSC, ISO C++ 17
*/

using Bitboard = uint64_t;

constexpr Bitboard FILE_A = 0x0101010101010101ULL; //Биты на вертикали
constexpr Bitboard FILE_B = FILE_A << 1;
constexpr Bitboard FILE_C = FILE_A << 2;
constexpr Bitboard FILE_D = FILE_A << 3;
constexpr Bitboard FILE_E = FILE_A << 4;
constexpr Bitboard FILE_F = FILE_A << 5;
constexpr Bitboard FILE_G = FILE_A << 6;
constexpr Bitboard FILE_H = FILE_A << 7;

constexpr Bitboard RANK_1 = 0x00000000000000FFULL; //Биты на горизонтали
constexpr Bitboard RANK_2 = RANK_1 << 8;
constexpr Bitboard RANK_3 = RANK_2 << 8;
constexpr Bitboard RANK_4 = RANK_3 << 8;
constexpr Bitboard RANK_5 = RANK_4 << 8;
constexpr Bitboard RANK_6 = RANK_5 << 8;
constexpr Bitboard RANK_7 = RANK_6 << 8;
constexpr Bitboard RANK_8 = RANK_7 << 8;

constexpr Bitboard NOT_FILE_A = ~FILE_A;
constexpr Bitboard NOT_FILE_H = ~FILE_H;

//Преобразовываем комер клетки в битборд с 1 битом
inline Bitboard squareToBit(int square) { 
    return 1ULL << square;
}

inline int bitToSquare(Bitboard b) {
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (int)idx;
}

enum Color { WHITE, BLACK, COLOR_NONE };
enum PieceType { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

struct Piece {
    PieceType type;
    Color color;
    Piece(PieceType t = NONE, Color c = COLOR_NONE) : type(t), color(c) {}
    bool operator==(const Piece& other) const { 
        return type == other.type && color == other.color;
    }
};

class Board {
private:
    Bitboard pieces[7][2]; //[тип фигуры][цвет] – битборд для каждого типа/цвета
    Bitboard allPieces[2]; //битборд всех фигур указанного цвета
    Color sideToMove; //кто ходит: WHITE или BLACK
    int enPassantSquare; //клетка, на которой можно взять на проходе (или -1)
    int halfMoveClock; //счётчик полуходов для правила 50 ходов
    int fullMoveNumber; //номер полного хода (начинается с 1)
    Bitboard castlingRights; //Для рокировок

public:
    Board() { reset(); }

    //Начальная расстановка
    void reset() {
        for (int i = 0; i < 7; ++i) {
            pieces[i][WHITE] = 0;
            pieces[i][BLACK] = 0;
        }
        allPieces[WHITE] = 0;
        allPieces[BLACK] = 0;
        //устанавливаем начальную позицию
        pieces[PAWN][WHITE] = RANK_2;
        pieces[PAWN][BLACK] = RANK_7;
        const int A1 = 0, B1 = 1, C1 = 2, D1 = 3, E1 = 4, F1 = 5, G1 = 6, H1 = 7;
        const int A8 = 56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63;

        pieces[ROOK][WHITE] = squareToBit(A1) | squareToBit(H1);
        pieces[ROOK][BLACK] = squareToBit(A8) | squareToBit(H8);
        pieces[KNIGHT][WHITE] = squareToBit(B1) | squareToBit(G1);
        pieces[KNIGHT][BLACK] = squareToBit(B8) | squareToBit(G8);
        pieces[BISHOP][WHITE] = squareToBit(C1) | squareToBit(F1);
        pieces[BISHOP][BLACK] = squareToBit(C8) | squareToBit(F8);
        pieces[QUEEN][WHITE] = squareToBit(D1);
        pieces[QUEEN][BLACK] = squareToBit(D8);
        pieces[KING][WHITE] = squareToBit(E1);
        pieces[KING][BLACK] = squareToBit(E8);

        allPieces[WHITE] = pieces[PAWN][WHITE] | pieces[KNIGHT][WHITE] | pieces[BISHOP][WHITE] |
            pieces[ROOK][WHITE] | pieces[QUEEN][WHITE] | pieces[KING][WHITE];
        allPieces[BLACK] = pieces[PAWN][BLACK] | pieces[KNIGHT][BLACK] | pieces[BISHOP][BLACK] |
            pieces[ROOK][BLACK] | pieces[QUEEN][BLACK] | pieces[KING][BLACK];

        sideToMove = WHITE;
        enPassantSquare = -1;
        halfMoveClock = 0;
        fullMoveNumber = 1;
        castlingRights = 0b1111;
    }
    //Проверяем расстановку фигур
    Piece pieceAt(int square) const {
        Bitboard mask = squareToBit(square);
        for (int color = 0; color < 2; ++color) {
            for (int type = PAWN; type <= KING; ++type) {
                if (pieces[type][color] & mask) {
                    return Piece(static_cast<PieceType>(type), static_cast<Color>(color));
                }
            }
        }
        return Piece(NONE, COLOR_NONE);
    }
    //Проверяем наличие шаха и прочие ходы
    std::vector<std::pair<int, int>> generatePseudoMoves() const {
        std::vector<std::pair<int, int>> moves;
        Color us = sideToMove;
        Bitboard ourPieces = allPieces[us];
        Bitboard enemyPieces = allPieces[opposite(us)];
        Bitboard empty = ~(allPieces[WHITE] | allPieces[BLACK]);

        //Пешки
        Bitboard pawns = pieces[PAWN][us];
        if (us == WHITE) {
            //Обычный ход на 1 клетку
            Bitboard single = (pawns << 8) & empty;
            Bitboard temp = single;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to - 8;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            //Ход на 2 клетки
            Bitboard doublePush = ((pawns & RANK_2) << 16) & empty & (empty << 8);
            temp = doublePush;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to - 16;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            //Взятия
            Bitboard capturesLeft = ((pawns & NOT_FILE_A) << 9) & enemyPieces;
            temp = capturesLeft;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to - 9;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            Bitboard capturesRight = ((pawns & NOT_FILE_H) << 7) & enemyPieces;
            temp = capturesRight;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to - 7;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            //Взятие на проходе
            if (enPassantSquare != -1) {
                Bitboard epMask = squareToBit(enPassantSquare);
                Bitboard attackers = (epMask >> 9) & pawns & NOT_FILE_A;
                if (attackers) {
                    int from = bitToSquare(attackers);
                    moves.emplace_back(from, enPassantSquare);
                }
                attackers = (epMask >> 7) & pawns & NOT_FILE_H;
                if (attackers) {
                    int from = bitToSquare(attackers);
                    moves.emplace_back(from, enPassantSquare);
                }
            }
        }
        else { //Чёрные
            Bitboard single = (pawns >> 8) & empty;
            Bitboard temp = single;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to + 8;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            Bitboard doublePush = ((pawns & RANK_7) >> 16) & empty & (empty >> 8);
            temp = doublePush;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to + 16;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            Bitboard capturesLeft = ((pawns & NOT_FILE_A) >> 7) & enemyPieces;
            temp = capturesLeft;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to + 7;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            Bitboard capturesRight = ((pawns & NOT_FILE_H) >> 9) & enemyPieces;
            temp = capturesRight;
            while (temp) {
                int to = bitToSquare(temp);
                int from = to + 9;
                moves.emplace_back(from, to);
                temp &= temp - 1;
            }
            if (enPassantSquare != -1) {
                Bitboard epMask = squareToBit(enPassantSquare);
                Bitboard attackers = (epMask << 7) & pawns & NOT_FILE_A;
                if (attackers) {
                    int from = bitToSquare(attackers);
                    moves.emplace_back(from, enPassantSquare);
                }
                attackers = (epMask << 9) & pawns & NOT_FILE_H;
                if (attackers) {
                    int from = bitToSquare(attackers);
                    moves.emplace_back(from, enPassantSquare);
                }
            }
        }

        //Кони
        Bitboard knights = pieces[KNIGHT][us];
        while (knights) {
            int from = bitToSquare(knights);
            knights &= knights - 1;
            Bitboard toMask = knightAttackFrom(from) & ~ourPieces;
            while (toMask) {
                int to = bitToSquare(toMask);
                toMask &= toMask - 1;
                moves.emplace_back(from, to);
            }
        }

        //Ладьи
        Bitboard rooks = pieces[ROOK][us];
        while (rooks) {
            int from = bitToSquare(rooks);
            rooks &= rooks - 1;
            Bitboard toMask = rookAttackFrom(from, allPieces[WHITE] | allPieces[BLACK]) & ~ourPieces;
            while (toMask) {
                int to = bitToSquare(toMask);
                toMask &= toMask - 1;
                moves.emplace_back(from, to);
            }
        }

        //Слоны
        Bitboard bishops = pieces[BISHOP][us];
        while (bishops) {
            int from = bitToSquare(bishops);
            bishops &= bishops - 1;
            Bitboard toMask = bishopAttackFrom(from, allPieces[WHITE] | allPieces[BLACK]) & ~ourPieces;
            while (toMask) {
                int to = bitToSquare(toMask);
                toMask &= toMask - 1;
                moves.emplace_back(from, to);
            }
        }

        //Ферзи
        Bitboard queens = pieces[QUEEN][us];
        while (queens) {
            int from = bitToSquare(queens);
            queens &= queens - 1;
            Bitboard toMask = (rookAttackFrom(from, allPieces[WHITE] | allPieces[BLACK]) |
                bishopAttackFrom(from, allPieces[WHITE] | allPieces[BLACK])) & ~ourPieces;
            while (toMask) {
                int to = bitToSquare(toMask);
                toMask &= toMask - 1;
                moves.emplace_back(from, to);
            }
        }

        //Король ходы
        int kingPos = bitToSquare(pieces[KING][us]);
        if (kingPos != -1) {
            Bitboard kingMoves = kingAttackFrom(kingPos) & ~ourPieces;
            while (kingMoves) {
                int to = bitToSquare(kingMoves);
                kingMoves &= kingMoves - 1;
                moves.emplace_back(kingPos, to);
            }
        }

        //Рокировки
        if (us == WHITE) {
            if ((castlingRights & 1) && !isSquareAttacked(4, BLACK) && !isSquareAttacked(5, BLACK) && !isSquareAttacked(6, BLACK) &&
                (allPieces[WHITE] & squareToBit(5)) == 0 && (allPieces[WHITE] & squareToBit(6)) == 0) {
                moves.emplace_back(4, 6); //Короткая рокировка
            }
            if ((castlingRights & 2) && !isSquareAttacked(4, BLACK) && !isSquareAttacked(3, BLACK) && !isSquareAttacked(2, BLACK) &&
                (allPieces[WHITE] & squareToBit(3)) == 0 && (allPieces[WHITE] & squareToBit(2)) == 0) {
                moves.emplace_back(4, 2); //Длинная рокировка
            }
        }
        else {
            if ((castlingRights & 4) && !isSquareAttacked(60, WHITE) && !isSquareAttacked(61, WHITE) && !isSquareAttacked(62, WHITE) &&
                (allPieces[BLACK] & squareToBit(61)) == 0 && (allPieces[BLACK] & squareToBit(62)) == 0) {
                moves.emplace_back(60, 62);
            }
            if ((castlingRights & 8) && !isSquareAttacked(60, WHITE) && !isSquareAttacked(59, WHITE) && !isSquareAttacked(58, WHITE) &&
                (allPieces[BLACK] & squareToBit(59)) == 0 && (allPieces[BLACK] & squareToBit(58)) == 0) {
                moves.emplace_back(60, 58);
            }
        }

        return moves;
    }

    void makeMove(int from, int to, PieceType promotion = QUEEN) {
        Piece moving = pieceAt(from);
        Piece captured = pieceAt(to);
        bool isCapture = captured.type != NONE;
        bool isPawnMove = moving.type == PAWN;

        if (isPawnMove || isCapture) {
            halfMoveClock = 0;
        }
        else {
            halfMoveClock++;
        }

        if (sideToMove == BLACK) {
            fullMoveNumber++;
        }

        removePiece(from);
        if (isCapture) {
            removePiece(to);
        }

        if (moving.type == PAWN && to == enPassantSquare) {
            int captureSquare = (sideToMove == WHITE) ? to - 8 : to + 8;
            removePiece(captureSquare);
        }

        addPiece(to, moving.type, moving.color);

        if (moving.type == PAWN && (to >= 56 || to <= 7)) {
            removePiece(to);
            addPiece(to, promotion, moving.color);
        }
        //Передвижение ладьи при рокировке
        if (moving.type == KING && std::abs(to - from) == 2) {
            int rookFrom, rookTo;
            if (to > from) { 
                rookFrom = from + 3; rookTo = from + 1; 
            }
            else { 
                rookFrom = from - 4; rookTo = from - 1;
            }
            Piece rook = pieceAt(rookFrom);
            if (rook.type == ROOK) {
                removePiece(rookFrom);
                addPiece(rookTo, ROOK, rook.color);
            }
        }

        if (moving.type == PAWN && std::abs(to - from) == 16) {
            enPassantSquare = (from + to) / 2;
        }
        else {
            enPassantSquare = -1;
        }

        updateCastlingRights(from, to, moving.type);
        sideToMove = opposite(sideToMove);
    }

    bool isKingInCheck() const {
        Color us = sideToMove;
        int kingSquare = bitToSquare(pieces[KING][us]);
        if (kingSquare == -1) {
            return false;
        }
        return isSquareAttacked(kingSquare, opposite(us));
    }

    bool isSquareAttacked(int square, Color attacker) const {
        Bitboard target = squareToBit(square);
        //пешки
        Bitboard pawns = pieces[PAWN][attacker];
        if (attacker == WHITE) {
            if ((pawns & (target >> 7) & NOT_FILE_H) || (pawns & (target >> 9) & NOT_FILE_A)) {
                return true;
            }
        }
        else {
            if ((pawns & (target << 7) & NOT_FILE_A) || (pawns & (target << 9) & NOT_FILE_H)) {
                return true;
            }
        }
        //кони
        if (pieces[KNIGHT][attacker] & knightAttackFrom(square)) {
            return true;
        }
        //король
        if (pieces[KING][attacker] & kingAttackFrom(square)) {
            return true;
        }
        //ладьи и ферзи (горизонталь/вертикаль)
        Bitboard rookQueen = pieces[ROOK][attacker] | pieces[QUEEN][attacker];
        if (rookQueen & rookAttackFrom(square, allPieces[WHITE] | allPieces[BLACK])) {
            return true;
        }
        //слоны и ферзи (диагональ)
        Bitboard bishopQueen = pieces[BISHOP][attacker] | pieces[QUEEN][attacker];
        if (bishopQueen & bishopAttackFrom(square, allPieces[WHITE] | allPieces[BLACK])) {
            return true;
        }
        return false;
    }
    //Проверка на легальные ходы, если хоть один существует возвращает true
    bool hasLegalMoves() const {
        auto moves = generatePseudoMoves();
        for (auto [from, to] : moves) {
            Board copy = *this;
            copy.makeMove(from, to);
            if (!copy.isKingInCheck()) {
                return true;
            }
        }
        return false;
    }

    void print() const {
        //Верхняя строка с буквами
        std::wcout << L"    a   b   c   d   e   f   g   h\n";
        std::wcout << L"  +---+---+---+---+---+---+---+---+\n";
        for (int rank = 7; rank >= 0; --rank) {
            std::wcout << (rank + 1) << L" | ";
            for (int file = 0; file < 8; ++file) {
                int square = rank * 8 + file;
                Piece p = pieceAt(square);
                wchar_t ch = L' ';
                if (p.type != NONE) {
                    const wchar_t* whiteSymbols = L" ♙♘♗♖♕♔";
                    const wchar_t* blackSymbols = L" ♟♞♝♜♛♚";
                    if (p.color == WHITE) {
                        ch = whiteSymbols[p.type];
                    }
                    else {
                        ch = blackSymbols[p.type];
                    }
                }
                std::wcout << ch << L" | ";
            }
            std::wcout << L"\n";
            if (rank > 0) {
                std::wcout << L"  +---+---+---+---+---+---+---+---+\n";
            }
        }
        std::wcout << L"  +---+---+---+---+---+---+---+---+\n";
        std::wcout << L"    a   b   c   d   e   f   g   h\n";
        std::wcout << L"Side to move: " << (sideToMove == WHITE ? L"White" : L"Black") << L"\n";
    }
    
    bool makeMoveFromString(const std::string& str) {
        if (str.length() < 4) {
            return false;
        }
        int fromFile = str[0] - 'a';
        int fromRank = str[1] - '1';
        int toFile = str[2] - 'a';
        int toRank = str[3] - '1';
        if (fromFile < 0 || fromFile >= 8 || fromRank < 0 || fromRank >= 8 ||
            toFile < 0 || toFile >= 8 || toRank < 0 || toRank >= 8) {
            return false;
        }
        int from = fromRank * 8 + fromFile;
        int to = toRank * 8 + toFile;

        PieceType promotion = QUEEN;
        if (str.length() == 5) {
            char prom = str[4];
            switch (prom) {
            case 'n': {
                promotion = KNIGHT;
                break;
            }
            case 'b': {
                promotion = BISHOP;
                break;
            }
            case 'r': {
                promotion = ROOK;
                break;
            }
            default: {
                promotion = QUEEN;
            }
            }
        }

        auto moves = generatePseudoMoves();
        for (auto [f, t] : moves) {
            if (f == from && t == to) {
                Board copy = *this;
                copy.makeMove(from, to, promotion);
                if (!copy.isKingInCheck()) {
                    makeMove(from, to, promotion);
                    return true;
                }
            }
        }
        return false;
    }

    Color getSideToMove() const { return sideToMove; }

    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) {
            return false;
        }

        //Сохраняем доску в виде 8 строк по 8 символов
        for (int rank = 7; rank >= 0; --rank) {
            for (int fileIdx = 0; fileIdx < 8; ++fileIdx) {
                int square = rank * 8 + fileIdx;
                Piece p = pieceAt(square);
                char ch = '.';
                if (p.type != NONE) {
                    const char* white = " PNBRQK";
                    const char* black = " pnbrqk";
                    if (p.color == WHITE) {
                        ch = white[p.type];
                    }
                    else {
                        ch = black[p.type];
                    }
                }
                file << ch;
            }
            file << '\n';
        }

        //Сохраняем параметры: сторона, en passant, рокировки, полуходы, номер хода
        file << (sideToMove == WHITE ? 'w' : 'b') << '\n';

        //enPassantSquare: координата или '-'
        if (enPassantSquare == -1)
            file << "-\n";
        else {
            int fileIdx = enPassantSquare % 8;
            int rank = enPassantSquare / 8;
            file << char('a' + fileIdx) << (rank + 1) << '\n';
        }

        //Права рокировки (KQkq)
        std::string castleStr = "";
        if (castlingRights & 1) {
            castleStr += 'K';
        }
        if (castlingRights & 2) {
            castleStr += 'Q';
        }
        if (castlingRights & 4) {
            castleStr += 'k';
        }
        if (castlingRights & 8) {
            castleStr += 'q';
        }
        if (castleStr.empty()) castleStr = "-";
        file << castleStr << '\n';

        file << halfMoveClock << '\n';
        file << fullMoveNumber << '\n';

        file.close();
        return true;
    }

    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            return false;
        }

        std::string line;
        //Читаем 8 строк доски
        for (int rank = 7; rank >= 0; --rank) {
            if (!std::getline(file, line) || line.length() < 8) {
                return false;
            }
            for (int fileIdx = 0; fileIdx < 8; ++fileIdx) {
                int square = rank * 8 + fileIdx;
                char ch = line[fileIdx];
                //Удаляем фигуру с этого квадрата (если была)
                removePiece(square);
                if (ch == '.') {
                    continue;
                }

                Color color;
                PieceType type;
                if (ch >= 'A' && ch <= 'Z') {
                    color = WHITE;
                    ch = tolower(ch);
                }
                else {
                    color = BLACK;
                }
                switch (ch) {
                case 'p': {
                    type = PAWN;
                    break;
                }
                case 'n': {
                    type = KNIGHT;
                    break;
                }
                case 'b': {
                    type = BISHOP;
                    break;
                }
                case 'r': {
                    type = ROOK;
                    break;
                }
                case 'q': {
                    type = QUEEN;
                    break;
                }
                case 'k': {
                    type = KING;
                    break;
                }
                default: {
                    return false;
                }
                }
                addPiece(square, type, color);
            }
        }

        if (!(file >> line) || line.empty()) {
            return false;
        }
        sideToMove = (line[0] == 'w') ? WHITE : BLACK;

        if (!(file >> line)) {
            return false;
        }
        if (line == "-") {
            enPassantSquare = -1;
        }
        else if (line.length() == 2) {
            int fileIdx = line[0] - 'a';
            int rank = line[1] - '1';
            if (fileIdx >= 0 && fileIdx < 8 && rank >= 0 && rank < 8){
                enPassantSquare = rank * 8 + fileIdx;
            }
            else {
                enPassantSquare = -1;
            }
        }
        else {
            enPassantSquare = -1;
        }

        if (!(file >> line)) {
            return false;
        }
        castlingRights = 0;
        for (char c : line) {
            if (c == 'K') {
                castlingRights |= 1;
            }
            else if (c == 'Q') {
                castlingRights |= 2;
            }
            else if (c == 'k') {
                castlingRights |= 4;
            }
            else if (c == 'q') {
                castlingRights |= 8;
            }
        }

        if (!(file >> halfMoveClock)) {
            return false;
        }
        if (!(file >> fullMoveNumber)) {
            return false;
        }

        //После чтения чисел нужно сбросить состояние потока для дальнейшего ввода
        file.ignore();

        return true;
    }

private:
    Color opposite(Color c) const { return (c == WHITE) ? BLACK : WHITE; }

    void addPiece(int square, PieceType type, Color color) {
        Bitboard mask = squareToBit(square);
        pieces[type][color] |= mask;
        allPieces[color] |= mask;
    }

    void removePiece(int square) {
        Bitboard mask = squareToBit(square);
        for (int type = PAWN; type <= KING; ++type) {
            for (int color = 0; color < 2; ++color) {
                if (pieces[type][color] & mask) {
                    pieces[type][color] ^= mask;
                    allPieces[color] ^= mask;
                    return;
                }
            }
        }
    }

    static Bitboard rookAttackFrom(int sq, Bitboard blockers) {
        int rank = sq / 8, file = sq % 8;
        Bitboard attacks = 0;
        for (int r = rank + 1; r < 8; ++r) {
            attacks |= squareToBit(r * 8 + file);
            if (blockers & squareToBit(r * 8 + file)) {
                break;
            }
        }
        for (int r = rank - 1; r >= 0; --r) {
            attacks |= squareToBit(r * 8 + file);
            if (blockers & squareToBit(r * 8 + file)) {
                break;
            }
        }
        for (int f = file + 1; f < 8; ++f) {
            attacks |= squareToBit(rank * 8 + f);
            if (blockers & squareToBit(rank * 8 + f)) {
                break;
            }
        }
        for (int f = file - 1; f >= 0; --f) {
            attacks |= squareToBit(rank * 8 + f);
            if (blockers & squareToBit(rank * 8 + f)) {
                break;
            }
        }
        return attacks;
    }

    static Bitboard bishopAttackFrom(int sq, Bitboard blockers) {
        int rank = sq / 8, file = sq % 8;
        Bitboard attacks = 0;
        for (int r = rank + 1, f = file + 1; r < 8 && f < 8; ++r, ++f) {
            attacks |= squareToBit(r * 8 + f);
            if (blockers & squareToBit(r * 8 + f)) {
                break;
            }
        }
        for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; ++r, --f) {
            attacks |= squareToBit(r * 8 + f);
            if (blockers & squareToBit(r * 8 + f)) {
                break;
            }
        }
        for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; --r, ++f) {
            attacks |= squareToBit(r * 8 + f);
            if (blockers & squareToBit(r * 8 + f)) {
                break;
            }
        }
        for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; --r, --f) {
            attacks |= squareToBit(r * 8 + f);
            if (blockers & squareToBit(r * 8 + f)) {
                break;
            }
        }
        return attacks;
    }

    static Bitboard knightAttackFrom(int sq) {
        const int offsets[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
        Bitboard attacks = 0;
        int r = sq / 8, f = sq % 8;
        for (int off : offsets) {
            int to = sq + off;
            if (to < 0 || to >= 64) {
                continue;
            }
            int tr = to / 8, tf = to % 8;
            if (abs(tr - r) <= 2 && abs(tf - f) <= 2 && !(abs(tr - r) == 0 || abs(tf - f) == 0)) {
                attacks |= squareToBit(to);
            }
        }
        return attacks;
    }

    static Bitboard kingAttackFrom(int sq) {
        Bitboard attacks = 0;
        int r = sq / 8, f = sq % 8;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int df = -1; df <= 1; ++df) {
                if (dr == 0 && df == 0) {
                    continue;
                }
                int nr = r + dr, nf = f + df;
                if (nr >= 0 && nr < 8 && nf >= 0 && nf < 8) {
                    attacks |= squareToBit(nr * 8 + nf);
                }
            }
        }
        return attacks;
    }

    void updateCastlingRights(int from, int to, PieceType movingType) {
        if (movingType == KING) {
            if (sideToMove == WHITE) {
                castlingRights &= ~0b11;
            }
            else {
                castlingRights &= ~0b1100;
            }
        }
        if (movingType == ROOK) {
            if (from == 0) {
                castlingRights &= ~0b01;
            }
            if (from == 7) {
                castlingRights &= ~0b10;
            }
            if (from == 56) {
                castlingRights &= ~0b0100;
            }
            if (from == 63) {
                castlingRights &= ~0b1000;
            }
        }
        Piece captured = pieceAt(to);
        if (captured.type == ROOK) {
            if (to == 0) {
                castlingRights &= ~0b01;
            }
            if (to == 7) {
                castlingRights &= ~0b10;
            }
            if (to == 56) {
                castlingRights &= ~0b0100;
            }
            if (to == 63) {
                castlingRights &= ~0b1000;
            }
        }
    }
};

void gameLoop(Board& board) {
    std::string input;
    while (true) {
        board.print();
        if (!board.hasLegalMoves()) {
            if (board.isKingInCheck()) {
                std::wcout << L"Checkmate! " << (board.getSideToMove() == WHITE ? L"White" : L"Black") << L" loses.\n";
            }
            else {
                std::wcout << L"Stalemate. Draw.\n";
            }
            break;
        }
        std::wcout << L"Enter move (e.g., e2e4) or 'save' to save or 'quit' to main menu: ";
        std::cin >> input;
        if (input == "quit") {
            break;
        }
        else if (input == "save") {
            std::wcout << L"Enter filename to save: ";
            std::string fname;
            std::cin >> fname;
            if (board.saveToFile(fname)) {
                std::wcout << L"Game saved.\n";
            }
            else {
                std::wcout << L"Save failed.\n";
            }
            continue;
        }
        if (board.makeMoveFromString(input)) {
            std::wcout << L"Move accepted.\n";
        }
        else {
            std::wcout << L"Invalid move. Try again.\n";
        }
    }
}

void showRules() {
    std::wcout << L"\n=== RULES ===\n";
    std::wcout << L"- Moves are entered as four characters: starting square and ending square,\n";
    std::wcout << L"  e.g., e2e4, g1f3, e1g1 (for castling).\n";
    std::wcout << L"- For pawn promotion, add fifth character: q (queen), r (rook), b (bishop), n (knight).\n";
    std::wcout << L"  Example: a7a8q\n";
    std::wcout << L"- Castling: move the king two squares towards the rook; the rook moves automatically.\n";
    std::wcout << L"  Example: e1g1 (white short) or e8c8 (black long).\n";
    std::wcout << L"- En passant is handled automatically.\n";
    std::wcout << L"- Save: type 'save' during the game, then enter filename.\n";
    std::wcout << L"- Quit to main menu: type 'quit' during the game.\n";
    std::wcout << L"Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

int main() {
    _setmode(_fileno(stdout), _O_U16TEXT);
    Board board;
    std::string input;
    std::wcout << L"\n=== Welcome to CHESS PVP ===\n";
    while (true) {
        std::wcout << L"\n=== MAIN MENU ===\n";
        std::wcout << L"1. New game\n";
        std::wcout << L"2. Load game\n";
        std::wcout << L"3. Rules\n";
        std::wcout << L"0. Quit\n";
        std::wcout << L"Your choice: ";

        std::string choice;
        std::cin >> choice;
        if (choice == "0") {
            std::wcout << L"Goodbye!\n";
            break;
        }
        else if (choice == "1") {
            board.reset();
            std::wcout << L"New game started.\n";
            //Игровой цикл
            gameLoop(board);
        }
        else if (choice == "2") {
            std::wcout << L"Enter filename to load: ";
            std::string fname;
            std::cin >> fname;
            if (board.loadFromFile(fname)) {
                std::wcout << L"Game loaded.\n";
                gameLoop(board);
            }
            else {
                std::wcout << L"Failed to load game.\n";
            }
        }
        else if (choice == "3") {
            showRules();
        }
        else {
            std::wcout << L"Invalid choice.\n";
        }
    }
    return 0;
}