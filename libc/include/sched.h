#ifndef davecc_sched_h
#define davecc_sched_h

struct sched_param {
  int sched_priority;
};

#ifdef __cplusplus
extern "C" {
#endif

int sched_getcpu(void);
int sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif
