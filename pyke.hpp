#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "gamestate.hpp"
#include "make_move.hpp"
#include "maskset.hpp"
#include "move.hpp"
#include "piece_moves.hpp"
#include "position.hpp"
#include "stack.hpp"
#include "util.hpp"

#ifndef PYKE_H
#define PYKE_H

namespace pyke {

template <bool white, int depth_to_go, bool print_move>
uint64_t count_moves(Position& pos);

// Returns whether a square is under attack.
template <bool white>
static inline bool is_attacked(Square square, Board& b) {
	using namespace piece_move;
	BitBoard& c = b.occ_board;
	return (get_pawn_move<white, PawnMoveType::ATTACKS>(square, b.occ_board) & b.get_piece_board<!white, PAWN>())
		|| (get_knight_move(square) & b.get_piece_board<!white, KNIGHT>())
		|| (get_rook_move(square, c) & (b.get_piece_board<!white, ROOK>() | b.get_piece_board<!white, QUEEN>()))
		|| (get_bishop_move(square, c) & (b.get_piece_board<!white, BISHOP>() | b.get_piece_board<!white, QUEEN>()))
		|| (get_king_move(square) & b.get_piece_board<!white, KING>());
}

// Returns the reach of a given piece. For pawns, it returns the reach without double push.
template <bool white, Piece piece>
static inline BitBoard make_reach_board(Square square, Board& b) {
	BitBoard& occ = b.occ_board;
	switch (piece) {
	case PAWN:
		return piece_move::get_pawn_move<white, PawnMoveType::NON_DOUBLE>(square, b.occ_board);
	case KING:
		return piece_move::get_king_move(square);
	case ROOK:
		return piece_move::get_rook_move(square, occ);
	case BISHOP:
		return piece_move::get_bishop_move(square, occ);
	case KNIGHT:
		return piece_move::get_knight_move(square);
	case QUEEN:
		return piece_move::get_queen_move(square, occ);
	default:
		throw std::invalid_argument("Illegal piece given.");
	}
}

template <bool white, int depth_to_go, bool print_move>
static inline uint64_t generate_ep_moves(Position& pos, Square king_sq) {
	// Board cpy = pos.board.copy();
	uint64_t ret = 0;
	uint8_t ep = pos.gamestate.get_en_passant();
	// In most cases, no ep is possible.
	if (!ep) return 0;
	auto make_en_passant = [&](int8_t from_offset) {
		uint64_t loc_ret = 0;
		// Rank to move to.
		File to = ep & 0b00001111;
		// From which file the pawn comes.
		File from = to + from_offset;
		// The start and end rank.
		Rank start_rank = white ? 3 : 4;
		Rank end_rank = white ? 2 : 5;
		// Start and end square.
		Square start_square = from + start_rank * 8;
		Square end_square = to + end_rank * 8;

		Board cp = pos.board.copy();
		ep_move<white>(start_square, end_square, pos);
		if (!is_attacked<white>(king_sq, pos.board)) {
			if constexpr (depth_to_go <= 1)
				loc_ret += 1;
			else
				loc_ret += 1 + count_moves<white, depth_to_go - 1, false>(pos);
		}
		unmake_ep_move<white>(start_square, end_square, pos);

		if (loc_ret && print_move)
			std::cout << make_chess_notation(start_square) << make_chess_notation(end_square) << ": "
					  << std::to_string(loc_ret) << '\n';
		return loc_ret;
	};

	if (ep & 0b1000'0000) ret += make_en_passant(-1);
	if (ep & 0b0100'0000) ret += make_en_passant(1);

	/*
	 if (!cpy.is_equal(pos.board)) {
		std::cout << "EEEEEEEEEEEEEEEEEEERHMMM" << '\n';
	}
	*/

	return ret;
}

// Create the castling move for given player and direction.
template <bool white, bool kingside, int depth_to_go, bool print_move>
static inline uint64_t generate_castle_move(Position& pos, Square king_square) {
	Board& b = pos.board;

	bool has_right = white ? pos.gamestate.can_castle_king<white>() : pos.gamestate.can_castle_queen<white>();
	Square to = white ? (kingside ? 62 : 58) : (kingside ? 6 : 2);
	Square middle_square = white ? (kingside ? 61 : 59) : (kingside ? 5 : 3);

	bool middle_attacked = is_attacked<white>(middle_square, b);
	bool to_attacked = is_attacked<white>(to, b);

	bool to_occ = b.square_occ(to);
	bool middle_occ = b.square_occ(middle_square);

	// Check if the castling move is legal.
	if (!has_right || middle_attacked || to_attacked || to_occ || middle_occ) return 0;
	if constexpr (depth_to_go <= 1 && !print_move)
		return 1;
	else {
		const uint8_t code = white ? (kingside ? 0 : 1) : (kingside ? 2 : 3);
		castle_move<white, code>(pos);
		int ret = 1 + count_moves<!white, depth_to_go - 1, false>(pos);
		unmake_castle_move<white, code>(pos);
		return ret;
	}
}

// Create king moves.
template <bool white, int depth_to_go, bool print_move>
static inline uint64_t generate_king_moves(BitBoard cmt, Square king_square, Position& pos) {
	cmt &= piece_move::get_king_move(king_square);
	uint64_t ret = 0;
	while (cmt) {
		Square to = pop(cmt);
		uint64_t n = 0;
		if constexpr (depth_to_go < 1 && !print_move) {
			throw std::invalid_argument("Code should not reach this point.");
		} else {
			if (pos.board.square_occ(to)) {
				Piece captured = pos.board.get_piece<white>(to);
				capture_move_wrapper<white, KING>(king_square, to, pos, captured);
				if (!is_attacked<white>(to, pos.board)) n += count_moves<!white, depth_to_go - 1, false>(pos);
				unmake_capture_wrapper<white, KING>(king_square, to, pos, captured);
			} else {
				plain_move<white, KING>(king_square, to, pos);
				if (!is_attacked<white>(to, pos.board)) n += count_moves<!white, depth_to_go - 1, false>(pos);
				unmake_plain_move<white, KING>(king_square, to, pos);
			}
		}
		if (print_move && n)
			std::cout << make_chess_notation(king_square) << make_chess_notation(to) << ": " << std::to_string(n)
					  << '\n';
		ret += n;
	}
	return ret;
}

// Pawn double pushes.
template <bool white, int depth_to_go, bool print_move>
static inline uint64_t generate_pawn_double(BitBoard cmt, Position& pos, BitBoard source) {
	uint64_t ret = 0;
	if constexpr (depth_to_go <= 1 && !print_move) {
		ret = __builtin_popcountll(piece_move::get_pawn_double<white>(source, pos.board.occ_board) & cmt);
	} else {
		while (source) {
			Square from = pop(source);
			BitBoard to_board = cmt & piece_move::get_pawn_double<white>(square_to_mask(from), pos.board.occ_board);

			while (to_board) {
				Square to = pop(to_board);
				uint64_t n = 0;
				pawn_double<white>(from, to, pos);
				n += count_moves<!white, depth_to_go - 1, false>(pos);
				unmake_pawn_double<white>(from, to, pos);
				if constexpr (print_move)
					std::cout << make_chess_notation(from) << make_chess_notation(to) << ": " << std::to_string(n)
							  << '\n';
				ret += n;
			}
		}
	}
	return ret;
}

// Generate all possible promotion moves.
template <bool white, int depth_to_go, bool print_move>
static inline uint64_t generate_promotions(Position& pos, BitBoard cmt, BitBoard source) {
	return 0;
	/*all         Board::is_equal(Board const&
	const auto generate_promo_pieces = [&](const Piece piece, const Piece captured, Square from, Square to) {
		promo_move<white, piece, captured>(from, to, pos);
		int count = 1 + count_moves<!white, depth_to_go - 1>(pos);
		unmake_promo_move<white, piece, captured>(from, to, pos);
		return count;
	};
	int ret = 0;
	while (source) {
		Square from = pop(source);
		while (cmt) {
			Square to = pop(cmt);
			if constexpr (depth_to_go <= 1) {
				return 4;
			} else {
				const Piece at_to = pos.board.get_piece<white>(to);
				ret += generate_promo_pieces(ROOK, at_to, from, to);
				ret += generate_promo_pieces(KNIGHT, at_to, from, to);
				ret += generate_promo_pieces(BISHOP, at_to, from, to);
				ret += generate_promo_pieces(QUEEN, at_to, from, to);
			}
		}
	}
	return ret;
*/
}

// Push moves from a given position to the target squares.
template <bool white, Piece p, bool capture, int depth_to_go, bool print_move>
static inline uint64_t generate_move_or_capture(BitBoard cmt, Square from, Position& pos) {
	uint64_t ret = 0;
	while (cmt) {
		uint64_t n = 0;
		Square to = pop(cmt);
		if constexpr (depth_to_go <= 1 && !print_move) {
			throw std::invalid_argument("Code should not have reached this point.");
		} else if constexpr (capture) {
			// Capture moves.
			const Piece captured = pos.board.get_piece<!white>(to);
			capture_move_wrapper<white, p>(from, to, pos, captured);
			n += count_moves<!white, depth_to_go - 1, false>(pos);
			unmake_capture_wrapper<white, p>(from, to, pos, captured);
		} else {
			// Plain, non special moves.
			plain_move<white, p>(from, to, pos);

			n += count_moves<!white, depth_to_go - 1, false>(pos);
			unmake_plain_move<white, p>(from, to, pos);
		}
		if constexpr (print_move)
			std::cout << make_chess_notation(from) << make_chess_notation(to) << ": " << std::to_string(n) << '\n';
		ret += n;
	}
	return ret;
}

template <bool white, int depth_to_go, bool print_move>
static inline uint64_t generate_pawn_moves(BitBoard cmt, BitBoard pieces, Position& pos) {
	uint64_t ret = 0;
	BitBoard blockers = pos.board.occ_board;
	BitBoard cmt_free = cmt & ~blockers;
	BitBoard cmt_captures = cmt & blockers;
	BitBoard pawns_on_start = pieces & (white ? pawn_start_w : pawn_start_b);
	BitBoard pawns_on_promo = pieces & (white ? promotion_from_w : promotion_from_b);
	ret += generate_pawn_double<white, depth_to_go, print_move>(cmt, pos, pawns_on_start);
	// ret += generate_promotions<white, depth_to_go, print_move>(pos, cmt, pawns_on_promo);
	pieces &= ~pawns_on_promo;

	// Generate moves in bulk.
	if constexpr (depth_to_go <= 1 && !print_move) {
		ret += __builtin_popcountll(piece_move::get_pawn_forward<white>(pieces) & cmt_free);
	}

	// For all remaining pieces.
	while (pieces) {
		Square from = pop(pieces);
		BitBoard captures = piece_move::get_pawn_move<white, PawnMoveType::ATTACKS>(from, blockers) & cmt_captures;
		if constexpr (depth_to_go <= 1 && !print_move) {
			ret += __builtin_popcountll(captures);
		} else {
			BitBoard non_captures = piece_move::get_pawn_move<white, PawnMoveType::FORWARD>(from, blockers) & cmt_free;
			// Non-captures.
			ret += generate_move_or_capture<white, PAWN, false, depth_to_go, print_move>(non_captures, from, pos);
			// Captures.
			ret += generate_move_or_capture<white, PAWN, true, depth_to_go, print_move>(captures, from, pos);
		}
	}
	return ret;
}

// Create moves given a from and to board..
template <bool white, Piece p, int depth_to_go, bool print_move>
static inline uint64_t generate_moves(BitBoard cmt, BitBoard pieces, Position& pos) {
	uint64_t ret = 0;

	// For all instances of given piece.
	while (pieces) {
		Square from = pop(pieces);
		if constexpr (depth_to_go <= 1 && !print_move) {
			ret += __builtin_popcountll(cmt & make_reach_board<white, p>(from, pos.board));
		} else {
			BitBoard captures;
			BitBoard non_captures;
			BitBoard piece_moves_to = cmt & make_reach_board<white, p>(from, pos.board);
			captures = piece_moves_to & (white ? pos.board.b_board : pos.board.w_board);
			non_captures = piece_moves_to & ~captures;

			// Non-captures.
			ret += generate_move_or_capture<white, p, false, depth_to_go, print_move>(non_captures, from, pos);
			// Captures.
			ret += generate_move_or_capture<white, p, true, depth_to_go, print_move>(captures, from, pos);
		}
	}
	return ret;
}

// For a given piece, generate all possible normal/capture moves.
template <bool white, Piece p, int depth_to_go, bool print_move>
static inline uint64_t generate_any(Position& pos) {
	Board& b = pos.board;
	uint64_t ret = 0;
	BitBoard can_move_from = b.get_piece_board<white, p>();
	// Split pinned pieces from non-pinned pieces.
	BitBoard pinned_dg = can_move_from & pos.msk->pinmask_dg;
	BitBoard pinned_orth = can_move_from & pos.msk->pinmask_orth;
	BitBoard unpinned = can_move_from & ~(pinned_dg | pinned_orth);

	if constexpr (p == PAWN) {
		ret += generate_pawn_moves<white, depth_to_go, print_move>(pos.msk->can_move_to, unpinned, pos);
		ret += generate_pawn_moves<white, depth_to_go, print_move>(
			pos.msk->can_move_to & pos.msk->pinmask_dg, pinned_dg, pos
		);
		ret += generate_pawn_moves<white, depth_to_go, print_move>(
			pos.msk->can_move_to & pos.msk->pinmask_orth, pinned_orth, pos
		);
	} else {
		// Unpinned.
		ret += generate_moves<white, p, depth_to_go, print_move>(pos.msk->can_move_to, unpinned, pos);
		// Pinned diagonally.
		ret += generate_moves<white, p, depth_to_go, print_move>(
			pos.msk->can_move_to & pos.msk->pinmask_dg, pinned_dg, pos
		);
		// Pinned orthogonally.
		ret += generate_moves<white, p, depth_to_go, print_move>(
			pos.msk->can_move_to & pos.msk->pinmask_orth, pinned_orth, pos
		);
	}
	return ret;
}

// Create move list for given position.
template <bool white, int depth_to_go, bool print_move>
uint64_t count_moves(Position& pos) {
	if constexpr (depth_to_go < 1) return 1;
	uint64_t ret = 0;
	// Make king mask.
	Square king_square = __builtin_clzll(pos.board.get_piece_board<white, KING>());
	pos.msk = new MaskSet;
	pos.msk->create_masks<white>(pos.board, king_square);
	ret += generate_king_moves<white, depth_to_go, print_move>(pos.msk->can_move_to, king_square, pos);
	// Conditionals only taken when king is in check. If double check, only king can move. Else, limit the
	// target squares to the checkmask and skip castling moves..
	if (pos.msk->checkers >= 2)
		return ret;
	else if (pos.msk->checkers) {
		pos.msk->can_move_to &= pos.msk->check_mask;
		goto no_castle;
	}
	// Castling moves.
	// ret += generate_castle_move<white, true, depth_to_go, print_move>(pos, king_square);
	// ret += generate_castle_move<white, false, depth_to_go, print_move>occ_board(pos, king_square);
no_castle:
	// Generate moves.all         Board::is_equal(Board const&
	ret += generate_any<white, QUEEN, depth_to_go, print_move>(pos);
	ret += generate_any<white, ROOK, depth_to_go, print_move>(pos);
	ret += generate_any<white, BISHOP, depth_to_go, print_move>(pos);
	ret += generate_any<white, KNIGHT, depth_to_go, print_move>(pos);
	ret += generate_any<white, PAWN, depth_to_go, print_move>(pos);
	ret += generate_ep_moves<white, depth_to_go, print_move>(pos, king_square);
	return ret;
}

};	// namespace pyke

#endif
