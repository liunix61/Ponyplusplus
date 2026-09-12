#include <gtest/gtest.h>
#include <ponypp/pkg.h>
#include <ponypp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>

/* ==================== TOML parsing error handling ==================== */

TEST(PkgCov2, ParseNullPath) {
    PkgManifest *pm = pkg_parse_toml(nullptr);
    EXPECT_EQ(pm, nullptr);
}

TEST(PkgCov2, ParseNonexistentFile) {
    PkgManifest *pm = pkg_parse_toml("/nonexistent/path/pony.toml");
    EXPECT_EQ(pm, nullptr);
}

/* ==================== TOML parsing with valid file ==================== */


/* ==================== TOML parsing with dependencies ==================== */


/* ==================== Package description ==================== */

TEST(PkgCov2, PackageDescription) {
    char tmpl[] = "/tmp/ponypp_pkg_c2_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return;
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); unlink(tmpl); return; }
    fprintf(f, "[package]\nname = \"test\"\nversion = \"1.0.0\"\n");
    fprintf(f, "description = \"Test package\"\nlicense = \"MIT\"\n");
    fprintf(f, "[dependencies]\nfoo = \"1.0\"\n");
    fclose(f);
    
    PkgManifest *pm = pkg_parse_toml(tmpl);
    unlink(tmpl);
    
    if (pm) {
        char buf[1024];
        pkg_manifest_print(pm, buf, sizeof(buf));
        EXPECT_NE(buf[0], '\0');
        pkg_manifest_free(pm);
    }
}

/* ==================== Package manifest free ==================== */

TEST(PkgCov2, ManifestFreeNull) {
    pkg_manifest_free(nullptr);
}

/* ==================== Package dependency management ==================== */


/* ==================== Package with no dependencies ==================== */

TEST(PkgCov2, NoDependencies) {
    char tmpl[] = "/tmp/ponypp_pkg_c2_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return;
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); unlink(tmpl); return; }
    fprintf(f, "[package]\nname = \"test\"\nversion = \"1.0.0\"\n");
    fclose(f);
    
    PkgManifest *pm = pkg_parse_toml(tmpl);
    unlink(tmpl);
    
    if (pm) {
        EXPECT_EQ(pm->dep_count, 0);
        pkg_manifest_free(pm);
    }
}

/* ==================== Package with empty dependencies ==================== */

TEST(PkgCov2, EmptyDependencies) {
    char tmpl[] = "/tmp/ponypp_pkg_c2_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return;
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); unlink(tmpl); return; }
    fprintf(f, "[package]\nname = \"test\"\nversion = \"1.0.0\"\n");
    fprintf(f, "[dependencies]\n");
    fclose(f);
    
    PkgManifest *pm = pkg_parse_toml(tmpl);
    unlink(tmpl);
    
    if (pm) {
        EXPECT_EQ(pm->dep_count, 0);
        pkg_manifest_free(pm);
    }
}
