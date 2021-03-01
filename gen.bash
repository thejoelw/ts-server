#!/bin/bash

set -e
set -x

TMP_DIR=$(mktemp -d)

for file in build-*/tup.config; do
  tup generate --config "$file" $TMP_DIR/$(dirname "$file").sh &
done

wait
mv $TMP_DIR/* .

rmdir $TMP_DIR