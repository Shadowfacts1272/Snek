#include <iostream>

enum move_data {MAX_MOVES = 256};

enum move_flags {
							none, 
							k_castling, 
							q_castling, 
							enpassant, 
							doublepush,
							n_promote,
							b_promote,
							r_promote,
							q_promote
};

//Regarding conversions between flags and promotion
int promotion_flags[9] = {0, 0, 0, 0, 0, 2, 4, 6, 8};

//FFFFCCCCPPPPTTTTTTSSSSSS
struct MOVE {
	uint64_t move; //Maybe unsigned 64 bit int 

	inline void set_score(int val){
		//clear the score 
		move &= 0xffffff;
		//add new score
		move |= val << 24;
	}

	inline int source()   {return move & 0x3f;}
	inline int target()   {return (move >> 6) & 0x3f;}
	inline int piece()    {return (move >> 12) & 0xf;}
	inline int capture()  {return (move >> 16) & 0xf;}
	inline int flag()		  {return (move >> 20) & 0xf;}
	inline int score()    {return (move >> 24) & 0xff;}
};

struct movelist {
	MOVE moves[MAX_MOVES] = {};
	int index = 0;
	void add_move(int move){
		moves[index++].move = move;
	}
	int size(){
		return index;
	}
};
