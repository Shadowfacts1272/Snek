#pragma once
#include <iostream>
#include <cmath>
#include "bit_twiddling.h"
#include "fen.h"
#include "hash.h"
#include "initialize_magic_bitboard.h"
#include "magic_masks.h"
#include "magics.h"
#include "preinitialized_arrays.h"
#include "squares.h"
#include "eval.h"

enum Piece_Types {P, p, N, n, B, b, R, r, Q, q, K, k, E};

enum colors {WHITE, BLACK, BOTH};

class Legal_Moves{
	public:
		//declare bitboard array
		uint64_t bitboards [13];
		//Castling rights
		bool Castle_White_Kingside;
		bool Castle_White_Queenside;
		bool Castle_Black_Kingside;
		bool Castle_Black_Queenside;
		std::vector <int> Castling_Rights;
		//Remembering en passant squares
		std::vector <int> En_Passant_Sq;
		//side to move
		bool side_to_move;
		//Occupancies
		uint64_t occupancies[3];
		//get rook attacks
		uint64_t rook_attacks(int square, uint64_t occupancy);
		uint64_t bishop_attacks(int square, uint64_t occupancy);
		uint64_t queen_attacks(int square, uint64_t occupancy);
		//Rectangular lookup tables ([sq1] [sq2])
		uint64_t Rect_Lookup [64] [64]; 
		//For initializing Rect_Lookup 
		uint64_t in_between(int sq1, int sq2);
		//init slider piece's attack tables
		void init_sliders_attacks(int bishop);
		//update occupancies
		void update_occupancies();
		//Direction relative to square one to square two
		int pin_direction [64] [64];
		//xray rook attacks (for pins)
		uint64_t xray_rook_attacks(uint64_t occ, uint64_t blockers, int rookSq);
		//xray bishop attacks (for pins)
		uint64_t xray_bishop_attacks(uint64_t occ, uint64_t blockers, int bishopSq);
		//get absolute pins
		uint64_t absolute_pins(int side, int squareOfKing);
		//direction between two squares
		int direction(int sq1, int sq2);
		const uint64_t rook_positions [4] = {0x80, 0x8000000000000000, 0x1, 0x100000000000000};
		const uint64_t castle_constants [4] = {0x60, 0xc, 0x6000000000000000, 0xc00000000000000};
		const uint64_t castle_king_constants [2] = {0x10, 0x1000000000000000};
		const uint64_t en_passant_ranks [2] = {0xff00000000, 0xff000000};
		//promotion ranks
		const uint64_t promotion_ranks [2] = {0xff000000000000, 0xff00};
		//Castle masks for castle rights bits
		const int castle_modifiers [4] [2] = {
			{0xe, 0xf}, 
			{0xd, 0xf},
			{0xb, 0xf},
			{0x7, 0xf},
		};
		const uint64_t space_setbacks [2] = {0xe, 0xe00000000000000};
		const int remove_castle [2] = {0xa, 0x5};
		const int eval_multiplier [2] = {1, -1};
		//Pieces that attack a square
		uint64_t square_attackers(int square, int Side);
		//Attack map that doesn't treat king as a blocker so king has correct avoid mask
		uint64_t king_danger_squares(int side);
		//To castle, or not to castle
		bool castle_kingside(int side, uint64_t attacks);
		bool castle_queenside(int side, uint64_t attacks);
		bool legal_en_passant(int side, int source, int target);
		void legal_moves(movelist &move_list);
		void legal_captures(movelist &move_list);
		int count_moves();
		//Piece list for Redundancy (E is fun)
		int piece_list [64] = {
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
			E, E, E, E, E, E, E, E,
		};
		//print board (and other related stuff)
		void print();
		void print_bits(uint64_t bitboards);
		//Return move encoding
		int get_move(int source, int target, int piece, int capture, int flag, int score){
			return (source | (target << 6) | (piece << 12) | (capture << 16) | (flag << 20) | (score << 24));
		}
		//Make and unmake move
		void push_move(MOVE move);
		void pop_move(MOVE move);
		void Initialize_Everything(std::string input);
		void print_move_list(movelist &move_list);
		void print_move_scores(movelist &move_list);
		void remove_sq(int sq);
		void set_sq(int sq, int piece);
		void initialize_eval();
		//Score static position
		int static_evaluation();
		//Zobrist hashing and stuff
		uint64_t rand64() {
			static uint64_t next = 1;
			next = next * 1103515245 + 12345;
			return next;
		}
		uint64_t hash = 0;
		uint64_t piece_keys [13] [64] = {};
		uint64_t castle_keys [16] = {};
		uint64_t en_passant_keys [65] = {};
		uint64_t turn_key;
		void init_hash(int board[64], std::vector <int> en_passant, std::vector <int> castling, bool side_to_move);
	
		//This assumes check condition has been met beforehand
		void push_null_move();
		void pop_null_move();
	
		bool in_check(){
			return square_attackers(get_ls1b(bitboards[K + side_to_move]), !side_to_move);
		}
		void print_info(){
			print();
			std::cout<<"Side to Move: "<<(side_to_move ? "BLACK" : "WHITE")<<"\n";
			std::cout<<"Castling: ";
			//Kingside White
			if (Castling_Rights.back() & 1) {
    			std::cout<<"K";
			}
				//Queenside White
			if (Castling_Rights.back() & 4) {
				std::cout<<"Q";
			}
				//Kingside Black
			if (Castling_Rights.back() & 2) {
				std::cout<<"k";
			}
				//Queenside Black
			if (Castling_Rights.back() & 8) {
				std::cout<<"q";
			}
			std::cout<<std::endl;
			std::cout<<"En Passant Square: "<<coordinates[En_Passant_Sq.back()]<<"\n";
			//std::cout<<"0x"<<std::hex<<hash<<"\n";
		}
		//Change side to move
		void flip(){
			side_to_move ^= 1;
		}
	};