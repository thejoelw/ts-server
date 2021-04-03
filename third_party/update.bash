#!/bin/bash

set -e
set -x

# Change working directory to script location
pushd `dirname $0` > /dev/null

rm -rf uWebSockets readerwriterqueue

git clone --recursive git@github.com:uNetworking/uWebSockets.git --branch v19.0.0a5
pushd uWebSockets
git apply ../uWebSockets.patch
pushd uSockets
make -j8
popd
popd

git clone git@github.com:cameron314/readerwriterqueue.git
pushd readerwriterqueue
git checkout v1.0.4
popd

popd > /dev/null
