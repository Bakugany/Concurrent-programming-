package cp2024.solution;

import cp2024.circuit.*;

import java.util.List;
import java.util.ArrayList;
import java.util.concurrent.*;

public class ParallelCircuitSolver implements CircuitSolver {
    private volatile boolean acceptComputations = true;
    private SolveTask rootTask;

    @Override
    public CircuitValue solve(Circuit c) {
        if (!acceptComputations) {
            return new ParallelBrokenCircuitValue();
        }
        try {
            BlockingQueue<Pair> resultQueue = new LinkedBlockingQueue<>();
            rootTask = new SolveTask(c.getRoot(), resultQueue, 0);
            Thread rootThread = new Thread(rootTask);
            rootThread.start();
            return new ParallelCircuitValue(resultQueue);
        } catch (Exception e) {
            return new ParallelBrokenCircuitValue();
        }
    }

    @Override
    public void stop() {
        acceptComputations = false;
        if (rootTask != null) {
            rootTask.cancel();
        }
    }

    private class SolveTask implements Runnable {
        private final int index;
        private final CircuitNode node;
        private final List<SolveTask> childTasks = new ArrayList<>();
        private final List<Thread> childThreads = new ArrayList<>();
        private final BlockingQueue<Pair> resultQueue;
        private Thread currentThread;

        SolveTask(CircuitNode node, BlockingQueue<Pair> resultQueue, int index) {
            this.node = node;
            this.resultQueue = resultQueue;
            this.index = index;
        }

        @Override
        public void run() {
            currentThread = Thread.currentThread();
            try {
                if (!acceptComputations) {
                    throw new CancellationException();
                }

                if (node.getType() == NodeType.LEAF) {
                    try {
                        if (((LeafNode) node).getValue()) {
                            resultQueue.put(new Pair(index, Result.TRUE));
                        } else {
                            resultQueue.put(new Pair(index, Result.FALSE));
                        }
                        return;
                    } catch (InterruptedException e) {
                        resultQueue.put(new Pair(index, Result.BROKEN));
                    }
                }

                CircuitNode[] args = node.getArgs();
                BlockingQueue<Pair> childResultQueue = new LinkedBlockingQueue<>();
                for (int i = 0; i < args.length; i++) {
                    SolveTask task = new SolveTask(args[i], childResultQueue, i);
                    childTasks.add(task);
                    Thread thread = new Thread(task);
                    childThreads.add(thread);
                    thread.start();
                }

                Result result = switch (node.getType()) {
                    case IF -> solveIF(childResultQueue, args.length);
                    case AND -> solveAND(childResultQueue, args.length);
                    case OR -> solveOR(childResultQueue, args.length);
                    case GT -> solveGT(childResultQueue, args.length, ((ThresholdNode) node).getThreshold());
                    case LT -> solveLT(childResultQueue, args.length, ((ThresholdNode) node).getThreshold());
                    case NOT -> solveNOT(childResultQueue);
                    default -> throw new RuntimeException("Illegal type " + node.getType());
                };

                resultQueue.put(new Pair(index, result));
            } catch (Exception e) {
                try {
                    resultQueue.put(new Pair(index, Result.BROKEN));
                } catch (InterruptedException ignored) {
                }
            }
        }

        public void cancel() {
            if (currentThread != null) {
                currentThread.interrupt();
            }
            for (SolveTask childTask : childTasks) {
                childTask.cancel();
            }
        }

        private Result solveNOT(BlockingQueue<Pair> queue) throws Exception {
            if (!acceptComputations) {
                throw new CancellationException();
            }
            Result task = queue.take().getValue();
            if (task == Result.BROKEN) {
                return Result.BROKEN;
            }
            if (task == Result.TRUE) {
                return Result.FALSE;
            }
            return Result.TRUE;
        }

        private Result solveLT(BlockingQueue<Pair> queue, int numTasks, int threshold) throws Exception {
            int gotTrue = 0;
            int gotFalse = 0;
            if (gotTrue >= threshold) {
                return Result.FALSE;
            }
            if (gotFalse > numTasks - threshold) {
                return Result.TRUE;
            }
            for (int i = 0; i < numTasks; i++) {
                if (!acceptComputations) {
                    throw new CancellationException();
                }
                Result task = queue.take().getValue();
                if (task == Result.BROKEN) {
                    return Result.BROKEN;
                }
                if (task == Result.TRUE) {
                    gotTrue++;
                }
                if (task == Result.FALSE) {
                    gotFalse++;
                }
                if (gotTrue >= threshold) {
                    return Result.FALSE;
                }
                if (gotFalse > numTasks - threshold) {
                    return Result.TRUE;
                }
            }
            return Result.FALSE;
        }

        private Result solveGT(BlockingQueue<Pair> queue, int numTasks, int threshold) throws Exception {
            int gotTrue = 0;
            int gotFalse = 0;
            if (gotFalse >= numTasks - threshold) {
                return Result.FALSE;
            }
            for (int i = 0; i < numTasks; i++) {
                if (!acceptComputations) {
                    throw new CancellationException();
                }
                Result task = queue.take().getValue();
                if (task == Result.BROKEN) {
                    return Result.BROKEN;
                }
                if (task == Result.TRUE) {
                    gotTrue++;
                }
                if (task == Result.FALSE) {
                    gotFalse++;
                }
                if (gotTrue > threshold) {
                    return Result.TRUE;
                }
                if (gotFalse >= numTasks - threshold) {
                    return Result.FALSE;
                }
            }
            return Result.FALSE;
        }

        private Result solveOR(BlockingQueue<Pair> queue, int numTasks) throws Exception {
            for (int i = 0; i < numTasks; i++) {
                if (!acceptComputations) {
                    throw new CancellationException();
                }
                Result task = queue.take().getValue();
                if (task == Result.TRUE) {
                    return Result.TRUE;
                }
                if (task == Result.BROKEN) {
                    return Result.BROKEN;
                }
            }
            return Result.FALSE;
        }

        private Result solveAND(BlockingQueue<Pair> queue, int numTasks) throws Exception {
            for (int i = 0; i < numTasks; i++) {
                if (!acceptComputations) {
                    throw new CancellationException();
                }
                Result task = queue.take().getValue();
                if (task == Result.FALSE) {
                    return Result.FALSE;
                }
                if (task == Result.BROKEN) {
                    return Result.BROKEN;
                }
            }
            return Result.TRUE;
        }

        private Result solveIF(BlockingQueue<Pair> queue, int numTasks) throws Exception {
            if (!acceptComputations) {
                throw new CancellationException();
            }
            Result condition = null;
            Result firstOption = null;
            Result secondOption = null;
            for (int i = 0; i < numTasks; i++) {
                Pair task = queue.take();
                if (task.getKey() == 0) {
                    condition = task.getValue();
                    if (condition == Result.BROKEN) {
                        return Result.BROKEN;
                    }
                    if (condition == Result.TRUE && firstOption != null) {
                        return firstOption;
                    }
                    if (condition == Result.FALSE && secondOption != null) {
                        return secondOption;
                    }
                } else if (task.getKey() == 1) {
                    firstOption = task.getValue();
                    if (condition == Result.TRUE) {
                        return firstOption;
                    }
                    if (firstOption == secondOption){
                        return firstOption;
                    }
                } else {
                    secondOption = task.getValue();
                    if (condition == Result.FALSE) {
                        return secondOption;
                    }
                    if (firstOption == secondOption){
                        return firstOption;
                    }
                }
            }
            return Result.BROKEN;
        }
    }
}