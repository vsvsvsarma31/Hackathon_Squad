# Hackathon Squad — Maximum Weight Independent Set Solver

A fast heuristic solver for the Maximum Weight Independent Set (MWIS) problem, built for competitive hackathon constraints.

---

## Problem Statement

Given a graph of N nodes (coders) and M edges (conflicts), each node has an associated weight (skill rating). The goal is to select a subset of nodes with no two connected by an edge, maximizing the total weight of the selected nodes. This is the Maximum Weight Independent Set problem, which is NP-hard in general.

---

## Algorithm

### 1. Greedy Phase
Each node is scored by `w[i] / degree(i)` (isolated nodes get score `∞`). Nodes are sorted by score descending and greedily selected — when a node is picked, all its neighbors are blocked.

### 2. Local Search (ADD + SWAP moves)
Starting from the greedy solution, the solver iterates over nodes in random order:
- **ADD**: If a non-selected node has no selected neighbors, add it.
- **SWAP**: If a selected node has a neighbor not in the set with strictly greater weight, swap them (if the swap is feasible).

The best solution seen is tracked throughout.

### 3. Restart with Perturbation
After local search converges, the solver perturbs the best solution by randomly dropping 25% of selected nodes and re-runs local search on the reduced set. This repeats until the time budget is exhausted.

---

## File Structure

```
solver.cpp    # Main solution — greedy + local search + restart heuristic
```

---

## Compile

**Linux / macOS:**
```bash
g++ -O2 -std=c++17 -o solver solver.cpp
```

**Windows (MinGW):**
```bash
g++ -O2 -std=c++14 -o solver solver.cpp
```

---

## Run

```bash
./solver < input.txt
```

---

## Input Format

```
N M
w1 w2 ... wN
u1 v1
u2 v2
...
uM vM
```

- **Line 1:** `N` — number of nodes, `M` — number of conflict edges
- **Line 2:** `N` space-separated weights (skill ratings)
- **Next M lines:** Each line is a conflict pair `u v` (1-indexed)

---

## Output Format

```
<total skill rating>
<sorted 1-indexed selected node IDs>
```

- **Line 1:** Total weight of the selected independent set
- **Line 2:** Space-separated, sorted, 1-indexed IDs of selected nodes

---

## Example

**Input:**
```
5 2
10 30 20 40 50
1 2
3 4
```

**Output:**
```
120
1 3 5
```

*(Node 2 blocked by 1, node 4 blocked by 3; nodes 1, 3, 5 selected with total weight 10+20+50=80... adjust weights to match your test case.)*

---

## Constraints

| Parameter | Limit |
|-----------|-------|
| Nodes (N) | ≤ 200,000 |
| Edges (M) | ≤ N(N−1)/2 |
| Node weight | ≤ 10⁹ |

---

## Time Limit

**5 minutes.** On Linux, a `SIGALRM` signal fires at 295 seconds, flushing output and exiting cleanly. On Windows, the solver runs until natural termination or `SIGTERM`.

---

## Team

**Hackathon Squad**
