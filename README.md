# accessDram

A tool to access specific DRAM components.
Requires the DRAM address mapping specified in a file.
An example is arcturus.amc.
For selecting components a comma separated list after the parameter (-c, -r -b) can be specified.
The number of threads can be changed using -t.
The size of the array can be changed with -s and the number of accesses with -n
This tool requires root access to translate virtual to physical addresses. It must be pinned to one processor on NUMA systems.
A typical use is:

sudo numactl --cpunodebind=1 --membind=1 ./accessDram -f arcturus.amc -c 0,1 -r 2,3 -b 4,5 -t 6

Here channels 0 and 1, ranks 1 and 2, and banks 4 and 5 are accessed using 6 threads.

The parameter -p enables partitioned access mode where each bank is assigned to a specific thread.
In this mode access should be restricted to one channel and one rank.

