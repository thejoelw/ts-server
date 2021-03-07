#!/bin/bash

set -e
set -x

# Change working directory to script location
pushd `dirname $0` > /dev/null

rm -rf argparse uWebSockets readerwriterqueue

git clone git@github.com:p-ranav/argparse.git
pushd argparse
git checkout 9903a22904fed8176c4a1f69c4b691304b23c78e
popd

git clone --recursive git@github.com:uNetworking/uWebSockets.git --branch v19.0.0a4
pushd uWebSockets/uSockets
make -j8
popd

git clone git@github.com:cameron314/readerwriterqueue.git
pushd readerwriterqueue
git checkout v1.0.4
popd

popd > /dev/null
