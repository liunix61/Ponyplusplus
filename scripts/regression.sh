#!/bin/bash
# regression.sh — Pony++ 统一回归测试 runner（nanonode 式工程化）
# 用法:
#   bash scripts/regression.sh          # C 测试 + gtest 全量
#   bash scripts/regression.sh --quick  # 仅 C 测试（快速回归）
#   bash scripts/regression.sh --san    # C 测试 + gtest + sanitizer 矩阵(asan/ubsan)
#   bash scripts/regression.sh --cov    # C 测试 + 覆盖率报告(gcovr)
set -u
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
REPORT="$ROOT/build/regression_report.txt"
mkdir -p build
: > "$REPORT"

PASS=0; FAIL=0; FAILED_LIST=()

log() { echo "$@" | tee -a "$REPORT"; }

# ---- 阶段 0: 环境清理（nanonode 规范: 每轮测试前清理临时产物）----
log "== [0] cleanup =="
rm -f /tmp/ponypp_*.c /tmp/ponypp_gen.c /tmp/ponypp_*.pony 2>/dev/null
mv "$ROOT"/*.gcov "$ROOT/build-cov/" 2>/dev/null || true
log "   tmp files cleaned; gcov artifacts moved to build-cov/"

# ---- 阶段 1: C 测试（tests/*.c, make test 体系）----
log ""
log "== [1] C tests (tests/*.c) =="
if make -s test >>"$REPORT" 2>&1; then
    log "C-TESTS: PASS"
    PASS=$((PASS+1))
else
    log "C-TESTS: FAIL (see report tail)"
    FAIL=$((FAIL+1)); FAILED_LIST+=("C-TESTS")
fi

# ---- 阶段 2: gtest 全量（cmake + ctest, ~175 个）----
if [ "${1:-}" != "--quick" ]; then
    log ""
    log "== [2] gtest (cmake build-cmake + ctest) =="
    cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Debug > /dev/null 2>&1
    if cmake --build build-cmake -j4 >>"$REPORT" 2>&1; then
        CTMP=$(mktemp)
        (cd build-cmake && ctest -j4 --timeout 180 --output-on-failure > "$CTMP" 2>&1)
        CT_RES=$(grep -E "tests passed|tests failed" "$CTMP" | tail -2)
        log "   $CT_RES"
        if grep -q "100% tests passed" "$CTMP"; then
            log "GTEST: PASS"
            PASS=$((PASS+1))
        else
            log "GTEST: FAIL — failed tests:"
            grep -E "^\s+Failed\s" "$CTMP" | head -30 | tee -a "$REPORT"
            FAIL=$((FAIL+1)); FAILED_LIST+=("GTEST")
        fi
        cp "$CTMP" build/ctest_last.log
        rm -f "$CTMP"
    else
        log "GTEST: BUILD-FAIL"
        FAIL=$((FAIL+1)); FAILED_LIST+=("GTEST-BUILD")
    fi
fi

# ---- 阶段 3: sanitizer 矩阵（--san）----
if [ "${1:-}" = "--san" ]; then
    for SAN in asan ubsan; do
        log ""
        log "== [3.$SAN] sanitizer: $SAN =="
        BDIR="build-$SAN"
        if [ "$SAN" = "asan" ]; then SANFLAGS="-fsanitize=address -fno-omit-frame-pointer"; else SANFLAGS="-fsanitize=undefined"; fi
        make -s clean > /dev/null 2>&1
        if make -s test CFLAGS="-g -O1 $SANFLAGS -Iinclude -std=c11 -Wall" >>"$REPORT" 2>&1; then
            log "SAN-$SAN: PASS"; PASS=$((PASS+1))
        else
            log "SAN-$SAN: FAIL"; FAIL=$((FAIL+1)); FAILED_LIST+=("SAN-$SAN")
        fi
    done
    make -s clean > /dev/null 2>&1
    make -s all > /dev/null 2>&1  # 恢复普通构建
fi

# ---- 阶段 4: 覆盖率（--cov）----
if [ "${1:-}" = "--cov" ]; then
    log ""
    log "== [4] coverage (gcovr) =="
    if [ -d build-cov ]; then
        (cd build-cov && gcovr -r .. --html --html-details -o coverage.html ../src 2>/dev/null | tail -3) | tee -a "$REPORT"
        log "coverage report: build-cov/coverage.html"
    else
        log "COV: build-cov/ not found (cmake -B build-cov -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='--coverage' first)"
    fi
fi

# ---- 汇总 ----
log ""
log "== SUMMARY =="
log "PASS: $PASS  FAIL: $FAIL"
if [ $FAIL -gt 0 ]; then
    log "FAILED: ${FAILED_LIST[*]}"
    log "report: $REPORT"
    exit 1
fi
log "ALL REGRESSION GREEN"
log "report: $REPORT"
