Create a graph with #particles vertices and just make a vertex-independent
transformation. This corresponds to a Map operation in the MapReduce framework.
The transformation is repeated a #iterations times.

# Experiment 1
--------------

#iterations = 1000

| # particles | strong node time (s) | weak node time (s) | distributed system (s) |
|:-----------:|:--------------------:|:------------------:|:----------------------:|
| 1k          | 0.14                 | 0.54               | 34 (33-40)             |
| 10k         | 1.3                  | 5.2                | 38 (36-40)             |
| 100k        | 13                   | 52                 | 60                     |

# Experiment 2
--------------

Using the distributed setup, 1000 particles and varying the number of iterations.

| # iterations | ~ time (s) | 
|:------------:|:----------:|
| 1            | 0.03       |
| 10           | 0.3        |
| 100          | 2.7        |
| 1000         | 30         |
