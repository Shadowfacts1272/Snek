#include <bitset>
#include <fstream>
#include <stdio.h>
#include <utility>
#include "uci.h"
#include "timer.h"
#include "hash.h"
#include "history.h"
#include "see.h"

//g++ -pthread -std=c++17 -Ofast -DNDEBUG -m64 -mpopcnt -flto -static -o C:\Users\kyley\OneDrive\Documents\chess\engines\snek-dev.exe C:\Users\kyley\OneDrive\Documents\chess\snek\*.cpp

const int MAX_PLY = 64;

//Perft function
inline uint64_t Perft(Legal_Moves &board, int depth){
	if (depth == 1) {
		movelist move_list;
    board.legal_moves(move_list);
    return move_list.size();
  } else {
    uint64_t total = 0;
		movelist move_list;
    board.legal_moves(move_list);
    for(int i = 0; i < move_list.size(); i++) {
      board.push_move(move_list.moves[i]);
      total += Perft(board, depth - 1);
      board.pop_move(move_list.moves[i]);
    }
    return total;
  }
}

void Perft_Split(Legal_Moves &board, int depth){
	movelist move_list;
	uint64_t nodes = 0;
	uint64_t t_nodes = 0;

	board.legal_moves(move_list);

	MOVE move;
	std::cout << "\nGo PERFT: " << depth << "\n";
	for(int i = 0; i < move_list.size(); i++){
		nodes = 0;
		move = move_list.moves[i];
		board.push_move(move_list.moves[i]);
		std::cout << coordinates[move.source()];
		std::cout << coordinates[move.target()];
		std::cout << promoted_pieces[promotion_flags[move.flag()]];
		if(depth == 1){
			nodes ++;
		} else {
			nodes += Perft(board, depth - 1);
		}
		t_nodes += nodes;
		board.pop_move(move);
		std::cout << ":		" << nodes << "\n";
	}
	std::cout << "Total nodes:	" <<t_nodes << "\n";
}

typedef struct killer {
	MOVE table_1 [MAX_PLY];
	MOVE table_2 [MAX_PLY];

	void append (MOVE beta_cutoff, int ply){
		table_2[ply] = table_1[ply]; 
		table_1[ply] = beta_cutoff; 
	}

	void clear() {
    for (int i = 0; i < MAX_PLY; i++) {
      table_1 [i].move = 0;
			table_2 [i].move = 0;
    }
  }
} KILLER;

//A principal variation table and associated stuff
int pv_length [MAX_PLY];
MOVE pv_table [MAX_PLY] [MAX_PLY];

//Search class
class Search {
	public:
  //Using piece square table to help order moves
	int order_psqt [12] [64];

	//In which first field is victim and second field is the attacker 
	//Up to 255
	int mvv_lva [12] [12] = {
		{15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10},
		{15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10},
		{25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20},
		{25, 25, 24, 24, 23, 23, 22, 22, 21, 21, 20, 20},
		{35, 35, 34, 34, 33, 33, 32, 32, 31, 31, 30, 30},
		{35, 35, 34, 34, 33, 33, 32, 32, 31, 31, 30, 30},
		{45, 45, 44, 44, 43, 43, 42, 42, 41, 41, 40, 40},
		{45, 45, 44, 44, 43, 43, 42, 42, 41, 41, 40, 40},
		{55, 55, 54, 54, 53, 53, 52, 52, 51, 51, 50, 50},
		{55, 55, 54, 54, 53, 53, 52, 52, 51, 51, 50, 50},
		{0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 },
		{0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 }
	};

	//Determine move similarity using move properties
	bool compare_move (MOVE a_move, MOVE b_move){
		return ((a_move.source() == b_move.source()) && (a_move.target() == b_move.target()) && (a_move.piece() == b_move.piece()));
	}

	//Static Exchange Evaluation piece values
	const int see_val [13] = {100, 100, 300, 300, 300, 300, 500, 500, 900, 900, 9999, 9999, 0};

	//Attain the least valuable piece
	uint64_t least_valuable_piece(uint64_t attacks, int by_side, int &piece, Legal_Moves &board)
	{
	  for (piece = by_side; piece <= K + by_side; piece += 2) {
	    uint64_t subset = attacks & board.bitboards[piece];
	    if(subset){
	      return subset & -subset; // single bit
			}
	  }
	  return 0; // empty set
	}

	//The main SEE routine
	int see(MOVE move, Legal_Moves &board){
	  int gain[32], d = 0;
		//Move properties
		int source = move.source(), target_sq = move.target(), tar_piece = board.piece_list[target_sq], piece = move.piece();
	
		//Pieces that can be used for potential xray attacks
		bool stm = board.side_to_move;
		uint64_t hv_xray = (board.bitboards[R] | board.bitboards[r] | board.bitboards[Q] | board.bitboards[q]) & board.rook_attacks(target_sq, 0);
		uint64_t da_xray = (board.bitboards[B] | board.bitboards[b] | board.bitboards[Q] | board.bitboards[q]) & board.bishop_attacks(target_sq, 0);
	
		//Source square bitboard
	  uint64_t from_set = 1ULL << source;
	  uint64_t occ = board.occupancies[BOTH];
	  uint64_t attacks = board.square_attackers(target_sq, 0) | board.square_attackers(target_sq, 1);
		//Get initial target value
	  gain[d] = see_val[tar_piece];
	  do {
			//next depth and side
	    d++; 
			stm ^= 1;
			//speculative store, if defended
	    gain[d]  = see_val[piece] - gain[d-1]; 
			//pruning does not influence the result
	    if(std::max(-gain[d-1], gain[d]) < 0) break; 
			//reset bit in set to traverse
	    attacks ^= from_set;
			//reset bit in temporary occupancy for xrays
	    occ &= ~from_set; 
	    if(from_set & (hv_xray | board.bitboards[P] | board.bitboards[p])){
				//If rook related pins
				attacks |= board.xray_rook_attacks(occ, occ, target_sq) & hv_xray;
			} 
			if (from_set & (da_xray | board.bitboards[P] | board.bitboards[p])){
				//If bishop related pins
				attacks |= board.xray_bishop_attacks(occ, occ, target_sq) & da_xray;
			}
	    from_set = least_valuable_piece(attacks, stm, piece, board);
	  } while(from_set);
		while (--d){
		  gain[d-1]= -std::max(-gain[d-1], gain[d]);
		}
		return gain[0];
	}

	inline void get_best_move(movelist &move_list, int start_index){
		for(int i = start_index + 1; i <= move_list.size(); i++){
			if(move_list.moves[i].score() > move_list.moves[start_index].score()){
				std::swap(move_list.moves[i], move_list.moves[start_index]);
			}
		}
	}

	//Initialize some things, quite explanatory
	void move_order_psqt_init(){
	  for(int pc = 0; pc < 6; ++pc){
	    for(int sq = 0; sq < 64; ++sq){
	      order_psqt[pc * 2][sq] = mg_table[pc][sq ^ 56]/15;
	    }
	    for(int sq = 0; sq < 64; ++sq){
	      order_psqt[pc * 2 + 1][sq] = mg_table[pc][sq]/15;
	    }
	  }
	}

	//Some useful move ordering constant
	const int remove_side = ~1;

	void score_moves(movelist &move_list, int ply, KILLER &killer, HISTORY history, Legal_Moves &board){
		//Create some variables for usage in loop
		int source;
		int target;
		int piece;
		int capture;
		int score;
		MOVE move;

		//Let the loop begin
		for(int n; n < move_list.size(); n++){
			move = move_list.moves[n];
			source = move.source();
			target = move.target();
			piece = move.piece();
			capture = move.capture();
			score = 0;

			if(capture == E){
				//If it isn't a capture
				score = history.table[piece] [target];
				if(move.move == killer.table_1[ply].move){
					score += 180;
					break;
				} else if (move.move == killer.table_2[ply].move){
					score += 90;
					break;
				}
			} else {
				if((target & remove_side) < (piece & remove_side)){
					//Do MVV-LVA
					score = mvv_lva [capture] [piece];
				} else {
					//Otherwise, do SEE
					score = see(move, board);
				}
			}
			move_list.moves[n].set_score(score + order_psqt[piece] [target] - order_psqt[piece] [source]);
		}
	}

	//Seperate score moves function for q_search
	void score_moves_q(movelist &move_list, Legal_Moves &board){
		//Create some variables for usage in loop
		int source;
		int target;
		int piece;
		int capture;
		int score;
		MOVE move;

		//Let the loop begin
		for(int n; n < move_list.size(); n++){
			move = move_list.moves[n];
			source = move.source();
			target = move.target();
			piece = move.piece();
			capture = move.capture();
			score = 0;
			
			if((target & remove_side) < (piece & remove_side)){
				//Do MVV-LVA
				score = mvv_lva [capture] [piece];
			} else {
				//Otherwise, do SEE
				score = see(move, board);
			}		
			move_list.moves[n].set_score(score + order_psqt[piece] [target] - order_psqt[piece] [source]);
		}
	}

	//Quiescence
	int queiscence(int alpha, int beta, Legal_Moves& board) {
		int stand_pat = board.static_evaluation();
		if(stand_pat >= beta){
			return beta;
		}
		if(alpha < stand_pat){
			alpha = stand_pat;
		}
		int score;
		movelist move_list;
		board.legal_moves(move_list);
		for(int n = 0; n < move_list.size(); n++){
			board.push_move(move_list.moves[n]);
			score = queiscence(-beta, -alpha, board);
			board.pop_move(move_list.moves[n]);
			if(score >= beta){
				return beta;
			}
			if(score > alpha){
				alpha = score;
			}
		}
		return alpha;
	}

	int alphabeta(int alpha, int beta, int depth, int ply, Legal_Moves& board){
		int score;
		//PV Table
		pv_length[ply] = ply;
		//queiscence
		if(depth == 0){
			return queiscence(alpha, beta, board);
		} 
	
		movelist move_list;
		board.legal_moves(move_list);
		for(int n = 0; n < move_list.size(); n++){
			board.push_move(move_list.moves[n]);
			score = -alphabeta(-beta, -alpha, depth - 1, ply + 1, board);
			board.pop_move(move_list.moves[n]);
			
			if(score >= beta){
				return beta;
			}
			
			if(score > alpha){
				score = alpha;
				pv_table[ply][ply] = move_list.moves[n];
				for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++){
				pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];
				}
				pv_length[ply] = pv_length[ply + 1];
			}
		}
		return alpha;
	}
};

void see_testing(Search &search, Legal_Moves &board){
	int counter = 0;
	int score;
	int correct = 0;
  while(counter < 74){ 
		//Do SEE stuff
    board.Initialize_Everything(test_pos[counter].pos);
		score = search.see(get_move_from_input(test_pos[counter].move, board), board);
		if(score == test_pos[counter].score){
			correct++;
		}
		counter++;
  }
	std::cout<<correct<<" out of 74 positions are correct";
}

int double_pushed [64] [64];

int main() {
	//1r3r2/5p2/4p2p/2k1n1P1/2PN1nP1/1P3P2/8/2KR1B1R b - - 0 1
	
	//Initialize everything
	Legal_Moves board;
	Timer time;
	Search search;
	parse_position(board);
	tables_init();
	init_eval(board.piece_list);
	search.move_order_psqt_init();
	HISTORY history;
	KILLER killer;

	int high = 0;
	int low = 0;
	std::vector <int> psqt_scores;
	for(int i = 0; i < 6; i++){
		for(int j = 0; j < 64; j++){
			for(int k = 0; k < 64; k++){
				psqt_scores.push_back(mg_table[i][j] - mg_table[i][k]);
				if(mg_table[i][j] - mg_table[i][k] > high){
					high = mg_table[i][j] - mg_table[i][k];
				} else if (mg_table[i][j] - mg_table[i][k] < low){
					low = mg_table[i][j] - mg_table[i][k];
				}
			}
		}
	}

	//Begin!
	board.print();
	movelist move_list;
	board.legal_moves(move_list);
	search.score_moves(move_list, 1, killer, history, board);
	board.print_move_scores(move_list);
	
	std::cout<<search.alphabeta(-15000, 15000, 3, 0, board)<<"\n";
	std::cout<<(search.order_psqt[N] [a3] - search.order_psqt[N] [b1]);
	print_move(pv_table[0][0]);
	board.push_move(pv_table[0][0]);
	board.print();
	for (int count = 0; count < pv_length[0]; count++){
  		std::cout<<print_move(pv_table[0][count]);
  	}
}