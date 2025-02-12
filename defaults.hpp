// Default boards.
#include <array>
#include <cstdint>
#include <unordered_map>

#ifndef DEFAULTS_H
#define DEFAULTS_H

// 8 bits startsquare, 8 bits end square.
typedef uint8_t Square;
typedef uint8_t Piece;
typedef uint64_t BitBoard;
typedef uint8_t Flag;
typedef uint8_t Rank;
typedef uint8_t File;
typedef std::pair<Square, Square> sq_pair;
typedef uint8_t CastlingRights;
typedef uint8_t MoveType;
typedef uint64_t NodeCount;

// Default bitboards.
constexpr uint64_t INIT_ROOK_SQUARES = 0b1000000100000000000000000000000000000000000000000000000010000001ULL;
constexpr uint64_t INIT_KNIGHT_SQUARES = 0b0100001000000000000000000000000000000000000000000000000001000010ULL;
constexpr uint64_t INIT_BISHOP_SQUARES = 0b0010010000000000000000000000000000000000000000000000000000100100ULL;
constexpr uint64_t INIT_QUEEN_SQUARES = 0b0001000000000000000000000000000000000000000000000000000000010000ULL;
constexpr uint64_t INIT_KING_SQUARES = 0b0000100000000000000000000000000000000000000000000000000000001000ULL;
constexpr uint64_t INIT_PAWN_SQUARES = 0b0000000011111111000000000000000000000000000000001111111100000000ULL;
constexpr uint64_t INIT_BLACK_PIECES = 0b1111111111111111000000000000000000000000000000000000000000000000ULL;
constexpr uint64_t INIT_TOTAL_SQUARES = 0b1111111111111111000000000000000000000000000000001111111111111111ULL;
constexpr uint64_t INIT_WHITE_PIECES = 0b0000000000000000000000000000000000000000000000001111111111111111ULL;

// Pieces.
#define PAWN	   1
#define KING	   2
#define ROOK	   3
#define BISHOP	   4
#define KNIGHT	   5
#define QUEEN	   6
#define QUEEN_DIAG 7
#define QUEEN_ORTH 8
#define EMPTY	   0

#define occb   b.occ_board
#define popcnt __builtin_popcountll

enum class PawnMoveType { ATTACKS, FORWARD, DOUBLE_FORWARD, NON_DOUBLE, ALL };

const std::unordered_map<Square, uint8_t> get_castle_code{{62, 0}, {58, 1}, {6, 2}, {2, 3}};

inline constexpr BitBoard promotion_to_squares = (0b1111'1111ULL) | (0b1111'1111ULL << 56);
inline constexpr BitBoard pawn_start_w = INIT_PAWN_SQUARES & INIT_WHITE_PIECES;
inline constexpr BitBoard pawn_start_b = INIT_PAWN_SQUARES & INIT_BLACK_PIECES;
inline constexpr BitBoard promotion_from_w = pawn_start_b;
inline constexpr BitBoard promotion_from_b = pawn_start_w;

inline constexpr uint8_t king_start_squares[4] = {60, 60, 4, 4};
inline constexpr uint8_t rook_start_squares[4] = {63, 56, 7, 0};
inline constexpr uint8_t king_end_squares[4] = {62, 58, 6, 2};
inline constexpr uint8_t rook_end_squares[4] = {61, 59, 5, 3};
inline constexpr uint8_t queenside_middle_squares[4] = {1, 57};

inline constexpr std::array<Piece, 6> non_king_pieces = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, EMPTY};
inline constexpr std::array<Piece, 7> pieces = {PAWN, KING, KNIGHT, BISHOP, ROOK, QUEEN, EMPTY};
inline constexpr BitBoard no_edges = 0b0111'1110'0111'1110'0111'1110'0111'1110'0111'1110'0111'1110'0111'1110'0111'1110;

#define bq_mask (0b0001)
#define bk_mask (0b0010)
#define wq_mask (0b0100)
#define wk_mask (0b1000)

#define can_capture_left(b)	 b & 0x7F7F7F7F7F7F7F7F
#define can_capture_right(b) b & 0xFEFEFEFEFEFEFEFE

#endif	// !DEFAULTS_H
