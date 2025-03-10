package cp2024.solution;


import cp2024.circuit.CircuitValue;

import java.util.concurrent.BlockingQueue;

public class ParallelCircuitValue implements CircuitValue {
    private final BlockingQueue<Pair> value;

    public ParallelCircuitValue(BlockingQueue<Pair> value) {
        this.value = value;
    }

    public boolean getValue() throws InterruptedException{
        Result result = value.take().getValue();
        if (result == Result.TRUE) {
            return true;
        } else if (result == Result.FALSE) {
            return false;
        } else {
            throw new InterruptedException();
        }
    }
}