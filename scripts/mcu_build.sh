#!/usr/bin/env bash
# Pony++ MCU 构建脚本
# 使用 WAMR (WebAssembly Micro Runtime) 为 STM32/ESP32 生成 WASM
#
# 用法:
#   ./scripts/mcu_build.sh [target] [--mcu stm32f4|stm32h7|esp32|esp32s3]
#   target: .pny 源文件或目录
#
# 示例:
#   ./scripts/mcu_build.sh hello.pny --mcu stm32f4
#   ./scripts/mcu_build.sh ./src/ --mcu esp32
#
# 输出:
#   build/mcu/<target>.wasm - 精简 WASM (无 WASI 依赖)
#   build/mcu/<target>.bin  - 可选二进制导出
#
# 依赖:
#   - ponyppc (pony++ 编译器)
#   - wamrc (WAMR 编译器, 可选)
#   - wamr-compile 或 wamrc 工具
#   - STM32/ESP32 交叉编译器 (可选, 用于链接)

set -euo pipefail

# 颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 默认值
MCU="generic"
PONYPPC="${PONYPPC:-ponyppc}"
WAMRC="${WAMRC:-wamrc}"
BUILD_DIR="build/mcu"
VERBOSE=0

usage() {
    echo "Pony++ MCU 构建脚本 (WAMR 集成)"
    echo ""
    echo "用法: $0 [options] <source.pny|dir>"
    echo ""
    echo "选项:"
    echo "  --mcu <type>       MCU 类型: stm32f4|stm32h7|esp32|esp32s3|generic (默认: generic)"
    echo "  --ponyppc <path>   ponyppc 编译器路径 (默认: \$PONYPPC 或 ponyppc)"
    echo "  --wamrc <path>     wamrc 编译器路径 (默认: \$WAMRC 或 wamrc)"
    echo "  --build <dir>      构建输出目录 (默认: build/mcu)"
    echo "  -v                 详细输出"
    echo "  -h, --help         显示帮助"
    echo ""
    echo "示例:"
    echo "  $0 hello.pny --mcu stm32f4"
    echo "  $0 ./src/ --mcu esp32 --build out/"
    echo ""
    echo "MCU 预设内存配置:"
    echo "  STM32F4:  1 page (64KB), max 2 pages"
    echo "  STM32H7:  2 pages (128KB), max 4 pages"
    echo "  ESP32:    1 page (64KB), max 2 pages"
    echo "  ESP32S3:  2 pages (128KB), max 4 pages"
    echo "  Generic:  1 page (64KB), max 2 pages"
    exit 0
}

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# 解析参数
while [[ $# -gt 0 ]]; do
    case "$1" in
        --mcu)
            MCU="$2"; shift 2 ;;
        --ponyppc)
            PONYPPC="$2"; shift 2 ;;
        --wamrc)
            WAMRC="$2"; shift 2 ;;
        --build)
            BUILD_DIR="$2"; shift 2 ;;
        -v)
            VERBOSE=1; shift ;;
        -h|--help)
            usage ;;
        -*)
            log_error "未知选项: $1"
            usage ;;
        *)
            SOURCE="$1"; shift ;;
    esac
done

# 检查源文件
if [[ -z "${SOURCE:-}" ]]; then
    log_error "未指定源文件"
    usage
fi

if [[ ! -e "$SOURCE" ]]; then
    log_error "源文件不存在: $SOURCE"
    exit 1
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"

# 检查工具
if [[ ! -x "$PONYPPC" ]] && ! command -v "$PONYPPC" &>/dev/null; then
    log_error "ponyppc 未找到: $PONYPPC"
    exit 1
fi

log_info "Pony++ MCU 构建"
log_info "  源文件:   $SOURCE"
log_info "  MCU 目标: $MCU"
log_info "  构建目录: $BUILD_DIR"

# 编译 Pony++ -> WASM
log_info "编译 Pony++ 源码 -> WASM..."

# 收集所有 .pny 文件
PNY_FILES=()
if [[ -d "$SOURCE" ]]; then
    while IFS= read -r -d '' f; do
        PNY_FILES+=("$f")
    done < <(find "$SOURCE" -name "*.pny" -print0 | sort -z)
else
    PNY_FILES=("$SOURCE")
fi

if [[ ${#PNY_FILES[@]} -eq 0 ]]; then
    log_error "未找到 .pny 文件"
    exit 1
fi

log_info "  找到 ${#PNY_FILES[@]} 个 .pny 文件"

# 编译每个文件
for pny in "${PNY_FILES[@]}"; do
    name=$(basename "$pny" .pny)
    wasm_out="${BUILD_DIR}/${name}.wasm"

    if [[ $VERBOSE -eq 1 ]]; then
        log_info "  编译: $pny -> $wasm_out"
        "$PONYPPC" --target mcu-wasm --mcu "$MCU" -o "$wasm_out" "$pny"
    else
        "$PONYPPC" --target mcu-wasm --mcu "$MCU" -o "$wasm_out" "$pny" 2>&1 || {
            log_warn "编译失败: $pny"
        }
    fi

    if [[ -f "$wasm_out" ]]; then
        size=$(stat -c %s "$wasm_out" 2>/dev/null || stat -f%z "$wasm_out" 2>/dev/null || wc -c < "$wasm_out")
        size=$(echo "$size" | tr -d ' ')
        log_info "  ✓ $name.wasm ($size bytes)"
    fi
done

# 检查是否有 WAMR 工具
if command -v "$WAMRC" &>/dev/null; then
    log_info "  WAMR 编译器可用: $WAMRC"
    # 可以用 wamrc 进行进一步优化
    # wamrc -s -O "$wasm_out" -o "$wasm_out.optimized"
else
    log_warn "  WAMR 编译器不可用, 跳过 WAMR 优化"
fi

# 生成构建报告
echo "" > "${BUILD_DIR}/build_report.txt"
echo "Pony++ MCU Build Report" >> "${BUILD_DIR}/build_report.txt"
echo "========================" >> "${BUILD_DIR}/build_report.txt"
echo "Date:     $(date)" >> "${BUILD_DIR}/build_report.txt"
echo "MCU:      $MCU" >> "${BUILD_DIR}/build_report.txt"
echo "Source:   $SOURCE" >> "${BUILD_DIR}/build_report.txt"
echo "Build:    $BUILD_DIR" >> "${BUILD_DIR}/build_report.txt"
echo "" >> "${BUILD_DIR}/build_report.txt"
echo "Output files:" >> "${BUILD_DIR}/build_report.txt"
for f in "${BUILD_DIR}"/*.wasm; do
    [[ -f "$f" ]] && echo "  $(basename "$f") ($(wc -c < "$f") bytes)" >> "${BUILD_DIR}/build_report.txt"
done
echo "" >> "${BUILD_DIR}/build_report.txt"
echo "MCU Preset:" >> "${BUILD_DIR}/build_report.txt"
case "$MCU" in
    stm32f4)  echo "  STM32F4: 1 page (64KB), max 2 pages" >> "${BUILD_DIR}/build_report.txt" ;;
    stm32h7)  echo "  STM32H7: 2 pages (128KB), max 4 pages" >> "${BUILD_DIR}/build_report.txt" ;;
    esp32)    echo "  ESP32: 1 page (64KB), max 2 pages" >> "${BUILD_DIR}/build_report.txt" ;;
    esp32s3)  echo "  ESP32S3: 2 pages (128KB), max 4 pages" >> "${BUILD_DIR}/build_report.txt" ;;
    *)        echo "  Generic: 1 page (64KB), max 2 pages" >> "${BUILD_DIR}/build_report.txt" ;;
esac

log_info "构建报告: ${BUILD_DIR}/build_report.txt"
log_info "完成!"