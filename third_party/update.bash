#!/bin/bash

set -e
set -x

# Change working directory to script location
pushd `dirname $0` > /dev/null

rm -rf uWebSockets readerwriterqueue

git clone git@github.com:uNetworking/uWebSockets.git
pushd uWebSockets
git checkout 7956546836904402ad18a00a1aa033c7a9c5c5ab
git submodule update --init --recursive
# git apply ../uWebSockets.patch
pushd uSockets
make -j8
popd
popd

git clone git@github.com:cameron314/readerwriterqueue.git
pushd readerwriterqueue
git checkout v1.0.5
popd

popd > /dev/null
