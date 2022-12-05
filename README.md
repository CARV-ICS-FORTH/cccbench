cccbench
=======

cccbench (carv-ccbench) is a modification of ccbench, written by Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>. Its purpose is for measuring the cache-coherence latencies of a processor, i.e., the latencies of `loads`, `stores`, `compare-and-swap (CAS)`, `fetch-and-increment (FAI)`, `test-and-set (TAS)`, and `swap (SWAP)`. The latencies that ccbench measures can be used to understand and predict the behavior of sharing and synchronization on the underlying hardware platform. CARV ccbench contributes Three new features to the original ccbench.

1. Ports it to arm64 architecture
2. Adds a programming interface that allows ccbench to be used as a library.
3. Through the use of hwloc, it allows the choice of instrumentor core selection and buffer binding to a NUMA node passed as parameter(those options are currently supported only in the library version).

For a reference to the standalone use of cccbench, refer to the orig-README.md (the interface is the same as the original ccbench)
For the library version of cccbench, refer to the header file in the source code: include/lib-ccbench.h 


