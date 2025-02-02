#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "board.hpp"
#include "gamestate.hpp"
#include "maskset.hpp"
#include "move.hpp"
#include "stack.hpp"

#ifndef POSITION_H
#define POSITION_H

struct Position {
	Position() {}

	Board board;
	uint8_t ep_flag;
	bool white_turn = true;
	Stack<MaskSet> masks;

	template <bool white, Piece p>
	inline BitBoard piecebrd() {
		return board.get_piece_board<white, p>();
	}

	// Returns whether a square is under attack.
	template <bool white>
	inline bool is_attacked(Square square) {
		return (get_pawn_move<white, PawnMoveType::ATTACKS>(square_to_mask(square), board.occ_board)
				& board.get_piece_board<!white, PAWN>())
			|| (get_knight_move(square) & board.get_piece_board<!white, KNIGHT>())
			|| (get_rook_move(square, board.occ_board)
				& (board.get_piece_board<!white, ROOK>() | board.get_piece_board<!white, QUEEN>()))
			|| (get_bishop_move(square, board.occ_board)
				& (board.get_piece_board<!white, BISHOP>() | board.get_piece_board<!white, QUEEN>()))
			|| (get_king_move(square) & board.get_piece_board<!white, KING>());
	}
};

#endif
