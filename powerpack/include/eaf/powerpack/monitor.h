#ifndef __EAF_POWERPACK_MONITOR_H__
#define __EAF_POWERPACK_MONITOR_H__
#ifdef __cplusplus
extern "C" {
#endif

int eaf_monitor_init(void);
void eaf_monitor_exit(void);

void eaf_monitor_print_tree(char* buffer, size_t size);

#ifdef __cplusplus
}
#endif
#endif
