Create a graph with #particles vertices and just make a vertex-independent
transformation. This corresponds to a Map operation in the MapReduce framework.
The transformation is repeated a #iterations times.

### Experiment 1

\#iterations = 1000

| # particles | strong node time (s) | weak node time (s) | distributed system (s) |
|:-----------:|:--------------------:|:------------------:|:----------------------:|
| 1k          | 0.14                 | 0.54               | 34 (33-40)             |
| 10k         | 1.3                  | 5.2                | 38 (36-40)             |
| 100k        | 13                   | 52                 | 60                     |

### Experiment 2

Using the distributed setup, 1000 particles and varying the number of iterations.

| # iterations | ~ time (s) |
|:------------:|:----------:|
| 1            | 0.03       |
| 10           | 0.3        |
| 100          | 2.7        |
| 1000         | 30         |

### Experiment 3

Distributed setup. Compare aggregation framework, complete, and sparse graphs. Number of
particles equal to 1000. Time per iteration obtained averaging the total total time for
ten iterations.

| aggregation (s) | complete (s) | sparse (s) |
|:---------------:|:------------:|:----------:|
|    0.15         |      7.94    |    0.19    |

Time for the complete graph as a function of the number of particles.

| # particles | complete (s) |
|:-----------:|:------------:|
|    1000     |      7.9     |
|     100     |      0.2     |
|      10     |      0.15    |
