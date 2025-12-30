#!/bin/sh
set -eu

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
while getopts ":f:t" opt; do
  case ${opt} in
    f)
      FORMAT=${OPTARG}
      ;;
    t)
      ACTION_TEST=1
      ;;
    *)
      usage; exit 1
      ;;
  esac
done

if [ "${FORMAT}" = "debug" ]; then
  TARGET=minilsm_debug
elif [ "${FORMAT}" = "release" ]; then
  TARGET=minilsm
else
  if [ "${ACTION_TEST:-0}" = "1" ]; then
    TARGET=test_skiplist
  else
    echo "Unknown format: ${FORMAT}" >&2
    usage
    exit 1
  fi
fi

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

echo "Configuring project (out: $BUILD_DIR)"
mkdir -p "$BUILD_DIR"
cmake -S "$ROOT_DIR" -B "$BUILD_DIR"

echo "Building target: $TARGET"
cmake --build "$BUILD_DIR" --target "$TARGET" -- -j

echo "Build finished: target=$TARGET"

if [ "${TARGET}" = "test_skiplist" ]; then
  echo "Running tests..."
  (cd "$BUILD_DIR" && ctest --output-on-failure)
fi
