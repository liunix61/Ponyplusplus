/*
 * debugger.h - Pony++ DAP 调试器 (Debug Adapter Protocol)
 *
 * 实现 VS Code / IDE 兼容的 DAP 协议。
 * 支持: 断点、单步执行、变量检查、调用栈、Actor状态检查。
 *
 * Phase 3: 调试器
 */
#ifndef PONYPP_DEBUGGER_H
#define PONYPP_DEBUGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* ======================== DAP 协议常量 ======================== */

/* DAP 消息类型 */
typedef enum DapMessageType {
    DAP_MSG_REQUEST = 0,
    DAP_MSG_RESPONSE,
    DAP_MSG_EVENT
} DapMessageType;

/* DAP 事件类型 */
typedef enum DapEventKind {
    DAP_EVENT_INITIALIZED,
    DAP_EVENT_STOPPED,
    DAP_EVENT_CONTINUED,
    DAP_EVENT_TERMINATED,
    DAP_EVENT_OUTPUT,
    DAP_EVENT_BREAKPOINT,
    DAP_EVENT_THREAD,
    DAP_EVENT_ACTOR  /* Pony++ 特有: Actor 状态变化 */
} DapEventKind;

/* DAP 断点类型 */
typedef enum DapBreakpointKind {
    DAP_BP_LINE,       /* 行断点 */
    DAP_BP_FUNCTION,   /* 函数断点 */
    DAP_BP_ACTOR,      /* Actor 断点 (Pony++ 特有) */
    DAP_BP_MESSAGE     /* 消息断点 (Pony++ 特有) */
} DapBreakpointKind;

/* ======================== 断点结构 ======================== */

typedef struct DapBreakpoint {
    int id;
    DapBreakpointKind kind;
    char file[256];
    int line;
    char function[128];
    int actor_id;       /* Actor 断点: 目标 Actor */
    char method[128];   /* 消息断点: 目标方法 */
    int enabled;
    int hit_count;
    struct DapBreakpoint *next;
} DapBreakpoint;

/* ======================== 调用栈帧 ======================== */

typedef struct DapStackFrame {
    int id;
    char name[128];
    char file[256];
    int line;
    int column;
    int actor_id;       /* 所属 Actor */
    struct DapStackFrame *next;
} DapStackFrame;

/* ======================== 变量 ======================== */

typedef struct DapVariable {
    char name[128];
    char value[256];
    char type[64];
    int variables_reference;  /* 0 = 叶子, >0 = 可展开 */
    struct DapVariable *next;
} DapVariable;

/* ======================== 调试器状态 ======================== */

typedef enum DapState {
    DAP_STATE_IDLE,
    DAP_STATE_INITIALIZED,
    DAP_STATE_RUNNING,
    DAP_STATE_STOPPED,
    DAP_STATE_TERMINATED
} DapState;

/* ======================== 调试器上下文 ======================== */

typedef struct DapDebugger DapDebugger;

/* 回调: 运行时通知调试器 */
typedef void (*DapOnBreakpoint)(DapDebugger *dbg, const DapBreakpoint *bp);
typedef void (*DapOnActorCreated)(DapDebugger *dbg, int actor_id, const char *name);
typedef void (*DapOnMessageSent)(DapDebugger *dbg, int from_id, int to_id, const char *method);

/* ======================== 创建/销毁 ======================== */

DapDebugger *dap_new(FILE *in, FILE *out);
void dap_free(DapDebugger *dbg);

/* ======================== 消息处理 ======================== */

/* 处理一条 DAP 消息 (阻塞式读取 stdin) */
int dap_process_message(DapDebugger *dbg);

/* 运行主循环: 持续处理消息直到 terminated */
int dap_run(DapDebugger *dbg);

/* ======================== 断点管理 ======================== */

/* 设置行断点 */
int dap_set_breakpoint(DapDebugger *dbg, const char *file, int line);

/* 设置函数断点 */
int dap_set_function_breakpoint(DapDebugger *dbg, const char *function);

/* 设置 Actor 断点 (Pony++ 特有) */
int dap_set_actor_breakpoint(DapDebugger *dbg, int actor_id);

/* 设置消息断点 (Pony++ 特有) */
int dap_set_message_breakpoint(DapDebugger *dbg, const char *method);

/* 删除断点 */
int dap_remove_breakpoint(DapDebugger *dbg, int id);

/* 启用/禁用断点 */
int dap_enable_breakpoint(DapDebugger *dbg, int id, int enabled);

/* 检查是否命中断点 */
int dap_check_breakpoint(DapDebugger *dbg, const char *file, int line);
int dap_check_actor_breakpoint(DapDebugger *dbg, int actor_id);
int dap_check_message_breakpoint(DapDebugger *dbg, const char *method);

/* ======================== 执行控制 ======================== */

/* 继续执行 */
int dap_continue(DapDebugger *dbg);

/* 单步跳过 (next) */
int dap_step_over(DapDebugger *dbg);

/* 单步进入 (stepIn) */
int dap_step_into(DapDebugger *dbg);

/* 单步跳出 (stepOut) */
int dap_step_out(DapDebugger *dbg);

/* 暂停 */
int dap_pause(DapDebugger *dbg);

/* ======================== 状态检查 ======================== */

/* 获取调用栈 */
DapStackFrame *dap_get_stack_trace(DapDebugger *dbg, int *count);

/* 获取变量 */
DapVariable *dap_get_variables(DapDebugger *dbg, int variables_reference, int *count);

/* 获取 Actor 列表 (Pony++ 特有) */
typedef struct DapActorInfo {
    int id;
    char name[128];
    int state;           /* ActorState */
    size_t message_count;
    int mailbox_size;
} DapActorInfo;

DapActorInfo *dap_get_actors(DapDebugger *dbg, int *count);

/* 获取消息队列 (Pony++ 特有) */
typedef struct DapMessageInfo {
    int msg_id;
    int sender_id;
    int receiver_id;
    char method[128];
    size_t arg_size;
} DapMessageInfo;

DapMessageInfo *dap_get_messages(DapDebugger *dbg, int actor_id, int *count);

/* ======================== 运行时集成 ======================== */

/* 运行时回调注册 */
void dap_set_on_breakpoint(DapDebugger *dbg, DapOnBreakpoint cb);
void dap_set_on_actor_created(DapDebugger *dbg, DapOnActorCreated cb);
void dap_set_on_message_sent(DapDebugger *dbg, DapOnMessageSent cb);

/* 运行时通知 (由 runtime 调用) */
void dap_notify_breakpoint(DapDebugger *dbg, const char *file, int line);
void dap_notify_actor_created(DapDebugger *dbg, int actor_id, const char *name);
void dap_notify_message(DapDebugger *dbg, int from_id, int to_id, const char *method);

/* ======================== 工具函数 ======================== */

/* 获取调试器状态 */
DapState dap_get_state(DapDebugger *dbg);

/* 获取断点数量 */
int dap_get_breakpoint_count(DapDebugger *dbg);

/* 列出所有断点 */
DapBreakpoint *dap_list_breakpoints(DapDebugger *dbg);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_DEBUGGER_H */
