#include <gtest/gtest.h>
#include <ponypp/distributed.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ==================== Remote Actor Registry ==================== */




/* ==================== Message Serialization ==================== */


/* ==================== Supervisor Strategies ==================== */

TEST(DistCov7, SupervisorOneForAll) {
    DistSupervisor *sup = dist_supervisor_new("sup1", DIST_SUPERVISE_ONE_FOR_ALL, 3);
    ASSERT_TRUE(sup != nullptr);
    dist_supervisor_free(sup);
}

TEST(DistCov7, SupervisorRestForOne) {
    DistSupervisor *sup = dist_supervisor_new("sup2", DIST_SUPERVISE_REST_FOR_ONE, 3);
    ASSERT_TRUE(sup != nullptr);
    dist_supervisor_free(sup);
}

/* ==================== Heartbeat ==================== */



/* ==================== Connection Errors ==================== */

TEST(DistCov7, ConnectInvalidPort) {
    DistConnection *conn = dist_conn_connect("127.0.0.1", 0);
    EXPECT_TRUE(conn == nullptr);
}

TEST(DistCov7, ConnectInvalidHost) {
    DistConnection *conn = dist_conn_connect("invalid.host", 9000);
    EXPECT_TRUE(conn == nullptr);
}

/* ==================== Message Queue ==================== */





/* ==================== Node Discovery ==================== */


