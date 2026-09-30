#ifndef davecc_mach_task_h
#define davecc_mach_task_h

#include <mach/mach_init.h>

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t task_threads(task_inspect_t target_task,
                           thread_act_array_t* act_list,
                           mach_msg_type_number_t* act_listCnt);

#ifdef __cplusplus
}
#endif

#endif
