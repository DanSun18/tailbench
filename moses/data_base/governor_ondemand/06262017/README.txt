Running experiments for moses 

experiment 1: file e1.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 200
TBENCH_NCLIENTS= 1
SERVER_CORE = 11

TBENCH_QPS = 100
TENCH_CLIENT_THREADS = TBENCH_NCLIENTS
CLIENT_CORE = 12-23

OBERSERVATION:
95 percentile service time: 5.562831
95 percentile latency time: 652.120789

Queueing time is stablized after approximately 2000 requests, so this time try to run with WARMUPREQS = 2000




experiment 2: file e2.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 2000
TBENCH_NCLIENTS= 1
SERVER_CORE = 11

TBENCH_QPS = 100
TENCH_CLIENT_THREADS = TBENCH_NCLIENTS
CLIENT_CORE = 12-23

OBSERVATION:
95 percentile service time: 4.102357
95 percentile latency time: 8.083351

file e2_2.bin
95 percentile service time: 4.101193
95 percentile latency time: 8.076797

The results are quite consistent



experiment 3: file e3.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 2000
TBENCH_NCLIENTS= 1
SERVER_CORE = 11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = TBENCH_NCLIENTS
CLIENT_CORE = 12-23

OBSERVATION: 
queueing time stablizes after another 2000 requests (that is, 4000 warmup in total)
also, change MAXREQS TO 6000 in next run

95 percentile service time: 4.13538
95 percentile latency time: 2331.872255



experiment 4: file e4.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
TBENCH_NCLIENTS= 1
SERVER_CORE = 11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = TBENCH_NCLIENTS
CLIENT_CORE = 12-23

OBSERVATION:
95 percentile service time: 3.916728
95 percentile latency time: 14.324525

Is this a consitent result? running experiments with this parameter again
file e4_2.bin
95 percentile service time: 3.878937
95 percentile latency time: 13.405155

Compared to when QPS = 100. service time does not change by much, but latency is doubled. Run experiment 2 to determine if data obtained was legit. Turns out it is.

With roughly the same service time but increased latency time, it means queueing time must have increased. However, it is uncertain whether this is due to client being unable to sen requests on time or server capacity. Therefore, for the next experiment I am going to increase the number of client threads to 2.



experiment 5: file e5.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 6000
TBENCH_WARMUPREQS = 4000
TBENCH_NCLIENTS= 2
SERVER_CORE = 11

TBENCH_QPS = 200
TENCH_CLIENT_THREADS = TBENCH_NCLIENTS
CLIENT_CORE = 12-23

OBESERVATION: program is stuck. need to figure out why.
By testing server and client separately I found that request is sent but not responded by server. Trying to see if this is caused by the parameter TBENCH_NCLIENTS for server. Setting this to one for server, but client remains using two threads. By running again it seems that the problem is indeed caused by the parameter TBENCH_NCLIENtS differ from 1. 

However, experimental data (file e5_2.bin) suggests that the increases in latency is caused by limitation on the client side. This raises the question: if I increase the number of clients to issue request, would it decrease service time for when QPS = 100? 

95 percentile service time: 3.707507
95 percentile latency time: 7.561808

TODO: 
1) fix the problem when modifying TBENCH_NCLIENTS causes server to stop responding
2) increase the number of clients for QPS=100 and observe if there is any change in latency/service time



experiment 6: file e6.bin
SERVER_THREADS = 1
TBENCH_MAXREQS = 3000
TBENCH_WARMUPREQS = 2000
(TBENCH_NCLIENTS= 1) this parameter is by default
TENCH_CLIENT_THREADS = 2
SERVER_CORE = 11

TBENCH_QPS = 100
TENCH_CLIENT_THREADS = 2
CLIENT_CORE = 12-23

OBSERVATION:
No, increasing Client threads at QPS = 100 does not reduce service/latency time.However, it is uncertain if this is still the case after we fix the TBENCH_NCLIENTS problem
95 percentile service time: 4.158718
95 percentile latency time: 8.318623
