#!/usr/bin/env bash
set -euo pipefail

# Fetch and build GoogleTest into third_party/googletest/install
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TP_DIR="$ROOT_DIR/third_party/googletest"
INSTALL_DIR="$TP_DIR/install"

mkdir -p "$TP_DIR"
cd "$TP_DIR"

if [ ! -d "$TP_DIR/.git" ] && [ ! -f "$TP_DIR/CMakeLists.txt" ]; then
  echo "Downloading googletest release-1.14.0..."
  rm -rf ./*
  curl -L -o gtest.zip https://github.com/google/googletest/archive/refs/tags/release-1.14.0.zip
  unzip gtest.zip
  mv googletest-release-1.14.0/* . || true
  rm -rf googletest-release-1.14.0 gtest.zip
fi

mkdir -p build
cd build
cmake .. -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
cmake --build . -j
cmake --install . --prefix "$INSTALL_DIR"

echo "GoogleTest built and installed to $INSTALL_DIR"
echo "When building the project, ensure CMake variable GTEST_ROOT points to $INSTALL_DIR (default uses this path)."
