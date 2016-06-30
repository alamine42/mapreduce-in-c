# MapReduce in C

A basic implementation of map-reduce in C in the form of Word Count.

The project is composed of the following elements:
- driver.c : driver program responsible for reading data from file and allocating the tasks to the workers
- worker.c : contains all mapping logic
- dict.c : dictionary structure to hold word counts, used by workers
- reducer.c : contains reducing logic as a last step

Since this a networked setup, the driver is an HTTP client and workers are each their own HTTP server. However, workers are also HTTP clients when they communicate with the reducer, which is a multi-threaded HTTP server itself.

The whole thing was run on an AWS cluster with 6 EC2 nodes (1 driver, 4 workers, 1 reducer).

Programmer's note: the number and addresses of the workers are harcoded here, due to a time crunch. One obvious place of improvement is to put this information in configuration files. Another useful feature to add would be fault-tolerance / redundancy, whereby the driver would know if a worker node went down and would assign the work to another. FUTURE WORK.