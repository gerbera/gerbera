FROM alpine:3.14 AS builder

RUN apk add --no-cache tini gcc g++ pkgconf make \
    util-linux-dev sqlite-dev mariadb-connector-c-dev cmake zlib-dev fmt-dev \
    file-dev libexif-dev curl-dev ffmpeg-dev ffmpegthumbnailer-dev \
    libmatroska-dev libebml-dev taglib-dev pugixml-dev spdlog-dev \
    duktape-dev git bash \
    # packages to build libupnp
    autoconf automake libtool file

# Build libupnp
WORKDIR /libupnp_build
COPY scripts/install-pupnp.sh scripts/versions.sh ./
RUN ./install-pupnp.sh

# Build Gerbera
WORKDIR /gerbera_build
COPY . .
RUN mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS=-g1 \
        -DWITH_MAGIC=YES \
        -DWITH_MYSQL=YES \
        -DWITH_CURL=YES \
        -DWITH_JS=YES \
        -DWITH_TAGLIB=YES \
        -DWITH_AVCODEC=YES \
        -DWITH_FFMPEGTHUMBNAILER=YES \
        -DWITH_EXIF=YES \
        -DWITH_LASTFM=NO \
        -DWITH_SYSTEMD=NO \
        -DWITH_DEBUG=YES && \
    make -j$(nproc)

FROM alpine:3.14
RUN apk add --no-cache tini util-linux sqlite mariadb-connector-c zlib fmt \
    file libexif curl ffmpeg-libs ffmpegthumbnailer libmatroska libebml taglib \
    pugixml spdlog sqlite-libs duktape su-exec tzdata

# Copy libupnp
COPY --from=builder /usr/local/lib/libixml.so.* /usr/local/lib/libupnp.so.* /usr/lib/

# Copy Gerbera
COPY --from=builder /gerbera_build/build/gerbera /bin/gerbera
COPY --from=builder /gerbera_build/scripts/js /usr/local/share/gerbera/js
COPY --from=builder /gerbera_build/web /usr/local/share/gerbera/web
COPY --from=builder /gerbera_build/src/database/*/*.sql /gerbera_build/src/database/*/*.xml /usr/local/share/gerbera/
COPY --from=builder /gerbera_build/scripts/docker/docker-entrypoint.sh /usr/local/bin

RUN addgroup -S gerbera 2>/dev/null && \
    adduser -S -D -H -h /var/run/gerbera -s /sbin/nologin -G gerbera -g gerbera gerbera 2>/dev/null && \
    mkdir /var/run/gerbera/ && chmod 2775 /var/run/gerbera/ && \
    mkdir /content && chmod 777 /content

EXPOSE 49494
EXPOSE 1900/udp

ENTRYPOINT ["/sbin/tini", "--", "docker-entrypoint.sh"]
CMD ["gerbera","--port", "49494", "--config", "/var/run/gerbera/config.xml"]
