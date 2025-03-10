# Computational Verification of the Combinatorial Hypothesis

Given a multiset of natural numbers A, we denote by ∑A the sum over all its elements, i.e.,  
∑A = ∑(a ∈ A) a. For example, if A = {1, 2, 2, 2, 10, 10}, then ∑A = 27. For two multisets we write A ⊇ B if every element appears in A at least as many times as it appears in B.

For the purposes of this problem, let us adopt the following definitions.

## Definitions

**Definition.** A multiset A is called *d-bounded* for a natural number d if it is finite and all its elements belong to the set {1, …, d} (with any repetitions allowed).

**Definition.** A pair of d-bounded multisets A, B is called *uncontentious* (bezsporne) if for all subsets A′ ⊆ A and B′ ⊆ B the following holds:
  
  ∑A′ = ∑B′ ⟺ (A′ = B′ = ∅) ∨ (A′ = A ∧ B′ = B).

In other words, ∑A = ∑B, but apart from that the sums of any non-empty subsets of A and B must differ.

## Problem

For a fixed d ≥ 3 (we will not consider values of d less than 3) and given multisets A₀, B₀, we want to find uncontentious d-bounded multisets A ⊇ A₀ and B ⊇ B₀ that maximize the value ∑A (equivalently, ∑B). We denote this value by α(d, A₀, B₀). Let us assume that α(d, A₀, B₀) = 0 if A₀ and B₀ are not d-bounded or if they do not have d-bounded uncontentious supersets.

### Example

α(d, ∅, ∅) ≥ d(d − 1).

*Sketch of proof:*  
The multisets  
- A = {d, …, d (d−1 times)}  
- B = {d−1, …, d−1 (d times)}  
satisfy the conditions with ∑A = d(d − 1) = ∑B.

### Another Example

α(d, ∅, {1}) ≥ (d − 1)².

*Sketch of proof:*  
The multisets  
- A = {1, d, …, d (d−2 times)}  
- B = {d−1, …, d−1 (d−1 times)}  
satisfy the conditions with ∑A = 1 + d(d − 2) = (d − 1)² = ∑B.

It can be proven that the above examples are optimal, i.e.,  
α(d, ∅, ∅) = d(d − 1) and  
α(d, ∅, {1}) = (d − 1)².

Nevertheless, in this problem we want to verify this computationally, for as large values of d as possible, as well as to compute α for other prescribed multisets A₀, B₀.

## Recurrence with Backtracking

We can compute the values α(d, A₀, B₀) recursively by incrementally building multisets A ⊇ A₀ and B ⊇ B₀. Denote by AΣ = {∑A′ : A′ ⊆ A} the set of all possible subset sums that can be obtained from multiset A (note: this is a set, so we do not care about the number of ways a particular sum can be generated from the elements of the multiset). We use the following recurrence:

```
Solve(d, A, B):
    if ∑A > ∑B then swap(A, B)
    S ← AΣ ∩ BΣ
    if ∑A = ∑B then
        if S = {0, ∑A} then return ∑A
        else return 0
    else if S = {0} then
        return max(x ∈ {lastA, …, d} ∖ BΣ) Solve(d, A ∪ {x}, B)
    else return 0
```

where `lastA` denotes the element last added to A (in the case A = A₀ we take `lastA = 1`, i.e., the recursion adds elements to A in non-decreasing order).

In practice, to avoid computing the sum-sets AΣ and BΣ from scratch at each step, we pass along AΣ and BΣ in the recursion. When adding an element x to A, the new AΣ becomes AΣ ∪ (AΣ + x), where AΣ + x is the set obtained by adding x to each element of AΣ. The sum sets AΣ and BΣ are represented efficiently using so-called bitsets.

In the `reference` folder of the attached archive, you will find a detailed reference implementation: a relatively optimized, but sequential, recursive solution.

## Task

The task is to write two alternative implementations of the same computation:
1. A non-recursive (but still single-threaded) implementation.
2. A parallel (multi-threaded) implementation that achieves the best possible scalability.

Additionally, you must include a report presenting the scalability of the parallel version on selected tests with different numbers of threads.

The goal of the task is not a theoretical analysis of the mathematical problem nor micro-optimizations of bit-level operations; therefore, the implementations of operations on multisets are provided and should not be modified. The solution should perform exactly the same operations on multisets as the reference implementation, only in parallel (in particular, it may perform them in a different order). Details are provided further in the program requirements.

## Scalability

Assume that the reference version runs on the given input in time tₛ, and the parallel version run with n helper threads runs in time tₙ (assuming that the machine has at least n physical cores). Then, the scalability factor is given by tₛ / tₙ. The higher the factor, the better the scalability; an ideally scaling solution would achieve a factor of n (technically, it is possible to have slightly more if the single-threaded version runs faster than the reference implementation).

## Suggested Approach

It is suggested to start with a non-recursive implementation (e.g., using your own implementation of a stack) and then parallelize it. Note that in the non-recursive implementation one should pay special attention to memory allocation and deallocation.

The difficulty in parallelizing lies in the fact that it is hard to predict which branches of the recursion will be the most computation-intensive; the work cannot be partitioned in advance. At the same time, solutions that synchronize in every iteration with other threads will be too slow. It is therefore suggested that each thread work most of the time on its own branch and only occasionally consider offloading a sub-branch to a common task pool. Threads that have finished computing their branch can fetch additional tasks from the common pool.

## Input/Output Format

The functions for correct parsing of the input and formatting of the output are provided in the files `io.h` and `io.c`.

The program receives three lines on the standard input. The first line contains four numbers: t, d, n, and m. Here, t represents the number of helper threads, d is the parameter d, and n and m denote the number of forced elements in A₀ and B₀ respectively. The second and third lines contain n and m numbers from the range 1 to d: the elements of the multiset A₀ and B₀ respectively. The goal is to compute α(d, A₀, B₀) using t helper threads. (You may exclude the main thread from the count of t if it does no computations with `sumset.h`.)

### Example Input

```
8 10 0 1
1
```

This means computing α(10, ∅, {1}) using eight helper threads.

### Output

The output should display the solution A, B (i.e., the uncontentious d-bounded multisets A ⊇ A₀, B ⊇ B₀ that maximize ∑A = ∑B). The first line should print the number ∑A. The second and third lines should provide the description of multisets A and B respectively. The description of a multiset consists of printing the elements along with their multiplicities separated by single spaces. If an element appears with multiplicity 1, print only the element itself; if an element a appears k times, print it as "kxa" (multiplicity, the letter "x", and the element). The elements should be printed in ascending order (regardless of multiplicities).

In the case when there is no solution, you should print a zero sum and two empty sets, i.e.,

```
0

```

(with an empty second and third line).

### Example Output

```
81
9x9
1 8x10
```

This is a valid output for the above example (not necessarily unique).

## Requirements

You can assume that:
- The input conforms to the specified format.
- 1 ≤ t ≤ 64, 3 ≤ d ≤ 50, 0 ≤ n ≤ 100, 0 ≤ m ≤ 100.
- α(d, A₀, B₀) ≤ d(d − 1) for all d, A₀, B₀.

The focus is on the runtime performance, correctness of the results, and (to a lesser degree) the submitted report.
