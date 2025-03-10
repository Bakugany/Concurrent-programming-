# Concurrent Boolean Circuits

Boolean circuits represent Boolean expressions using graphs. For example, the expression  
x ∧ (x ∨ ¬y) ∧ (z ∨ y)  
can be represented as a tree, or, if we allow operators with large arity, as a tree.

Conventionally, Boolean computations are performed from left to right. For instance, in the expression  
x ∧ y  
the value of x is computed first and then y. Lazy evaluation can skip computing parts of subexpressions if the values already computed allow one to determine the value of the entire expression. For example, the expression  
true ∨ x  
does not need to compute x to know that the final value evaluates to true. Notice that if expressions do not generate side effects, the order of evaluation of subexpressions should not affect the overall value of the expression. Hence, the evaluation of subexpressions can be done concurrently.

Your task is to implement a program that allows the concurrent evaluation of Boolean expressions. The program should support simultaneous evaluation of multiple expressions, and compute the value of individual expressions concurrently.

## Boolean Expression

A Boolean expression is defined inductively. The constants `true` and `false` are Boolean expressions. The expression `NOT a`, which negates the Boolean expression `a`, is a Boolean expression. The conjunction `AND(a1, a2, …)` and the disjunction `OR(a1, a2, …)` of a number of Boolean expressions (at least two) are Boolean expressions. The conditional expression `IF(a, b, c)` is a Boolean expression. Also, the threshold expressions `GTx(a1, a2, …, an)` and `LTx(a1, a2, …, an)`, where n ≥ 1 and x ≥ 0 are integers, are Boolean expressions.

## Semantics

For an expression `a`, we denote its value by `[a]`.

- `[true] = true`
- `[false] = false`

- `[AND(a1, a2, …, an)] = true` if every expression `ai` (1 ≤ i ≤ n) satisfies `[ai] = true`, and  
  `[AND(a1, a2, …, an)] = false` otherwise.

- `[OR(a1, a2, …, an)] = true` if there exists an expression `ai` (1 ≤ i ≤ n) that satisfies `[ai] = true`, and  
  `[OR(a1, a2, …, an)] = false` otherwise.

- `[GTx(a1, a2, …, an)] = true` if at least `x + 1` of the expressions `ai` (1 ≤ i ≤ n) satisfy `[ai] = true`, and  
  `[GTx(a1, a2, …, an)] = false` otherwise.

- `[LTx(a1, a2, …, an)] = true` if at most `x − 1` of the expressions `ai` (1 ≤ i ≤ n) satisfy `[ai] = true`, and  
  `[LTx(a1, a2, …, an)] = false` otherwise.

- `[IF(a, b, c)] = [b]` if `[a] = true`, and  
  `[IF(a, b, c)] = [c]` otherwise.

## Specification

In the solution, circuits are represented by objects of the class `Circuit`, and their values are computed by objects implementing the `CircuitSolver` interface, referred to as solvers.

```java
public interface CircuitSolver {
    public CircuitValue solve(Circuit c);
    
    public void stop();
}
```

The `CircuitValue` interface is defined as follows:

```java
public interface CircuitValue {
    public boolean getValue() throws InterruptedException;
}
```

The method `solve(Circuit c)` immediately returns a special object of type `CircuitValue` representing the value of the circuit. This value can be obtained by invoking the method `CircuitValue.getValue()`, which waits until the value is computed. Solvers should support concurrent handling of multiple requests (calls to `solve()`) and compute the values of individual circuits concurrently.

Calling the method `stop()` should cause the solver to stop accepting new `solve()` tasks and immediately terminate all computations currently running in that solver. From that moment on, the `CircuitValue` objects resulting from new and cancelled computations may throw an `InterruptedException` when their `getValue()` method is invoked. Other objects should correctly return the computed values of the circuits.

The class representing circuits, `Circuit`, and its helper classes will be provided in an archive. The `Circuit` interface is defined as follows:

```java
public class Circuit {
    private final CircuitNode root;

    public final CircuitNode getRoot();
}
```

The Boolean expressions contained in the `root` field have a tree structure represented by classes that include, among others, the following fields (a detailed interface is available in the archive provided in the "Solution" section):

```java
public class CircuitNode {
  private final NodeType type;
  private final CircuitNode[] args;
  
  public CircuitNode[] getArgs();
  public NodeType getType();
}
```

For threshold nodes:

```java
public class ThresholdNode extends CircuitNode {
    public int getThreshold();
}
```

For leaf nodes:

```java
public class LeafNode extends CircuitNode {
  public boolean getValue(); 
}
```

The `NodeType` is an enumerated type:

```java
public enum NodeType {
  LEAF, GT, LT, AND, OR, NOT, IF
}
```

with the natural interpretation of the symbols.

## Concurrency: Lifetime and Safety

The program should allow multiple concurrent requests for `solve()`. The results of the `solve()` calls should be returned as quickly as possible, and the values of computed expressions do not have to be used in the order of the calls. Both the values of leaves and operators should be computed concurrently. In particular, note that calls to both `LeafNode.getValue()` and `getArgs()` may take an arbitrarily long time to compute, but they do not produce side effects and correctly handle interruptions.

It can be assumed that external calls to interrupts (i.e., not resulting from your implementation), such as invoking `Thread.interrupt()`, will be limited to threads during calls to the method `CircuitValue.getValue()`.

In the solution, you can assume that every node in the tree structure of the expression is unique and that the sets of nodes in the tree structures of circuits are mutually disjoint. In particular, each call to `solve()` receives a different instance of `Circuit`. Additionally, in the implementation it can be assumed that the method `stop()` will be called on every instance of the `CircuitSolver` class that is created.
