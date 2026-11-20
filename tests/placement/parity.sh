#!/bin/sh
# Builds the std-only driver and runs parity.js. Needs g++ (or c++) and node.
set -e
here=$(cd "$(dirname "$0")" && pwd)
out=${TMPDIR:-/tmp}/qute-placement-parity
${CXX:-c++} -std=c++17 -O1 -I"$here/../../src/core" "$here/parity.cpp" "$here/../../src/core/chatboxplacement.cpp" -o "$out"
node "$here/parity.js" "$out"
