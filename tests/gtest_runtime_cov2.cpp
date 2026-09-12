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

/* ==================== Actor registration ==================== */

TEST(RuntimeCov2, ActorNew) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        PnyActor *a = pny_actor_new(rt, "Worker", 0);
        EXPECT_NE(a, nullptr);
        pny_runtime_free(rt);
    }
}


/* ==================== Message creation ==================== */

TEST(RuntimeCov2, MessageNew) {
    PnyMessage *m = pny_msg_new("process", nullptr, 0);
    EXPECT_NE(m, nullptr);
    pny_msg_free(m);
}


/* ==================== Message serialization ==================== */



/* ==================== Memory allocation ==================== */


/* ==================== String operations ==================== */


/* ==================== GC ==================== */

TEST(RuntimeCov2, GCRun) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        pny_actor_gc_collect(nullptr);
        pny_runtime_free(rt);
    }
}

/* ==================== Scheduler ==================== */

TEST(RuntimeCov2, SchedulerRun) {
    PnyRuntime *rt = pny_runtime_new();
    if (rt) {
        pny_scheduler_tick(rt);
        pny_runtime_free(rt);
    }
}

/* ==================== Actor lookup ==================== */



/* ==================== Actor removal ==================== */

