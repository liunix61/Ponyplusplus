/*
 * Pony++ Regex - POSIX regcomp/regexec implementation
 */

#include "ponypp/regex.h"

#ifndef PONY_NO_REGEX

#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct PnyRegex {
    regex_t re;
    bool compiled;
    char errbuf[128];
};

PnyRegex *pny_regex_compile(const char *pattern, int flags) {
    if (!pattern) return NULL;
    PnyRegex *r = (PnyRegex *)calloc(1, sizeof(PnyRegex));
    if (!r) return NULL;

    int cflags = REG_EXTENDED;
    if (flags & PNY_REGEX_ICASE) cflags |= REG_ICASE;
    if (flags & PNY_REGEX_NOSUB) cflags |= REG_NOSUB;

    int rc = regcomp(&r->re, pattern, cflags);
    if (rc != 0) {
        regerror(rc, &r->re, r->errbuf, sizeof(r->errbuf));
        regfree(&r->re);
        /* 保留errbuf但标记未编译 */
        return r;  /* compiled=false, 可通过pny_regex_error读取 */
    }
    r->compiled = true;
    return r;
}

void pny_regex_free(PnyRegex *re) {
    if (!re) return;
    if (re->compiled) regfree(&re->re);
    free(re);
}

const char *pny_regex_error(const PnyRegex *re) {
    if (!re) return "null regex";
    if (re->compiled) return NULL;
    return re->errbuf[0] ? re->errbuf : "compile failed";
}

int pny_regex_match(const PnyRegex *re, const char *str, size_t len,
                    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS]) {
    if (!re || !re->compiled || !str) return PNY_REGEX_BAD_ARG;

    /* POSIX regexec需要NUL结尾字符串 */
    char *tmp = (char *)malloc(len + 1);
    if (!tmp) return PNY_REGEX_ERR;
    memcpy(tmp, str, len);
    tmp[len] = '\0';

    regmatch_t rm[PNY_REGEX_MAX_GROUPS];
    int rc = regexec(&re->re, tmp, PNY_REGEX_MAX_GROUPS, rm, 0);
    if (rc == REG_NOMATCH) { free(tmp); return PNY_REGEX_NOMATCH; }
    if (rc != 0) { free(tmp); return PNY_REGEX_ERR; }

    if (matches) {
        for (int i = 0; i < PNY_REGEX_MAX_GROUPS; i++) {
            matches[i].so = (int)rm[i].rm_so;
            matches[i].eo = (int)rm[i].rm_eo;
        }
    }

    /* 统计匹配组数 */
    int n = 0;
    for (int i = 0; i < PNY_REGEX_MAX_GROUPS; i++) {
        if (rm[i].rm_so >= 0) n++;
    }

    free(tmp);
    return n;
}

bool pny_regex_is_match(const PnyRegex *re, const char *str, size_t len) {
    return pny_regex_match(re, str, len, NULL) > 0;
}

/* 内部: 带替换的匹配执行 */
static int regex_replace_impl(const PnyRegex *re, const char *str, size_t len,
                              const char *repl, char *out, size_t out_len,
                              bool global) {
    if (!re || !re->compiled || !str || !repl || !out) return PNY_REGEX_BAD_ARG;

    size_t out_pos = 0;
    size_t search_from = 0;
    int replace_count = 0;

    while (search_from <= len) {
        /* 在 str[search_from..] 上匹配 */
        regmatch_t rm[PNY_REGEX_MAX_GROUPS];
        const char *sub = str + search_from;
        size_t sub_len = len - search_from;

        char *tmp = (char *)malloc(sub_len + 1);
        if (!tmp) return PNY_REGEX_ERR;
        memcpy(tmp, sub, sub_len);
        tmp[sub_len] = '\0';

        int rc = regexec(&re->re, tmp, PNY_REGEX_MAX_GROUPS, rm, 0);
        if (rc == REG_NOMATCH) {
            /* 复制剩余部分 */
            size_t remain = len - search_from;
            if (out_pos + remain >= out_len) { free(tmp); return PNY_REGEX_ERR; }
            memcpy(out + out_pos, str + search_from, remain);
            out_pos += remain;
            free(tmp);
            break;
        }
        if (rc != 0) { free(tmp); return PNY_REGEX_ERR; }

        /* 复制匹配前的部分 */
        size_t match_start = search_from + (size_t)rm[0].rm_so;
        size_t match_end = search_from + (size_t)rm[0].rm_eo;

        size_t prefix_len = match_start - search_from;
        if (out_pos + prefix_len >= out_len) { free(tmp); return PNY_REGEX_ERR; }
        memcpy(out + out_pos, str + search_from, prefix_len);
        out_pos += prefix_len;

        /* 展开替换串 ($1-$9反向引用) */
        for (size_t i = 0; repl[i]; i++) {
            if (repl[i] == '$' && repl[i+1] >= '1' && repl[i+1] <= '9') {
                int gi = repl[i+1] - '0';
                if (gi < PNY_REGEX_MAX_GROUPS && rm[gi].rm_so >= 0) {
                    size_t gs = search_from + (size_t)rm[gi].rm_so;
                    size_t ge = search_from + (size_t)rm[gi].rm_eo;
                    size_t glen = ge - gs;
                    if (out_pos + glen >= out_len) { free(tmp); return PNY_REGEX_ERR; }
                    memcpy(out + out_pos, str + gs, glen);
                    out_pos += glen;
                }
                i++;  /* 跳过数字 */
            } else {
                if (out_pos + 1 >= out_len) { free(tmp); return PNY_REGEX_ERR; }
                out[out_pos++] = repl[i];
            }
        }

        replace_count++;
        free(tmp);

        if (!global) {
            /* 复制剩余 */
            size_t remain = len - match_end;
            if (out_pos + remain >= out_len) return PNY_REGEX_ERR;
            memcpy(out + out_pos, str + match_end, remain);
            out_pos += remain;
            break;
        }

        /* 防止零宽匹配死循环 */
        if (match_end == search_from) match_end++;
        search_from = match_end;
    }

    out[out_pos] = '\0';
    return replace_count;
}

int pny_regex_replace_first(const PnyRegex *re, const char *str, size_t len,
                            const char *repl, char *out, size_t out_len) {
    return regex_replace_impl(re, str, len, repl, out, out_len, false);
}

int pny_regex_replace_all(const PnyRegex *re, const char *str, size_t len,
                          const char *repl, char *out, size_t out_len) {
    return regex_replace_impl(re, str, len, repl, out, out_len, true);
}

int pny_regex_match_str(const char *pattern, int flags, const char *str, size_t len,
                        PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS]) {
    PnyRegex *re = pny_regex_compile(pattern, flags);
    if (!re) return PNY_REGEX_ERR;
    if (!re->compiled) { pny_regex_free(re); return PNY_REGEX_SYNTAX; }
    int n = pny_regex_match(re, str, len, matches);
    pny_regex_free(re);
    return n;
}

#else /* PONY_NO_REGEX: MCU stub */

struct PnyRegex { int dummy; };

PnyRegex *pny_regex_compile(const char *pattern, int flags) {
    (void)pattern; (void)flags; return NULL;
}
void pny_regex_free(PnyRegex *re) { (void)re; }
const char *pny_regex_error(const PnyRegex *re) { (void)re; return "regex disabled"; }
int pny_regex_match(const PnyRegex *re, const char *str, size_t len,
                    PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS]) {
    (void)re; (void)str; (void)len; (void)matches; return PNY_REGEX_NO_LIB;
}
bool pny_regex_is_match(const PnyRegex *re, const char *str, size_t len) {
    (void)re; (void)str; (void)len; return false;
}
int pny_regex_replace_first(const PnyRegex *re, const char *str, size_t len,
                            const char *repl, char *out, size_t out_len) {
    (void)re; (void)str; (void)len; (void)repl; (void)out; (void)out_len;
    return PNY_REGEX_NO_LIB;
}
int pny_regex_replace_all(const PnyRegex *re, const char *str, size_t len,
                          const char *repl, char *out, size_t out_len) {
    (void)re; (void)str; (void)len; (void)repl; (void)out; (void)out_len;
    return PNY_REGEX_NO_LIB;
}
int pny_regex_match_str(const char *pattern, int flags, const char *str, size_t len,
                        PnyRegexMatch matches[PNY_REGEX_MAX_GROUPS]) {
    (void)pattern; (void)flags; (void)str; (void)len; (void)matches;
    return PNY_REGEX_NO_LIB;
}

#endif /* PONY_NO_REGEX */
