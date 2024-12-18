#include "board.hpp"
#include "gamestate.hpp"
#include "maskset.hpp"
#include "move.hpp"
#include "stack.hpp"

#ifndef POSITION_H
#define POSITION_H

struct Position {
	Position() {}
	Position(Position& other) {
		gamestate.set_data(other.gamestate.get_data());
		white_turn = other.white_turn;
		board = other.board.copy();
	}
	Board board;
	GameState gamestate;
	Stack<GameState> history;
	Stack<Move> movelist;
	Stack<MaskSet*> masks;
	bool white_turn = true;
	MaskSet* msk;

	bool is_equal(Position& other);

	inline void moved() {
		white_turn = !white_turn;
		history.push(gamestate);
		masks.push(msk);
		gamestate.reset_en_passant();
	}
	inline void unmoved() {
		white_turn = !white_turn;
		gamestate = history.pop();
		msk = masks.pop();
	}

	void print_position();
};

#endif
