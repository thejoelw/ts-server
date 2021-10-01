#!/bin/sh

sort --unique ts-server.files | xargs ls -1d 2>/dev/null > ts-server.files.tmp
mv ts-server.files.tmp ts-server.files

printf "src/\nthird_party/\nthird_party/spdlog/include/\nthird_party/rapidjson/include/\nthird_party/uWebSockets/uSockets/src/\nbuild-qtc/src/\n" > ts-server.includes
