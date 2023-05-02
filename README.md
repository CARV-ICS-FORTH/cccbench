cccbench
=======

cccbench (carv-ccbench) is a benchmark that measures the communication latency between two cores for very small (single cache line) messages.
To achieve this, the benchmark ping-pongs a single cache line between the two cores.
The initial inspitation for this benchmark is ccbench, written by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>. In fact before deciding to write
a benchmark from scratch, we decided to adapt ccbench to our requirements. Although, we have now taken a different approach, the porting effort and additional features we added to the original ccbench can be found in the `ccbench-port` branch of this repository.

cccbench can be used as a standalone tool or as a library. Within the source directory an example script `example-script.sh` is provided, demonstrating how to benchmark can be used to create a csv file, for a multi-iteration run that collects results for a set of cores of a machine.
The library interface of cccbench can be found in the `c2c.h` header file

