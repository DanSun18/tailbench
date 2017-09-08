This folder contains data obtained using the dynamic load tester programmed

experiment 1:

file: e1.bin e1.input

description: this experiment aims to confirm that the dynamic load tester is working. In doing this, I am going to change QPS from 200 to 800 then back to 200 again. While doing this I am also going to use htop to monitor CPU ussage. The data obtained is going to be compared with the pattern obtained for when QPS = 200 and when QPS = 1000. If they match then the conclusion is that load tester is working (at least for this experiment).

parameters:
RUN_NETWORKED_CORE = 4
HTOP_CORE = 6

SERVER_THREADS = 2
TBENCH_MAXREQS = 21000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 21-23

TBENCH_QPS = 200
TBENCH_CLIENT_THREADS = 1
CLIENT_CORE = 9-11

OBSERVATION
CPU usage increased from ~30% to ~90% during the process. It dropped back down to ~30% for a short time. This may be due to MAX_REQS being too small. This time I shall increase the number of max requests.



experiment 2:

file: e2.bin e2.input

parameters:
RUN_NETWORKED_CORE = 4
HTOP_CORE = 6

SERVER_THREADS = 2
TBENCH_MAXREQS = 30000
TBENCH_WARMUPREQS = 4000
SERVER_CORE = 21-23

TBENCH_QPS = 200
TBENCH_CLIENT_THREADS = 1
CLIENT_CORE = 9-11

OBSERVATION
As expected, a rise from ~30% CPU usage each on server rised to ~90% and dropped back to ~30%

Exiaminating of data
