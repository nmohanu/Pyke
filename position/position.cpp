#include "position.hpp"

#include <cstdint>

// Position constructor.
Position::Position() {}
// Position destructor.
Position::~Position() {}

// Create attack board for specific piece.
uint64_t Position::make_reach_board(Square square, bool is_black, Piece piece_type) {
	return (this->*move_functions[piece_type - 6 * is_black])(square, is_black);
}

// Do a move.
void Position::do_move(Move move) {
	// Decide what to do.
	uint8_t move_type = get_move_type(move);

	// Push current flag to the stack.
	flag_history.push(pos_flag);

	// Call move function based on move type.
	(this->*do_move_functions[move_type])(move);

	// Toggle player sign.
	player_sign = !player_sign;
}

// Function for plain move type.
void Position::plain_move(Move move) { move_piece(get_move_from(move), get_move_to(move), get_move_piece(move)); }

// Move piece from a to b.
void Position::move_piece(Square from, Square to, Piece piece) {
	// Create board masks.
	uint64_t start_square_mask = ~(square_to_mask(from));
	uint64_t end_square_mask = 1ULL << (square_to_mask(to));

	// Update piece board.
	bit_boards[piece] = (bit_boards[piece] & ~start_square_mask) | end_square_mask;
}

// Function for capturing moves.
void Position::capture_move(Move move) {
	Piece captured_piece = get_move_content(move);
	uint64_t end_square_mask = square_to_mask(get_move_to(move));
	// Move the moving piece.
	move_piece(get_move_from(move), get_move_to(move), get_move_piece(move));

	// Update board for captured piece.
	bit_boards[captured_piece] &= ~end_square_mask;
}

// Function for promotion moves.
void Position::promotion_move(Move move) {
	Square to = get_move_to(move);
	Piece piece = get_move_piece(move);
	Piece new_piece = get_move_content(move);
	uint64_t pos_mask = square_to_mask(to);

	// Move the pawn.
	move_piece(get_move_from(move), to, piece);

	// Update boards.
	bit_boards[piece] &= ~pos_mask;
	bit_boards[new_piece] |= pos_mask;
}

// Get the piece on given square.
uint8_t Position::get_piece(Square square) const {
	uint64_t bit_mask = 1ULL << (63 - square);
	uint8_t piece = 0;
	for (int i = 0; i < 12; i++) piece += !(bit_boards[i] & bit_mask) && (i == piece);
	return piece;
}