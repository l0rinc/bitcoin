#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export PACKAGES="python3-pip python3-zmq g++ libfontconfig1 libxcb1"
export PIP_PACKAGES="--break-system-packages pycapnp"
export CI_IMAGE_NAME_TAG="mirror.gcr.io/debian:trixie"
export CI_IMAGE_PLATFORM="linux/arm64"
export CI_LIMIT_STACK_SIZE=1
printf -v BITCOIN_CONFIG "%q " \
  --preset=dev-mode \
  -DREDUCE_EXPORTS=ON \
  -DCMAKE_CXX_FLAGS="-Wno-error=maybe-uninitialized"
export BITCOIN_CONFIG
