/*
 * DWARF + Source Map 测试
 */

#include <gtest/gtest.h>
#include <ponypp/dwarf.h>
#include <ponypp/codegen.h>
#include <cstdio>
#include <cstring>

/* ==================== DWARF Line Program ==================== */

TEST(Dwarf, NewFree) {
    DwarfLineProgram *dp = dwarf_line_new("/home/user", "test.pny");
    ASSERT_NE(dp, nullptr);
    EXPECT_EQ(dwarf_line_row_count(dp), 0u);
    EXPECT_EQ(dwarf_line_file_count(dp), 0u);
    dwarf_line_free(dp);
}

TEST(Dwarf, AddFiles) {
    DwarfLineProgram *dp = dwarf_line_new("/home/user", "main.pny");
    ASSERT_NE(dp, nullptr);

    int dir1 = dwarf_line_add_dir(dp, "/home/user/src");
    int dir2 = dwarf_line_add_dir(dp, "/home/user/lib");
    EXPECT_EQ(dir1, 1);
    EXPECT_EQ(dir2, 2);

    int f1 = dwarf_line_add_file(dp, "main.pny", 0);
    int f2 = dwarf_line_add_file(dp, "utils.pny", 1);
    int f3 = dwarf_line_add_file(dp, "actor.pny", 2);
    EXPECT_EQ(f1, 1);
    EXPECT_EQ(f2, 2);
    EXPECT_EQ(f3, 3);
    EXPECT_EQ(dwarf_line_file_count(dp), 3u);
    dwarf_line_free(dp);
}

TEST(Dwarf, EmitRows) {
    DwarfLineProgram *dp = dwarf_line_new("/home/user", "test.pny");
    ASSERT_NE(dp, nullptr);

    dwarf_line_add_dir(dp, "/home/user");
    dwarf_line_add_file(dp, "test.pny", 0);

    /* 模拟3行代码 */
    dwarf_line_set_addr(dp, 0x1000);
    dwarf_line_set_file(dp, 1);
    dwarf_line_set_line(dp, 1);
    dwarf_line_set_column(dp, 1);
    dwarf_line_emit_row(dp);

    dwarf_line_set_addr(dp, 0x1010);
    dwarf_line_set_line(dp, 5);
    dwarf_line_emit_row(dp);

    dwarf_line_set_addr(dp, 0x1020);
    dwarf_line_set_line(dp, 10);
    dwarf_line_emit_row(dp);

    dwarf_line_end_sequence(dp);

    EXPECT_EQ(dwarf_line_row_count(dp), 4u);  /* 3行 + end_sequence */
    dwarf_line_free(dp);
}

TEST(Dwarf, GenerateBytes) {
    DwarfLineProgram *dp = dwarf_line_new("/home/user", "test.pny");
    ASSERT_NE(dp, nullptr);

    dwarf_line_add_dir(dp, "/home/user");
    dwarf_line_add_file(dp, "test.pny", 0);

    dwarf_line_set_addr(dp, 0x1000);
    dwarf_line_set_file(dp, 1);
    dwarf_line_set_line(dp, 1);
    dwarf_line_set_line(dp, 5);
    dwarf_line_emit_row(dp);

    dwarf_line_end_sequence(dp);

    uint8_t buf[4096];
    size_t len = 0;
    int rc = dwarf_line_generate(dp, buf, sizeof(buf), &len);
    EXPECT_EQ(rc, 0);
    EXPECT_GT(len, 0u);

    /* 验证 unit_length */
    uint32_t unit_length = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    EXPECT_EQ(unit_length, len - 4);

    /* 验证 version = 5 */
    EXPECT_EQ(buf[4], 5);
    EXPECT_EQ(buf[5], 0);

    /* 验证 address_size = 8 */
    EXPECT_EQ(buf[6], 8);

    dwarf_line_free(dp);
}

TEST(Dwarf, SaveToFile) {
    DwarfLineProgram *dp = dwarf_line_new("/home/user", "test.pny");
    ASSERT_NE(dp, nullptr);

    dwarf_line_add_dir(dp, "/home/user");
    dwarf_line_add_file(dp, "test.pny", 0);

    dwarf_line_set_addr(dp, 0x1000);
    dwarf_line_set_file(dp, 1);
    dwarf_line_set_line(dp, 1);
    dwarf_line_emit_row(dp);

    dwarf_line_set_addr(dp, 0x1020);
    dwarf_line_set_line(dp, 42);
    dwarf_line_emit_row(dp);

    dwarf_line_end_sequence(dp);

    int rc = dwarf_line_save(dp, "/tmp/test.debug_line");
    EXPECT_EQ(rc, 0);

    /* 验证文件非空 */
    FILE *f = fopen("/tmp/test.debug_line", "rb");
    ASSERT_NE(f, nullptr);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fclose(f);
    EXPECT_GT(fsize, 20);

    dwarf_line_free(dp);
    remove("/tmp/test.debug_line");
}

/* ==================== Source Map ==================== */

TEST(SourceMap, NewFree) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);
    EXPECT_EQ(sourcemap_count(sm), 0u);
    sourcemap_free(sm);
}

TEST(SourceMap, AddLookup) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);

    sourcemap_add(sm, 1, "main.pny", 1, 1);
    sourcemap_add(sm, 10, "main.pny", 5, 1);
    sourcemap_add(sm, 20, "main.pny", 10, 1);
    EXPECT_EQ(sourcemap_count(sm), 3u);

    /* 查找: 最近的<=gen_line */
    SourceMapEntry e;
    EXPECT_EQ(sourcemap_lookup(sm, 1, &e), 0);
    EXPECT_EQ(e.source_line, 1);

    EXPECT_EQ(sourcemap_lookup(sm, 15, &e), 0);
    EXPECT_EQ(e.source_line, 5);

    EXPECT_EQ(sourcemap_lookup(sm, 25, &e), 0);
    EXPECT_EQ(e.source_line, 10);

    /* gen_line < 第一条 */
    EXPECT_NE(sourcemap_lookup(sm, 0, &e), 0);

    sourcemap_free(sm);
}

TEST(SourceMap, SaveLoad) {
    SourceMap *sm = sourcemap_new();
    ASSERT_NE(sm, nullptr);

    sourcemap_add(sm, 1, "test.pny", 1, 0);
    sourcemap_add(sm, 50, "test.pny", 25, 0);
    sourcemap_add(sm, 100, "test.pny", 50, 0);

    /* 保存JSON */
    int rc = sourcemap_save_json(sm, "/tmp/test.map");
    EXPECT_EQ(rc, 0);

    /* 重新加载 */
    SourceMap *loaded = sourcemap_load_json("/tmp/test.map");
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(sourcemap_count(loaded), 3u);

    sourcemap_free(sm);
    sourcemap_free(loaded);
    remove("/tmp/test.map");
}
