<h1>Sidewinder</h1>


<p>A work in progress chess engine written by StackFish5</p>

<H2>Some Credits Before The Other Stuff Begins (Because This Is Really Important</H2>
<br>

- Thanks to [Sazgr](https://replit.com/@Sazgr)
 for the history table implementation used in Sidewinder.

<h2>Already Implemented</h2>
<br>

- Black Magic Bitboards with redundant mailbox representation for make/unmake move
- Simple evaluation with PESTOS piece square tables and material weights
- Tapered Eval
- PVS (Principal Variation Search)
	- LMR
- Alpha-beta search
- Quiescence search
- Late Move Pruning
- Move ordering based on killer moves, history moves, psqt table and MVV-LVA
- Iterative Deepening
<br>

<h2>To Do List (Not in That Particular Order):</h2>
<br>

- Knight Outposts
- Rook on open file
- King safety
- Pawn hash table
- More move ordering
- Transposition table
- LMR
- Futility Pruning
- Delta Pruning
- Aspiration Windows
- Full UCI Compliance
- Get to at least 2500 rating in CCRL
- Fix up pv-node collection
- Speed up PERFT even more
- Null Move Pruning

<br>