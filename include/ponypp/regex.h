/*
 * Pony++ Regex Module
 *
 * POSIX正则表达式封装 (regcomp/regexec)
 * Linux: glibc内置 | MCU: 可选编译(PONY_NO_REGEX时stub)
 *
 * API:
 * - 编译/匹配/捕获组/替换/全局搜索
 */

#ifndef PONYPP_REGEX_H
#define PONYPP_REGEX_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PNY_REGEX_OK          0
#define PNY_REGEX_ERR        -1
#define PNY_REGEX_NO_LIB     -2   /* regex未编译 (MCU) */
#define PNY_REGEX_BAD_ARG    -3
#define PNY_REGEX_SYNTAX     -4   /* 正则语法错误 */
#define PNY_REGEX_NOMATCH    -5   /* 无匹配 */
#define PNY_REGEX_MAX_GROUPS  10  /* 最大捕获组数 */

/* 编译选项 */
#define PNY_REGEX_ICASE      0x01  /* 忽略大小写 */
#define PNY_REGEX_NOSUB      0x02  /* 不关心捕获组(更快) */

/* 匹配结果: 捕获组偏移 */
typedef struct {
    int so;   /* 起始偏移 (-1表示未使用) */
    int eo;   /* 结束偏移 */
} PnyRegexMatch;

/* 编译后的正则 */
typedef struct PnyRegex PnyRegex;

/* 编译正则表达式 */
PnyRegex *pny_regex_compile(const char *pattern, int flags);

/* 释放 */
void pny_regex_free(PnyRegex *re);

/* 获取语法错误信息 */
const char *pny_regex_error(const PnyRegex *re);

/* 匹配: 返回匹配数(>=0)或错误码(<0)
 * matches数组至少PNY_REGEX_MAX_GROUPS个元素
 * matches[0]=整体匹配, matches[1..9]=捕获组 */
int pny_regex_match(const PnyRegex *re, const char *str, size_t len,
                    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS]);

/* 简单判断是否匹配 */
bool pny_regex_is_match(const PnyRegex *re, const char *str, size_t len);

/* 替换: 第一个匹配 (repl支持$1-$9反向引用)
 * out需预分配, out_len为缓冲区大小, 返回写入长度或错误码 */
int pny_regex_replace_first(const PnyRegex *re, const char *str, size_t len,
                            const char *repl, char *out, size_t out_len);

/* 全局替换 */
int pny_regex_replace_all(const PnyRegex *re, const char *str, size_t len,
                          const char *repl, char *out, size_t out_len);

/* 便捷函数: 一次性编译+匹配+释放 */
int pny_regex_match_str(const char *pattern, int flags, const char *str, size_t len,
                        PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS]);

#ifdef __cplusplus
}
#endif

#endif /* PONYPP_REGEX_H */
