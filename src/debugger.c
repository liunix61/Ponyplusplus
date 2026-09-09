/*
 * debugger.c - Pony++ DAP 调试器实现
 *
 * 实现 VS Code / IDE 兼容的 DAP 协议。
 * 消息格式: Content-Length: N\r\n\r\n{JSON}
 *
 * Phase 3: 调试器
 */

#include "ponypp/debugger.h"
#include "ponypp/util.h"
#include <stdlib.h>
#include <string.h>

/* ======================== 内部结构 ======================== */

struct DapDebugger {
    FILE *in;
    FILE *out;
    DapState state;
    
    /* 断点链表 */
    DapBreakpoint *breakpoints;
    int next_bp_id;
    int bp_count;
    
    /* 调用栈 */
    DapStackFrame *stack_frames;
    int frame_count;
    
    /* 变量 */
    DapVariable *variables;
    int var_count;
    
    /* Actor 信息 */
    DapActorInfo *actors;
    int actor_count;
    
    /* 消息信息 */
    DapMessageInfo *messages;
    int message_count;
    
    /* 执行控制 */
    int step_mode;  /* 0=none, 1=over, 2=into, 3=out */
    int paused;
    
    /* 回调 */
    DapOnBreakpoint on_breakpoint;
    DapOnActorCreated on_actor_created;
    DapOnMessageSent on_message_sent;
    
    /* 序列号 */
    int seq;
    
    /* 客户端能力 */
    int supports_configuration_done;
    int supports_actors_request;  /* Pony++ 扩展 */
};

/* ======================== JSON 辅助函数 ======================== */

/* 简单 JSON 字符串提取 (避免依赖外部库) */
static const char *json_get_string(const char *json, const char *key, char *buf, size_t buf_size) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return NULL;
    
    pos = strchr(pos + strlen(pattern), ':');
    if (!pos) return NULL;
    pos++;
    
    /* 跳过空白 */
    while (*pos == ' ' || *pos == '\t') pos++;
    
    if (*pos != '"') return NULL;
    pos++;
    
    size_t i = 0;
    while (*pos && *pos != '"' && i < buf_size - 1) {
        if (*pos == '\\' && *(pos + 1)) {
            pos++;
            switch (*pos) {
                case 'n': buf[i++] = '\n'; break;
                case 't': buf[i++] = '\t'; break;
                case 'r': buf[i++] = '\r'; break;
                case '"': buf[i++] = '"'; break;
                case '\\': buf[i++] = '\\'; break;
                default: buf[i++] = *pos; break;
            }
        } else {
            buf[i++] = *pos;
        }
        pos++;
    }
    buf[i] = '\0';
    return buf;
}

/* 简单 JSON 整数提取 */
static int json_get_int(const char *json, const char *key, int default_val) {
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *pos = strstr(json, pattern);
    if (!pos) return default_val;
    
    pos = strchr(pos + strlen(pattern), ':');
    if (!pos) return default_val;
    pos++;
    
    while (*pos == ' ' || *pos == '\t') pos++;
    
    return atoi(pos);
}

/* ======================== DAP 消息发送 ======================== */

static void dap_send_response(DapDebugger *dbg, int request_seq, const char *command, 
                               int success, const char *body) {
    char msg[4096];
    int len;
    
    if (body && body[0]) {
        len = snprintf(msg, sizeof(msg),
            "{\"seq\":%d,\"type\":\"response\",\"request_seq\":%d,"
            "\"command\":\"%s\",\"success\":%s,\"body\":%s}",
            dbg->seq++, request_seq, command, success ? "true" : "false", body);
    } else {
        len = snprintf(msg, sizeof(msg),
            "{\"seq\":%d,\"type\":\"response\",\"request_seq\":%d,"
            "\"command\":\"%s\",\"success\":%s}",
            dbg->seq++, request_seq, command, success ? "true" : "false");
    }
    
    fprintf(dbg->out, "Content-Length: %d\r\n\r\n%s", len, msg);
    fflush(dbg->out);
}

static void dap_send_event(DapDebugger *dbg, const char *event, const char *body) {
    char msg[4096];
    int len;
    
    if (body && body[0]) {
        len = snprintf(msg, sizeof(msg),
            "{\"seq\":%d,\"type\":\"event\",\"event\":\"%s\",\"body\":%s}",
            dbg->seq++, event, body);
    } else {
        len = snprintf(msg, sizeof(msg),
            "{\"seq\":%d,\"type\":\"event\",\"event\":\"%s\"}",
            dbg->seq++, event);
    }
    
    fprintf(dbg->out, "Content-Length: %d\r\n\r\n%s", len, msg);
    fflush(dbg->out);
}

/* ======================== 创建/销毁 ======================== */

DapDebugger *dap_new(FILE *in, FILE *out) {
    DapDebugger *dbg = (DapDebugger *)calloc(1, sizeof(DapDebugger));
    if (!dbg) return NULL;
    
    dbg->in = in ? in : stdin;
    dbg->out = out ? out : stdout;
    dbg->state = DAP_STATE_IDLE;
    dbg->next_bp_id = 1;
    dbg->seq = 1;
    
    return dbg;
}

void dap_free(DapDebugger *dbg) {
    if (!dbg) return;
    
    /* 释放断点链 */
    DapBreakpoint *bp = dbg->breakpoints;
    while (bp) {
        DapBreakpoint *next = bp->next;
        free(bp);
        bp = next;
    }
    
    /* 释放调用栈 */
    DapStackFrame *sf = dbg->stack_frames;
    while (sf) {
        DapStackFrame *next = sf->next;
        free(sf);
        sf = next;
    }
    
    /* 释放变量 */
    DapVariable *var = dbg->variables;
    while (var) {
        DapVariable *next = var->next;
        free(var);
        var = next;
    }
    
    free(dbg->actors);
    free(dbg->messages);
    free(dbg);
}

/* ======================== 消息读取 ======================== */

static int dap_read_message(DapDebugger *dbg, char *buf, size_t buf_size) {
    char line[256];
    int content_length = 0;
    
    /* 读取 Content-Length 头 */
    while (fgets(line, sizeof(line), dbg->in)) {
        if (strncmp(line, "Content-Length:", 15) == 0) {
            content_length = atoi(line + 15);
            break;
        }
        /* 空行表示头部结束 */
        if (line[0] == '\r' || line[0] == '\n') break;
    }
    
    if (content_length <= 0) return -1;
    
    /* 跳过头部结束的空行 */
    while (fgets(line, sizeof(line), dbg->in)) {
        if (line[0] == '\r' || line[0] == '\n') break;
    }
    
    /* 读取消息体 */
    if ((size_t)content_length >= buf_size) return -1;
    
    size_t total = 0;
    while (total < (size_t)content_length) {
        size_t n = fread(buf + total, 1, content_length - total, dbg->in);
        if (n == 0) break;
        total += n;
    }
    buf[total] = '\0';
    
    return (int)total;
}

/* ======================== 请求处理 ======================== */

static void dap_handle_initialize(DapDebugger *dbg, int seq, const char *args) {
    dbg->supports_configuration_done = json_get_int(args, "supportsConfigurationDoneRequest", 0);
    dbg->state = DAP_STATE_INITIALIZED;
    
    const char *body = "{"
        "\"supportsConfigurationDoneRequest\":true,"
        "\"supportsFunctionBreakpoints\":true,"
        "\"supportsConditionalBreakpoints\":false,"
        "\"supportsHitConditionalBreakpoints\":false,"
        "\"supportsEvaluateForHovers\":false,"
        "\"supportsStepBack\":false,"
        "\"supportsSetVariable\":false,"
        "\"supportsRestartFrame\":false,"
        "\"supportsGotoTargetsRequest\":false,"
        "\"supportsStepInTargetsRequest\":false,"
        "\"supportsCompletionsRequest\":false,"
        "\"supportsModulesRequest\":false,"
        "\"supportsRestartRequest\":false,"
        "\"supportsExceptionOptions\":false,"
        "\"supportsValueFormattingOptions\":false,"
        "\"supportsExceptionInfoRequest\":false,"
        "\"supportTerminateDebuggee\":true,"
        "\"supportsDelayedStackTraceLoading\":false,"
        "\"supportsLoadedSourcesRequest\":false,"
        "\"supportsLogPoints\":false,"
        "\"supportsTerminateThreadsRequest\":false,"
        "\"supportsSetExpression\":false,"
        "\"supportsTerminateRequest\":true,"
        "\"supportsDataBreakpoints\":false,"
        "\"supportsReadMemoryRequest\":false,"
        "\"supportsWriteMemoryRequest\":false,"
        "\"supportsDisassembleRequest\":false,"
        "\"supportsCancelRequest\":false,"
        "\"supportsBreakpointLocationsRequest\":false,"
        "\"supportsClipboardContext\":false,"
        "\"supportsSteppingGranularity\":false,"
        "\"supportsInstructionBreakpoints\":false,"
        "\"supportsExceptionFilterOptions\":false,"
        "\"extension\":{\"ponyppActors\":true,\"ponyppMessages\":true}"
    "}";
    
    dap_send_response(dbg, seq, "initialize", 1, body);
    dap_send_event(dbg, "initialized", NULL);
}

static void dap_handle_launch(DapDebugger *dbg, int seq, const char *args) {
    char program[256];
    if (!json_get_string(args, "program", program, sizeof(program))) {
        dap_send_response(dbg, seq, "launch", 0, "{\"error\":\"missing program\"}");
        return;
    }
    
    dbg->state = DAP_STATE_RUNNING;
    dap_send_response(dbg, seq, "launch", 1, NULL);
}

static void dap_handle_set_breakpoints(DapDebugger *dbg, int seq, const char *args) {
    char file[256];
    if (!json_get_string(args, "source", file, sizeof(file))) {
        /* 尝试从 source.path 提取 */
        const char *src = strstr(args, "\"source\"");
        if (src) {
            json_get_string(src, "path", file, sizeof(file));
        }
    }
    
    /* 解析断点数组 (简化: 只取第一个断点的行号) */
    const char *bp_array = strstr(args, "\"breakpoints\"");
    if (!bp_array) {
        dap_send_response(dbg, seq, "setBreakpoints", 1, "{\"breakpoints\":[]}");
        return;
    }
    
    const char *line_pos = strstr(bp_array, "\"line\"");
    if (!line_pos) {
        dap_send_response(dbg, seq, "setBreakpoints", 1, "{\"breakpoints\":[]}");
        return;
    }
    
    int line = json_get_int(line_pos, "line", 0);
    
    /* 清除该文件的所有断点 */
    DapBreakpoint **pp = &dbg->breakpoints;
    while (*pp) {
        if (strcmp((*pp)->file, file) == 0) {
            DapBreakpoint *old = *pp;
            *pp = old->next;
            free(old);
            dbg->bp_count--;
        } else {
            pp = &(*pp)->next;
        }
    }
    
    /* 设置新断点 */
    int bp_id = -1;
    if (line > 0) {
        bp_id = dap_set_breakpoint(dbg, file, line);
    }
    
    char body[512];
    if (bp_id > 0) {
        snprintf(body, sizeof(body),
            "{\"breakpoints\":[{\"id\":%d,\"verified\":true,\"line\":%d}]}",
            bp_id, line);
    } else {
        snprintf(body, sizeof(body),
            "{\"breakpoints\":[{\"verified\":false,\"line\":%d}]}", line);
    }
    
    dap_send_response(dbg, seq, "setBreakpoints", 1, body);
}

static void dap_handle_configuration_done(DapDebugger *dbg, int seq) {
    dap_send_response(dbg, seq, "configurationDone", 1, NULL);
}

static void dap_handle_continue(DapDebugger *dbg, int seq) {
    dbg->paused = 0;
    dbg->step_mode = 0;
    dap_send_response(dbg, seq, "continue", 1, "{\"allThreadsContinued\":true}");
    dap_send_event(dbg, "continued", "{\"threadId\":1}");
}

static void dap_handle_next(DapDebugger *dbg, int seq) {
    dbg->step_mode = 1;
    dbg->paused = 0;
    dap_send_response(dbg, seq, "next", 1, NULL);
}

static void dap_handle_step_in(DapDebugger *dbg, int seq) {
    dbg->step_mode = 2;
    dbg->paused = 0;
    dap_send_response(dbg, seq, "stepIn", 1, NULL);
}

static void dap_handle_step_out(DapDebugger *dbg, int seq) {
    dbg->step_mode = 3;
    dbg->paused = 0;
    dap_send_response(dbg, seq, "stepOut", 1, NULL);
}

static void dap_handle_stack_trace(DapDebugger *dbg, int seq) {
    char body[4096];
    int offset = 0;
    
    offset += snprintf(body + offset, sizeof(body) - offset,
        "{\"stackFrames\":[");
    
    DapStackFrame *sf = dbg->stack_frames;
    int first = 1;
    while (sf && offset < (int)sizeof(body) - 256) {
        if (!first) body[offset++] = ',';
        offset += snprintf(body + offset, sizeof(body) - offset,
            "{\"id\":%d,\"name\":\"%s\",\"source\":{\"path\":\"%s\"},\"line\":%d,\"column\":%d}",
            sf->id, sf->name, sf->file, sf->line, sf->column);
        first = 0;
        sf = sf->next;
    }
    
    snprintf(body + offset, sizeof(body) - offset, "],\"totalFrames\":%d}", dbg->frame_count);
    dap_send_response(dbg, seq, "stackTrace", 1, body);
}

static void dap_handle_variables(DapDebugger *dbg, int seq, const char *args) {
    int var_ref = json_get_int(args, "variablesReference", 0);
    char body[4096];
    int offset = 0;
    
    offset += snprintf(body + offset, sizeof(body) - offset, "{\"variables\":[");
    
    DapVariable *var = dbg->variables;
    int first = 1;
    while (var && offset < (int)sizeof(body) - 256) {
        if (var->variables_reference == var_ref || var_ref == 0) {
            if (!first) body[offset++] = ',';
            offset += snprintf(body + offset, sizeof(body) - offset,
                "{\"name\":\"%s\",\"value\":\"%s\",\"type\":\"%s\",\"variablesReference\":%d}",
                var->name, var->value, var->type, var->variables_reference);
            first = 0;
        }
        var = var->next;
    }
    
    snprintf(body + offset, sizeof(body) - offset, "]}");
    dap_send_response(dbg, seq, "variables", 1, body);
}

static void dap_handle_threads(DapDebugger *dbg, int seq) {
    const char *body = "{\"threads\":[{\"id\":1,\"name\":\"main\"}]}";
    dap_send_response(dbg, seq, "threads", 1, body);
}

static void dap_handle_disconnect(DapDebugger *dbg, int seq) {
    dbg->state = DAP_STATE_TERMINATED;
    dap_send_response(dbg, seq, "disconnect", 1, NULL);
    dap_send_event(dbg, "terminated", NULL);
}

/* Pony++ 扩展: 获取 Actor 列表 */
static void dap_handle_actors(DapDebugger *dbg, int seq) {
    char body[4096];
    int offset = 0;
    
    offset += snprintf(body + offset, sizeof(body) - offset, "{\"actors\":[");
    
    for (int i = 0; i < dbg->actor_count && offset < (int)sizeof(body) - 256; i++) {
        if (i > 0) body[offset++] = ',';
        offset += snprintf(body + offset, sizeof(body) - offset,
            "{\"id\":%d,\"name\":\"%s\",\"state\":%d,\"messageCount\":%zu}",
            dbg->actors[i].id, dbg->actors[i].name, 
            dbg->actors[i].state, dbg->actors[i].message_count);
    }
    
    snprintf(body + offset, sizeof(body) - offset, "]}");
    dap_send_response(dbg, seq, "actors", 1, body);
}

/* Pony++ 扩展: 获取消息队列 */
static void dap_handle_messages(DapDebugger *dbg, int seq, const char *args) {
    int actor_id = json_get_int(args, "actorId", -1);
    char body[4096];
    int offset = 0;
    
    offset += snprintf(body + offset, sizeof(body) - offset, "{\"messages\":[");
    
    int first = 1;
    for (int i = 0; i < dbg->message_count && offset < (int)sizeof(body) - 256; i++) {
        if (actor_id >= 0 && dbg->messages[i].receiver_id != actor_id) continue;
        if (!first) body[offset++] = ',';
        offset += snprintf(body + offset, sizeof(body) - offset,
            "{\"msgId\":%d,\"from\":%d,\"to\":%d,\"method\":\"%s\",\"argSize\":%zu}",
            dbg->messages[i].msg_id, dbg->messages[i].sender_id,
            dbg->messages[i].receiver_id, dbg->messages[i].method,
            dbg->messages[i].arg_size);
        first = 0;
    }
    
    snprintf(body + offset, sizeof(body) - offset, "]}");
    dap_send_response(dbg, seq, "messages", 1, body);
}

/* ======================== 消息分发 ======================== */

int dap_process_message(DapDebugger *dbg) {
    char buf[8192];
    int len = dap_read_message(dbg, buf, sizeof(buf));
    if (len <= 0) return -1;
    
    /* 解析命令 */
    char command[128];
    if (!json_get_string(buf, "command", command, sizeof(command))) return -1;
    
    int seq = json_get_int(buf, "seq", 0);
    
    /* 提取 arguments 子对象 */
    const char *args = strstr(buf, "\"arguments\"");
    if (!args) args = "{}";
    
    /* 分发命令 */
    if (strcmp(command, "initialize") == 0) {
        dap_handle_initialize(dbg, seq, args);
    } else if (strcmp(command, "launch") == 0) {
        dap_handle_launch(dbg, seq, args);
    } else if (strcmp(command, "setBreakpoints") == 0) {
        dap_handle_set_breakpoints(dbg, seq, args);
    } else if (strcmp(command, "configurationDone") == 0) {
        dap_handle_configuration_done(dbg, seq);
    } else if (strcmp(command, "continue") == 0) {
        dap_handle_continue(dbg, seq);
    } else if (strcmp(command, "next") == 0) {
        dap_handle_next(dbg, seq);
    } else if (strcmp(command, "stepIn") == 0) {
        dap_handle_step_in(dbg, seq);
    } else if (strcmp(command, "stepOut") == 0) {
        dap_handle_step_out(dbg, seq);
    } else if (strcmp(command, "stackTrace") == 0) {
        dap_handle_stack_trace(dbg, seq);
    } else if (strcmp(command, "variables") == 0) {
        dap_handle_variables(dbg, seq, args);
    } else if (strcmp(command, "threads") == 0) {
        dap_handle_threads(dbg, seq);
    } else if (strcmp(command, "disconnect") == 0 || strcmp(command, "terminate") == 0) {
        dap_handle_disconnect(dbg, seq);
    } else if (strcmp(command, "actors") == 0) {
        dap_handle_actors(dbg, seq);
    } else if (strcmp(command, "messages") == 0) {
        dap_handle_messages(dbg, seq, args);
    } else {
        /* 未知命令 */
        dap_send_response(dbg, seq, command, 0, "{\"error\":\"unsupported command\"}");
    }
    
    return 0;
}

int dap_run(DapDebugger *dbg) {
    if (!dbg) return -1;
    
    while (dbg->state != DAP_STATE_TERMINATED) {
        if (dap_process_message(dbg) != 0) break;
    }
    
    return 0;
}

/* ======================== 断点管理 ======================== */

int dap_set_breakpoint(DapDebugger *dbg, const char *file, int line) {
    if (!dbg || !file || line <= 0) return -1;
    
    DapBreakpoint *bp = (DapBreakpoint *)calloc(1, sizeof(DapBreakpoint));
    if (!bp) return -1;
    
    bp->id = dbg->next_bp_id++;
    bp->kind = DAP_BP_LINE;
    strncpy(bp->file, file, sizeof(bp->file) - 1);
    bp->line = line;
    bp->enabled = 1;
    bp->hit_count = 0;
    
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    dbg->bp_count++;
    
    return bp->id;
}

int dap_set_function_breakpoint(DapDebugger *dbg, const char *function) {
    if (!dbg || !function) return -1;
    
    DapBreakpoint *bp = (DapBreakpoint *)calloc(1, sizeof(DapBreakpoint));
    if (!bp) return -1;
    
    bp->id = dbg->next_bp_id++;
    bp->kind = DAP_BP_FUNCTION;
    strncpy(bp->function, function, sizeof(bp->function) - 1);
    bp->enabled = 1;
    
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    dbg->bp_count++;
    
    return bp->id;
}

int dap_set_actor_breakpoint(DapDebugger *dbg, int actor_id) {
    if (!dbg || actor_id < 0) return -1;
    
    DapBreakpoint *bp = (DapBreakpoint *)calloc(1, sizeof(DapBreakpoint));
    if (!bp) return -1;
    
    bp->id = dbg->next_bp_id++;
    bp->kind = DAP_BP_ACTOR;
    bp->actor_id = actor_id;
    bp->enabled = 1;
    
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    dbg->bp_count++;
    
    return bp->id;
}

int dap_set_message_breakpoint(DapDebugger *dbg, const char *method) {
    if (!dbg || !method) return -1;
    
    DapBreakpoint *bp = (DapBreakpoint *)calloc(1, sizeof(DapBreakpoint));
    if (!bp) return -1;
    
    bp->id = dbg->next_bp_id++;
    bp->kind = DAP_BP_MESSAGE;
    strncpy(bp->method, method, sizeof(bp->method) - 1);
    bp->enabled = 1;
    
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    dbg->bp_count++;
    
    return bp->id;
}

int dap_remove_breakpoint(DapDebugger *dbg, int id) {
    if (!dbg) return -1;
    
    DapBreakpoint **pp = &dbg->breakpoints;
    while (*pp) {
        if ((*pp)->id == id) {
            DapBreakpoint *old = *pp;
            *pp = old->next;
            free(old);
            dbg->bp_count--;
            return 0;
        }
        pp = &(*pp)->next;
    }
    
    return -1;
}

int dap_enable_breakpoint(DapDebugger *dbg, int id, int enabled) {
    if (!dbg) return -1;
    
    DapBreakpoint *bp = dbg->breakpoints;
    while (bp) {
        if (bp->id == id) {
            bp->enabled = enabled;
            return 0;
        }
        bp = bp->next;
    }
    
    return -1;
}

int dap_check_breakpoint(DapDebugger *dbg, const char *file, int line) {
    if (!dbg || !file) return 0;
    
    DapBreakpoint *bp = dbg->breakpoints;
    while (bp) {
        if (bp->enabled && bp->kind == DAP_BP_LINE &&
            bp->line == line && strcmp(bp->file, file) == 0) {
            bp->hit_count++;
            return 1;
        }
        bp = bp->next;
    }
    
    return 0;
}

int dap_check_actor_breakpoint(DapDebugger *dbg, int actor_id) {
    if (!dbg) return 0;
    
    DapBreakpoint *bp = dbg->breakpoints;
    while (bp) {
        if (bp->enabled && bp->kind == DAP_BP_ACTOR && bp->actor_id == actor_id) {
            bp->hit_count++;
            return 1;
        }
        bp = bp->next;
    }
    
    return 0;
}

int dap_check_message_breakpoint(DapDebugger *dbg, const char *method) {
    if (!dbg || !method) return 0;
    
    DapBreakpoint *bp = dbg->breakpoints;
    while (bp) {
        if (bp->enabled && bp->kind == DAP_BP_MESSAGE &&
            strcmp(bp->method, method) == 0) {
            bp->hit_count++;
            return 1;
        }
        bp = bp->next;
    }
    
    return 0;
}

/* ======================== 执行控制 ======================== */

int dap_continue(DapDebugger *dbg) {
    if (!dbg) return -1;
    dbg->paused = 0;
    dbg->step_mode = 0;
    return 0;
}

int dap_step_over(DapDebugger *dbg) {
    if (!dbg) return -1;
    dbg->step_mode = 1;
    dbg->paused = 0;
    return 0;
}

int dap_step_into(DapDebugger *dbg) {
    if (!dbg) return -1;
    dbg->step_mode = 2;
    dbg->paused = 0;
    return 0;
}

int dap_step_out(DapDebugger *dbg) {
    if (!dbg) return -1;
    dbg->step_mode = 3;
    dbg->paused = 0;
    return 0;
}

int dap_pause(DapDebugger *dbg) {
    if (!dbg) return -1;
    dbg->paused = 1;
    dbg->step_mode = 0;
    return 0;
}

/* ======================== 状态检查 ======================== */

DapStackFrame *dap_get_stack_trace(DapDebugger *dbg, int *count) {
    if (!dbg) { if (count) *count = 0; return NULL; }
    if (count) *count = dbg->frame_count;
    return dbg->stack_frames;
}

DapVariable *dap_get_variables(DapDebugger *dbg, int variables_reference, int *count) {
    if (!dbg) { if (count) *count = 0; return NULL; }
    if (count) *count = dbg->var_count;
    (void)variables_reference;
    return dbg->variables;
}

DapActorInfo *dap_get_actors(DapDebugger *dbg, int *count) {
    if (!dbg) { if (count) *count = 0; return NULL; }
    if (count) *count = dbg->actor_count;
    return dbg->actors;
}

DapMessageInfo *dap_get_messages(DapDebugger *dbg, int actor_id, int *count) {
    if (!dbg) { if (count) *count = 0; return NULL; }
    (void)actor_id;
    if (count) *count = dbg->message_count;
    return dbg->messages;
}

/* ======================== 运行时集成 ======================== */

void dap_set_on_breakpoint(DapDebugger *dbg, DapOnBreakpoint cb) {
    if (dbg) dbg->on_breakpoint = cb;
}

void dap_set_on_actor_created(DapDebugger *dbg, DapOnActorCreated cb) {
    if (dbg) dbg->on_actor_created = cb;
}

void dap_set_on_message_sent(DapDebugger *dbg, DapOnMessageSent cb) {
    if (dbg) dbg->on_message_sent = cb;
}

void dap_notify_breakpoint(DapDebugger *dbg, const char *file, int line) {
    if (!dbg || dbg->state != DAP_STATE_RUNNING) return;
    
    if (dap_check_breakpoint(dbg, file, line)) {
        dbg->paused = 1;
        dbg->state = DAP_STATE_STOPPED;
        
        char body[512];
        snprintf(body, sizeof(body),
            "{\"reason\":\"breakpoint\",\"threadId\":1,\"source\":{\"path\":\"%s\"},\"line\":%d}",
            file, line);
        dap_send_event(dbg, "stopped", body);
        
        if (dbg->on_breakpoint) {
            DapBreakpoint *bp = dbg->breakpoints;
            while (bp) {
                if (bp->enabled && bp->kind == DAP_BP_LINE &&
                    bp->line == line && strcmp(bp->file, file) == 0) {
                    dbg->on_breakpoint(dbg, bp);
                    break;
                }
                bp = bp->next;
            }
        }
    }
}

void dap_notify_actor_created(DapDebugger *dbg, int actor_id, const char *name) {
    if (!dbg) return;
    
    /* 扩展 Actor 数组 */
    DapActorInfo *new_actors = (DapActorInfo *)realloc(dbg->actors,
        (dbg->actor_count + 1) * sizeof(DapActorInfo));
    if (!new_actors) return;
    
    dbg->actors = new_actors;
    DapActorInfo *info = &dbg->actors[dbg->actor_count];
    memset(info, 0, sizeof(DapActorInfo));
    info->id = actor_id;
    strncpy(info->name, name ? name : "", sizeof(info->name) - 1);
    info->state = 0;
    info->message_count = 0;
    info->mailbox_size = 0;
    dbg->actor_count++;
    
    if (dbg->on_actor_created) {
        dbg->on_actor_created(dbg, actor_id, name);
    }
    
    /* 发送 Actor 事件 */
    char body[256];
    snprintf(body, sizeof(body),
        "{\"actorId\":%d,\"name\":\"%s\"}", actor_id, name ? name : "");
    dap_send_event(dbg, "actor", body);
}

void dap_notify_message(DapDebugger *dbg, int from_id, int to_id, const char *method) {
    if (!dbg) return;
    
    /* 扩展消息数组 */
    DapMessageInfo *new_msgs = (DapMessageInfo *)realloc(dbg->messages,
        (dbg->message_count + 1) * sizeof(DapMessageInfo));
    if (!new_msgs) return;
    
    dbg->messages = new_msgs;
    DapMessageInfo *info = &dbg->messages[dbg->message_count];
    memset(info, 0, sizeof(DapMessageInfo));
    info->msg_id = dbg->message_count + 1;
    info->sender_id = from_id;
    info->receiver_id = to_id;
    strncpy(info->method, method ? method : "", sizeof(info->method) - 1);
    info->arg_size = 0;
    dbg->message_count++;
    
    if (dbg->on_message_sent) {
        dbg->on_message_sent(dbg, from_id, to_id, method);
    }
    
    /* 检查消息断点 */
    if (dap_check_message_breakpoint(dbg, method)) {
        dbg->paused = 1;
        dbg->state = DAP_STATE_STOPPED;
        
        char body[512];
        snprintf(body, sizeof(body),
            "{\"reason\":\"breakpoint\",\"threadId\":1,\"method\":\"%s\",\"from\":%d,\"to\":%d}",
            method, from_id, to_id);
        dap_send_event(dbg, "stopped", body);
    }
}

/* ======================== 工具函数 ======================== */

DapState dap_get_state(DapDebugger *dbg) {
    return dbg ? dbg->state : DAP_STATE_TERMINATED;
}

int dap_get_breakpoint_count(DapDebugger *dbg) {
    return dbg ? dbg->bp_count : 0;
}

DapBreakpoint *dap_list_breakpoints(DapDebugger *dbg) {
    return dbg ? dbg->breakpoints : NULL;
}
