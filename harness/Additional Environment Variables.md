Additional Environment Variables

META_THREAD_CORE : set cpu affinity of the meta thread to a particular logical core. Default to 7

RECEIVER_THREAD_CORE: set cpu affinity of the receiver thread to a particular logical core temporarily, as the meta-thread halts the receiver thread will migrate to the core assigned to meta thread. The default value is 0.

SERVER_THREAD_N_CORE: spcifies the cpu affinity of worker thread N. Should be specified for each worker thread. For example, if the server is set to have 2 worker threads, then both SERVER_THREAD_0_CORE and SERVER_THREAD_1_CORE should be specified. default value is 6-N.