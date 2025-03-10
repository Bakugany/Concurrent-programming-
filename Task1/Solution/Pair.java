package cp2024.solution;

public class Pair {
    private final Integer key;
    private final Result value;

    public Pair(Integer key, Result value) {
        this.key = key;
        this.value = value;
    }

    public Integer getKey() {
        return key;
    }

    public Result getValue() {
        return value;
    }

    @Override
    public String toString() {
        return "Pair{" +
                "key=" + key +
                ", value=" + value +
                '}';
    }
}