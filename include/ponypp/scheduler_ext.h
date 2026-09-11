#ifndef PONYPP_SCHEDULER_EXT_H
#define PONYPP_SCHEDULER_EXT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PnyActor;
struct PnyMessage;

/* ==================== 降级模式 ==================== */

typedef enum {
    PNY_DEGRADE_SERVER = 0,   /* M:N 工作窃取 */
    PNY_DEGRADE_BROWSER = 1,  /* Web Workers */
    PNY_DEGRADE_EMBEDDED = 2  /* 单线程 */
} PnyDegradeMode;

PnyDegradeMode pny_scheduler_detect_mode(void);
void pny_scheduler_set_mode(PnyDegradeMode mode);
PnyDegradeMode pny_scheduler_get_mode(void);
int pny_scheduler_get_worker_count(void);

/* ==================== 调度器优先级 ==================== */

#define PNY_PRIO_NEW_ACTOR    0
#define PNY_PRIO_NORMAL       128
#define PNY_PRIO_GAS_EXHAUST  254

void pny_prio_enqueue(struct PnyActor *a, struct PnyMessage *msg, int priority);
int pny_prio_dequeue(struct PnyActor **out_actor, struct PnyMessage **out_msg);
size_t pny_prio_count(void);

/* ==================== I/O 线程池 ==================== */

typedef void (*io_callback_t)(void *arg);

int pny_io_pool_init(int thread_count);
int pny_io_pool_submit(io_callback_t callback, void *arg);
size_t pny_io_pool_pending(void);
void pny_io_pool_shutdown(void);

/* ==================== 跨Actor引用扫描 ==================== */

void pny_cross_ref_add(struct PnyActor *source, struct PnyActor *target);
void pny_cross_ref_remove(struct PnyActor *source, struct PnyActor *target);
void pny_cross_ref_mark_reachable(void (*mark_fn)(struct PnyActor *));
size_t pny_cross_ref_count(void);
void pny_cross_ref_clear(void);

#ifdef __cplusplus
}
#endif
#endif /* PONYPP_SCHEDULER_EXT_H */
