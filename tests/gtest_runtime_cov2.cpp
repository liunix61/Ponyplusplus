#include <gtest/gtest.h>
#include <ponypp/runtime.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>

/* ==================== Runtime lifecycle ==================== */

TEST(RuntimeCov2, RuntimeNewFree) {
    PnyRuntime *rt = pny_runtime_new();
    EXPECT_NE(rt, nullptr);
    pny_runtime_free(rt);
}

TEST(RuntimeCov2, RuntimeNewNull) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        pny_runtime_free(rt);
    }
}

/* ==================== Actor registration ==================== */

TEST(RuntimeCov2, ActorRegister) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        EXPECT_NE(a, nullptr);
        pny_actor_register(rt, a);
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, ActorRegisterNull) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        pny_actor_register(rt, nullptr);
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, ActorRegisterNullRuntime) {
    PnyActor *a = nullptr;
    pny_actor_register(nullptr, a);
}

/* ==================== Message sending ==================== */

TEST(RuntimeCov2, MessageSend) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            int result = pny_send(rt, a, "process", nullptr, 0);
            EXPECT_EQ(result, 0);
        }
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, MessageSendNullActor) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        int result = pny_send(rt, nullptr, "process", nullptr, 0);
        EXPECT_NE(result, 0);
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, MessageSendNullRuntime) {
    int result = pny_send(nullptr, nullptr, "process", nullptr, 0);
    EXPECT_NE(result, 0);
}

/* ==================== Message pool ==================== */

TEST(RuntimeCov2, MessagePoolReuse) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            // Send multiple messages to trigger pool reuse
            for (int i = 0; i < 100; i++) {
                pny_send(rt, a, "process", nullptr, 0);
            }
        }
        pny_runtime_free(rt);
    }
}

/* ==================== Actor behavior ==================== */

TEST(RuntimeCov2, ActorBehavior) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            pny_actor_add_behavior(a, "process", nullptr, nullptr);
            pny_send(rt, a, "process", nullptr, 0);
        }
        pny_runtime_free(rt);
    }
}

/* ==================== Scheduler ==================== */

TEST(RuntimeCov2, SchedulerRun) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            pny_send(rt, a, "process", nullptr, 0);
            pny_scheduler_run(rt, 1);
        }
        pny_runtime_free(rt);
    }
}

/* ==================== GC ==================== */

TEST(RuntimeCov2, GCRun) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        pny_gc_run(rt);
        pny_runtime_free(rt);
    }
}

/* ==================== Memory allocation ==================== */

TEST(RuntimeCov2, MemoryAlloc) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        void *ptr = pny_alloc(rt, 1024);
        EXPECT_NE(ptr, nullptr);
        pny_free(rt, ptr);
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, MemoryAllocZero) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        void *ptr = pny_alloc(rt, 0);
        // May return nullptr or non-null depending on implementation
        if (ptr) pny_free(rt, ptr);
        pny_runtime_free(rt);
    }
}

/* ==================== String operations ==================== */

TEST(RuntimeCov2, StringAlloc) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        char *s = pny_string_alloc(rt, "hello");
        EXPECT_NE(s, nullptr);
        EXPECT_STREQ(s, "hello");
        pny_string_free(rt, s);
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, StringAllocNull) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        char *s = pny_string_alloc(rt, nullptr);
        EXPECT_EQ(s, nullptr);
        pny_runtime_free(rt);
    }
}

/* ==================== Actor lookup ==================== */

TEST(RuntimeCov2, ActorFind) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            PnyActor *found = pny_actor_find(rt, "Worker");
            EXPECT_EQ(found, a);
        }
        pny_runtime_free(rt);
    }
}

TEST(RuntimeCov2, ActorFindNonexistent) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *found = pny_actor_find(rt, "Nonexistent");
        EXPECT_EQ(found, nullptr);
        pny_runtime_free(rt);
    }
}

/* ==================== Actor removal ==================== */

TEST(RuntimeCov2, ActorRemove) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            pny_actor_remove(rt, a);
            PnyActor *found = pny_actor_find(rt, "Worker");
            EXPECT_EQ(found, nullptr);
        }
        pny_runtime_free(rt);
    }
}

/* ==================== Message count ==================== */

TEST(RuntimeCov2, MessageCount) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker");
        if (a) {
            EXPECT_EQ(pny_actor_message_count(a), 0);
            pny_send(rt, a, "process", nullptr, 0);
            EXPECT_EQ(pny_actor_message_count(a), 1);
        }
        pny_runtime_free(rt);
    }
}
