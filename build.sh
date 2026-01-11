#!/bin/bash

usage() {
  cat <<EOF
Usage: $0 -f <debug|release>

Build the minilsm library quickly.
  -f debug    build the debug-style library target (minilsm_debug)
  -f release  build the release-style library target (minilsm)

  -t          build tests and run the test suite

Examples:
  $0 -f debug
  $0 -f release
EOF
}

if [ $# -eq 0 ]; then
  usage
  exit 1
fi

FORMAT=""
ACTION_TEST=0  # 初始化变量，避免未定义
while getopts ":f:t" opt; do
  case ${opt} in
    f)
      FORMAT=${OPTARG}
      ;;
    t)
      ACTION_TEST=1
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage; exit 1
      ;;
    :)   # 捕获缺少参数的情况（比如 -f 后没跟值）
      echo "Option -$OPTARG requires an argument." >&2
      usage; exit 1
      ;;
  esac
done

# 1. 核心修改：根据 FORMAT 设置 CMAKE_BUILD_TYPE
if [ "${FORMAT}" = "debug" ]; then
  TARGET=minilsm
  CMAKE_BUILD_TYPE="Debug"  # Debug模式：带符号表、-O0
elif [ "${FORMAT}" = "release" ]; then
  TARGET=minilsm
  CMAKE_BUILD_TYPE="Release" # Release模式：无符号表、-O3
else
  if [ "${ACTION_TEST}" = "1" ]; then
    TARGET=test_skiplist
    CMAKE_BUILD_TYPE="Debug"  # 测试默认用Debug模式，方便调试
  else
    echo "Unknown format: ${FORMAT}" >&2
    usage
    exit 1
  fi
fi

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Configuring project (out: $BUILD_DIR, build type: $CMAKE_BUILD_TYPE)"
mkdir -p "$BUILD_DIR"
# 2. 核心修改：cmake 命令添加 -DCMAKE_BUILD_TYPE 参数
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"

echo "Building target: $TARGET (build type: $CMAKE_BUILD_TYPE)"
cmake --build "$BUILD_DIR" --target "$TARGET" -- -j

echo "Build finished: target=$TARGET, build type=$CMAKE_BUILD_TYPE"

if [ "${TARGET}" = "test_skiplist" ]; then
  echo "Running tests..."
  (cd "$BUILD_DIR" && ctest --output-on-failure)
fi