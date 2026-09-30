#ifndef davecc_mach_init_h
#define davecc_mach_init_h

#include <stdint.h>

typedef int kern_return_t;
typedef unsigned int mach_port_t;
typedef mach_port_t task_t;
typedef mach_port_t task_inspect_t;
typedef mach_port_t vm_map_t;
typedef mach_port_t thread_act_t;
typedef thread_act_t thread_t;
typedef thread_act_t* thread_act_array_t;
typedef unsigned int mach_msg_type_number_t;
typedef uint64_t vm_address_t;
typedef uint64_t vm_size_t;

#define KERN_SUCCESS 0

extern mach_port_t mach_task_self_;
#define mach_task_self() mach_task_self_

#endif
