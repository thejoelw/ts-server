FROM alpine AS build

RUN apk update && apk add --no-cache build-base pkgconfig git zstd-dev
RUN ln -s $(which g++) /usr/bin/clang++

WORKDIR /app
COPY build-release.sh .
COPY src .
COPY third_party .
RUN ./build-release.sh

FROM busybox

COPY --from=build build-release/ts-server /ts-server

RUN mkdir /ts_server_data

EXPOSE 9001

CMD ["/ts-server", "/ts_server_data"]