#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* ==================== Connection error handling ==================== */

TEST(DistCov8, ConnectToInvalidPort) {
    DistConnection *conn = dist_conn_connect("127.0.0.1", 1);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov8, ConnectToInvalidHost) {
    DistConnection *conn = dist_conn_connect("invalid.host.example.com", 8080);
    EXPECT_EQ(conn, nullptr);
}

TEST(DistCov8, ListenOnInvalidPort) {
    DistConnection *conn = dist_conn_listen(1);
    EXPECT_EQ(conn, nullptr);
}

/* ==================== Connection lifecycle ==================== */

TEST(DistCov8, ConnectionFreeNull) {
    dist_conn_free(nullptr);
}

TEST(DistCov8, ConnectionSendNull) {
    int result = dist_send(nullptr, "test", 4);
    EXPECT_NE(result, 0);
}

TEST(DistCov8, ConnectionRecvNull) {
    char buf[256];
    int result = dist_recv(nullptr, buf, sizeof(buf));
    EXPECT_NE(result, 0);
}

/* ==================== Runtime lifecycle ==================== */

TEST(DistCov8, RuntimeNewFree) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 0);
        if (dr) {
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}

TEST(DistCov8, RuntimeListenInvalidPort) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 1);
        if (dr) {
            int result = dist_runtime_listen(dr);
            EXPECT_NE(result, 0);
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}

/* ==================== Supervisor strategies ==================== */

TEST(DistCov8, SupervisorRestart) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) {
        dist_supervisor_free(sup);
    }
}

TEST(DistCov8, SupervisorStop) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_ONE_FOR_ALL, 3);
    if (sup) {
        dist_supervisor_free(sup);
    }
}

TEST(DistCov8, SupervisorEscalate) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_REST_FOR_ONE, 3);
    if (sup) {
        dist_supervisor_free(sup);
    }
}

/* ==================== Supervisor registration ==================== */

TEST(DistCov8, SupervisorRegister) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) {
        int result = dist_supervisor_register(sup, "Worker", "127.0.0.1", 8080, 1);
        EXPECT_EQ(result, 0);
        dist_supervisor_free(sup);
    }
}

/* ==================== Supervisor heartbeat ==================== */

TEST(DistCov8, SupervisorHeartbeat) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) {
        dist_supervisor_register(sup, "Worker", "127.0.0.1", 8080, 1);
        int result = dist_supervisor_heartbeat(sup, "Worker");
        EXPECT_EQ(result, 0);
        dist_supervisor_free(sup);
    }
}

/* ==================== Supervisor crash notification ==================== */

TEST(DistCov8, SupervisorNotifyCrash) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) {
        dist_supervisor_register(sup, "Worker", "127.0.0.1", 8080, 1);
        int result = dist_supervisor_notify_crash(sup, "Worker");
        EXPECT_EQ(result, 0);
        dist_supervisor_free(sup);
    }
}

/* ==================== Supervisor actor state ==================== */

TEST(DistCov8, SupervisorActorState) {
    DistSupervisor *sup = dist_supervisor_new("test", DIST_SUPERVISE_ONE_FOR_ONE, 3);
    if (sup) {
        dist_supervisor_register(sup, "Worker", "127.0.0.1", 8080, 1);
        DistActorState state = dist_supervisor_actor_state(sup, "Worker");
        EXPECT_EQ(state, DIST_ACTOR_RUNNING);
        dist_supervisor_free(sup);
    }
}

/* ==================== TLS error handling ==================== */



/* ==================== Node ID ==================== */

TEST(DistCov8, RuntimeNodeId) {
    PnyRuntime *local = pny_runtime_new();
    if (local) {
        DistributedRuntime *dr = dist_runtime_new(local, 0);
        if (dr) {
            const char *node_id = dist_runtime_node_id(dr);
            EXPECT_NE(node_id, nullptr);
            dist_runtime_free(dr);
        }
        pny_runtime_free(local);
    }
}
