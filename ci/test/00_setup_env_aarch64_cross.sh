#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export HOST=aarch64-linux-gnu
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:26.04"
export CI_IMAGE_PLATFORM="linux/amd64"
export PACKAGES="g++-aarch64-linux-gnu"
export DEP_OPTS="NO_IPC=1"
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
printf -v BITCOIN_CONFIG "%q " \
  --preset=dev-mode \
  -DENABLE_IPC=OFF \
  -DREDUCE_EXPORTS=ON
export BITCOIN_CONFIG
