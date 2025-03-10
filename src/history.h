#include <iostream>

typedef struct history {
    int table[12][64] = {};
    int sum;
    void edit(int piece, int to_square, int change) {
      table[piece][to_square] += change;
      sum += change;
      if (sum >= 0xFFFFFF || sum <= -0xFFFFFF) {
        age();
      }
    }
    void age() {
      for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 64; ++j) {
            table[i][j] = (table[i][j] >> 1) + (table[i][j] >> 2);
	        }
      }
      sum = (sum >> 1) + (sum >> 2);
    }
    void reset() {
      for (int i = 0; i < 12; ++i) {
        for (int j = 0; j < 64; ++j) {
          table[i][j] = 0;
        }
	    }
      sum = 0;
    }
} HISTORY;

