
#include <cstdint>

#include "defaults.hpp"
#include "position.hpp"

#ifndef MAKE_MOVE_H
#define MAKE_MOVE_H

// Add piece to board.
template <bool white, Piece p>
static void add_to_board(Board& b, Square s) {
	BitBoard mask = square_to_mask(s);
	*b.get_board_pointer<white, p>() |= mask;
	b.occ_board |= mask;
	if constexpr (white)
		b.w_board |= mask;
	else
		b.b_board |= mask;
}

// Remove piece from board.
template <bool white, Piece p>
static void remove_from_board(Board& b, Square s) {
	BitBoard mask = ~square_to_mask(s);
	*b.get_board_pointer<white, p>() &= mask;
	b.occ_board &= mask;
	if constexpr (white)
		b.w_board &= mask;
	else
		b.b_board &= mask;
}

// Move a piece.
template <bool white, Piece p>
static void move_piece(Square from, Square to, Board& b) {
	remove_from_board<white, p>(b, from);
	add_to_board<white, p>(b, to);
}

// Undo a piece move.
template <bool white, Piece p>
static void unmake_move_piece(Square from, Square to, Board& b) {
	remove_from_board<white>(b, to, p);
	add_to_board<white>(b, from, p);
}

// Do a plain, non capturing move.
template <bool white, Piece p>
static void plain_move(Square from, Square to, Position& pos) {
	pos.moved();
	move_piece<white, p>(from, to, pos.board);
}

// Undo a non capturing move.
template <bool white, Piece p>
static void unmake_plain_move(Square from, Square to, Position& pos) {
	pos.unmoved();
	unmake_move_piece<white, p>(from, to, pos.board);
}

// Castle and update castling rights.
template <bool white, uint8_t code>
static void castle_move(Position& pos) {
	pos.gamestate.rm_cr<white>();
	pos.moved();
	move_piece<white>(king_start_squares[code], king_end_squares[code], KING, pos.board);
	move_piece<white>(rook_start_squares[code], rook_end_squares[code], ROOK, pos.board);
}

// Undo castling move.
template <bool white, uint8_t code>
static void unmake_castle_move(Position& pos) {
	pos.unmoved();
	unmake_move_piece<white>(king_start_squares[code], king_end_squares[code], KING, pos.unmoved());
	unmake_move_piece<white>(rook_start_squares[code], rook_end_squares[code], ROOK, pos.unmoved());
}

// Do capture move.
template <bool white, Piece p, Piece captured>
static void capture_move(Square from, Square to, Position& pos) {
	pos.moved();
	remove_from_board<!white, captured>(pos.board, to);
	move_piece<white, p>(from, to, pos.board);
}

// Undo capture move.
template <bool white, Piece p, Piece captured>
static void unmake_capture_move(Square from, Square to, Position& pos) {
	pos.unmoved();
	unmake_move_piece<white, p>(from, to, pos.board);
	add_to_board<!white, captured>(pos.board, to);
}

// Do ep move.
template <bool white>
static void ep_move(Square from, Square to, Position& pos) {
	pos.moved();
	Square captured_sq = white ? (to - 8) : (to + 8);
	move_piece<white, PAWN>(from, to, pos.board);
	remove_from_board<!white, PAWN>(pos.board, captured_sq);
}

// Undo ep move.
template <bool white>
static void unmake_ep_move(Square from, Square to, Position& pos) {
	pos.unmoved();
	Square captured_sq = white ? (to - 8) : (to + 8);
	unmake_move_piece<white, PAWN>(from, to, PAWN, pos.board);
	add_to_board<white, PAWN>(pos.board, captured_sq);
}

// Do promo move.
template <bool white, Piece p, Piece captured>
static void promo_move(Square from, Square to, Position& pos) {
	pos.moved();
	// Check if promotion was also a capture.
	if constexpr (captured != EMPTY) remove_from_board<white, captured>(pos.board, to);
	remove_from_board<white, PAWN>(pos.board, from);
	add_to_board<white, p>(pos.board, to);
}

// Undo promo move.
template <bool white, Piece p, Piece captured>
static void unmake_promo_move(Square from, Square to, Position& pos) {
	pos.unmoved();
	add_to_board<white, PAWN>(pos.board, from, PAWN);
	remove_from_board<white, p>(pos.board, to);
	// Check if promotion was also a capture.
	if constexpr (captured != EMPTY) add_to_board<white, captured>(pos.board, to);
}

// Do pawn double forward move.
template <bool white>
static void pawn_double(Square from, Square to, Position& pos) {
	pos.moved();
	move_piece<white, PAWN>(from, to, pos.board);

	// pawn moved two forward, update en passant status.
	bool left_is_pawn = pos.board.get_piece<white>(to - 1) == PAWN;
	bool right_is_pawn = pos.board.get_piece<white>(to + 1) == PAWN;
	bool left_piece_sign = square_to_mask(to - 1) & pos.board.w_board;
	bool right_piece_sign = square_to_mask(to + 1) & pos.board.w_board;
	bool left_is_opp_pawn = left_is_pawn && (left_piece_sign != white);
	bool right_is_opp_pawn = right_is_pawn && (right_piece_sign != white);

	// Edge of board cases.
	bool not_edge_left = to % 8 != 0;
	bool not_edge_right = to % 8 != 7;

	// Check if en passant is possible.
	bool possible_left = left_is_opp_pawn && not_edge_left;
	bool possible_right = right_is_opp_pawn && not_edge_right;

	// Update flag.
	uint8_t file = to % 8;
	if (possible_left) pos.gamestate.set_en_passant(possible_left, file);
	if (possible_right) pos.gamestate.set_en_passant(possible_right, file);
}

// Undo double pawn move.
template <bool white>
static void unmake_pawn_double(Square from, Square to, Position& pos) {
	pos.unmoved();
	unmake_move_piece<white, PAWN>(from, to, pos.board);
}

template <bool white, Piece p>
static void capture_move_wrapper(Square from, Square to, Position& pos, Piece captured) {
	switch (captured) {
	case PAWN:
		capture_move<white, p, PAWN>(from, to, pos);
		break;
	case KNIGHT:
		capture_move<white, p, KNIGHT>(from, to, pos);
		break;
	case BISHOP:
		capture_move<white, p, BISHOP>(from, to, pos);
		break;
	case ROOK:
		capture_move<white, p, ROOK>(from, to, pos);
		break;
	case QUEEN:
		capture_move<white, p, QUEEN>(from, to, pos);
		break;
	}
}

template <bool white, Piece p>
static void unmake_capture_wrapper(Square from, Square to, Position& pos, Piece captured) {
	switch (captured) {
	case PAWN:
		unmake_capture_move<white, p, PAWN>(from, to, pos);
		break;
	case KNIGHT:
		unmake_capture_move<white, p, KNIGHT>(from, to, pos);
		break;
	case BISHOP:
		unmake_capture_move<white, p, BISHOP>(from, to, pos);
		break;
	case ROOK:
		unmake_capture_move<white, p, ROOK>(from, to, pos);
		break;
	case QUEEN:
		unmake_capture_move<white, p, QUEEN>(from, to, pos);
		break;
	}
}

#endif
