# DivineMove
A Computer Go Game program with GTP support.

It uses the Monte Carlo tree search algorithm which is proved to be useful for Computer Go in many papers.
We improved the original algorithm by adding cut-branches in UCT, reuse of existing tree data, and a "filling water" method for fast analyzing of the current game situation.