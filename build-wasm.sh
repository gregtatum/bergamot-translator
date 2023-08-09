#!/usr/bin/env bash
set -e
set -x

# Usage
Usage="Build translator to wasm (with/without wormhole).

Usage: $(basename "$0") [WORMHOLE]

    where:
    WORMHOLE      An optional string argument
                  - when specified on command line, builds wasm artifacts with wormhole
                  - when not specified (the default behaviour), builds wasm artifacts without wormhole."

if [ "$#" -gt 1 ]; then
  echo "Illegal number of parameters passed"
  echo "$Usage"
  exit
fi

WORMHOLE=false

if [ "$#" -eq 1 ]; then
  if [ "$1" = "WORMHOLE" ]; then
    WORMHOLE=true
  else
    echo "Illegal parameter passed"
    echo "$Usage"
    exit
  fi
fi

# Run script from the context of the script-containing directory
cd "$(dirname $0)"

# Prerequisite: Download and Install Emscripten using following instructions (unless the EMSDK env var is already set)
if [ "$EMSDK" == "" ]; then
  EMSDK_UPDATE_REQUIRED=0
  if [ ! -d "emsdk" ]; then
    git clone https://github.com/emscripten-core/emsdk.git
    EMSDK_UPDATE_REQUIRED=1
  else
    cd emsdk
    git fetch
    # Only pull if necessary
    if [ $(git rev-parse HEAD) != $(git rev-parse @{u}) ]; then
      git pull --ff-only
      EMSDK_UPDATE_REQUIRED=1
    fi
    cd -
  fi
  if [ "$EMSDK_UPDATE_REQUIRED" == "1" ]; then
    cd emsdk
    ./emsdk install 3.1.8
    ./emsdk activate 3.1.8
    cd -
  fi
  source ./emsdk/emsdk_env.sh
fi

# Compile
#    1. Create a folder where you want to build all the artifacts and compile
BUILD_DIRECTORY="build-wasm"
if [ ! -d ${BUILD_DIRECTORY} ]; then
  mkdir ${BUILD_DIRECTORY}
fi
cd ${BUILD_DIRECTORY}

if [ "$WORMHOLE" = true ]; then
  emcmake cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCOMPILE_WASM=on ../
else
  emcmake cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCOMPILE_WASM=on -DWORMHOLE=off ../
fi
emmake make -j10

#     2. Enable SIMD Wormhole via Wasm instantiation API in generated artifacts
if [ "$WORMHOLE" = true ]; then
  bash ../wasm/patch-artifacts-enable-wormhole.sh
fi

#     3. Import GEMM library from a separate wasm module
bash ../wasm/patch-artifacts-import-gemm-module.sh

set +x
echo ""
echo "Build complete"
echo ""
echo "  ./build-wasm/bergamot-translator-worker.js"
echo "  ./build-wasm/bergamot-translator-worker.wasm"

WASM_SIZE=$(wc -c bergamot-translator-worker.wasm | awk '{print $1}')
GZIP_SIZE=$(gzip -c bergamot-translator-worker.wasm | wc -c | xargs) # xargs trims the whitespace

# Convert it to human readable.
WASM_SIZE=$(awk 'BEGIN {printf "%.2f",'$WASM_SIZE'/1048576}')M
GZIP_SIZE=$(awk 'BEGIN {printf "%.2f",'$GZIP_SIZE'/1048576}')M

echo "  Uncompressed wasm size: $WASM_SIZE"
echo "  Compressed wasm size: $GZIP_SIZE"

# The artifacts (.js and .wasm files) will be available in the build directory
exit 0
