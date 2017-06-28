Today's experiment including redoing the ones from yesterday, but allocating one more core for serve meta-thread and observe its effect on service and latency time

Also, as we figured out, TBENCH_NCLIENTS should remain 1, because it specifies the number the client PROCESSES that connects to server, not the number of threads. Therefore, I am going to rearrange the sequence of variables separate variables for server and variables for client.


experiment 1: file e1.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 2000
SERVER_CORE = 10,11

TBENCH_QPS = 100
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 12-23

OBSERVATION:
95 percentile service time: 4.083396
95 percentile latency time: 8.123413

This does not differ much from yesterday's data



experiment 2: file e2.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 10,11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 2
CLIENT_CORE = 12-23

OBESERVATION: 

95 percentile service time: 3.903678
95 percentile latency time: 13.86711

file e2_2.bin

95 percentile service time: 3.90593
95 percentile latency time: 13.827382

There is an increase in latency time. Double check to see if the result is consitent.   Yes, the result it consistent

Let's try pin server on 1 core again




experiment 3: file e3.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 2
CLIENT_CORE = 12-23

OBERSERVATION:
95 percentile service time: 3.873462
95 percentile latency time: 13.361118

not different by much. What if we try 1 thread for client?




experiment 4: file e4.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 12-23

OBSERVATION:
still not different by much
95 percentile service time: 3.910379
95 percentile latency time: 14.45114

This time, pin the overall program to another core while executing, and use htop to monitor




experiment 5: file e5.bin     pin run_networked to core 6, htop on core 4 
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 10,11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 12-23
 
OBSERVATION:
more than 3 cores ar utilized on the client side, CPU run at 100% when server is starting up. Result is still different from yesterday's

95 percentile service time: 3.961928
95 percentile latency time: 14.271116

try to pin clients only on three cores




experiment 6: file e6.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 10,11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23


OBSERVATOIN:
95 percentile service time: 3.85489
95 percentile latency time: 13.22675

this does not work either

try increase server threads



experiment 7: file e7.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 2
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 9-11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.101416
95 percentile latency time: 5.265252

A bit higher service time but much lower latency time. This is even lower than when QPS = 100 with 1 thread for server. Let's try 2 thread for server with QPS = 100


experiment 8: file e8.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 2
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 2000
SERVER_CORE = 9-11

TBENCH_QPS = 100
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.273552
95 percentile latency time: 4.869475

Latency is reduced a lot. Let's try 3 threads for server


experiment 9: file e9.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 3
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 2000
SERVER_CORE = 8-11

TBENCH_QPS = 100
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.331734
95 percentile latency time: 4.689806

okay, not different by much from last experiment. Let's now try 3 server threads with QPS = 200 (comparing with experiment 7)


experiment 10: file e10.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 3
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 8-11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.300518
95 percentile latency time: 4.702689

This is almost as good as when QPS = 100, and better than 2 threads for server. Now tune up QPS to 300



experiment 11: file e11.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 3
TBENCH_MAXREQS = 9000
TBENCH_WARMUPREQS = 6000
SERVER_CORE = 8-11

TBENCH_QPS = 300
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.281555
95 percentile latency time: 5.022248

One more thread for server



experiment 12: file e12.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 4
TBENCH_MAXREQS = 9000
TBENCH_WARMUPREQS = 6000
SERVER_CORE = 7-11

TBENCH_QPS = 300
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.372937
95 percentile latency time: 4.908082
does not differ from last experiment by much



experiment 13: file e13.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 3
TBENCH_MAXREQS = 12000
TBENCH_WARMUPREQS = 8000
SERVER_CORE = 8-11

TBENCH_QPS = 400
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 3.937018
95 percentile latency time: 4.615134


experiment 14: file e14.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 3
TBENCH_MAXREQS = 15000
TBENCH_WARMUPREQS = 10000
SERVER_CORE = 8-11

TBENCH_QPS = 500
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 3.817842
95 percentile latency time: 5.17603

queueing time increases above 1 second. Add another thread for server and see if latency decreases



experiment 15: file e15.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 4
TBENCH_MAXREQS = 15000
TBENCH_WARMUPREQS = 10000
SERVER_CORE = 7-11

TBENCH_QPS = 500
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.110036
95 percentile latency time: 4.932133




experiment 16: file e16.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 4
TBENCH_MAXREQS = 18000
TBENCH_WARMUPREQS = 12000
SERVER_CORE = 7-11

TBENCH_QPS = 600
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 4.008603
95 percentile latency time: 4.639607



experiment 17: file e17.bin     pin run_networked to core 6, htop on core 4
SERVER_THREADS = 4
TBENCH_MAXREQS = 21000
TBENCH_WARMUPREQS = 14000
SERVER_CORE = 7-11

TBENCH_QPS = 700
TENCH_CLIENT_THREADS = 1
CLIENT_CORE = 21-23

OBSERVATION:
95 percentile service time: 3.859864
95 percentile latency time: 4.646472

Since data is stablized for now, I am going to write a script that charaterizes latency time for 1-4 server threads and with QPS = 100-1000
