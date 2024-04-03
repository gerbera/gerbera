ARG BASE_IMAGE=alpine:3.19
FROM ${BASE_IMAGE} AS builder

RUN apk add --no-cache  \
    bash \
    cmake zlib-dev \
    curl-dev \
    duktape-dev \
    ffmpeg4-dev \
    jpeg-dev \
    file-dev \
    fmt-dev \
    g++ \
    gcc \
    git \
    libebml-dev \
    expat-dev brotli-dev inih-dev inih-inireader-dev \
    libexif-dev \
    libmatroska-dev \
    wavpack wavpack-dev \
    make \
    mariadb-connector-c-dev \
    pkgconf \
    pugixml-dev \
    spdlog-dev \
    sqlite-dev \
    taglib-dev \
    tini \
    util-linux-dev \
    # packages to build libupnp
    autoconf \
    automake \
    libtool \
    file

# Build ffmpegthumbnailer
WORKDIR /ffmpegthumbnailer_build
COPY scripts/install-ffmpegthumbnailer.sh scripts/versions.sh ./
RUN ./install-ffmpegthumbnailer.sh

# Build libupnp
WORKDIR /libupnp_build
COPY scripts/install-pupnp.sh scripts/versions.sh ./
RUN ./install-pupnp.sh

# Build libexiv2
WORKDIR /libexiv2_build
COPY scripts/install-libexiv2.sh scripts/versions.sh ./
RUN ./install-libexiv2.sh

# Build Gerbera
WORKDIR /gerbera_build
COPY . .
RUN cmake -S . -B build --preset=release-pupnp \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS=-g1 \
        -DWITH_SYSTEMD=NO \
    && \
    cmake --build build -v -j$(nproc)

FROM ${BASE_IMAGE} AS gerbera
RUN apk add --no-cache \
    curl \
    duktape \
    ffmpeg4 \
    libjpeg \
    file \
    fmt \
    libebml \
    libexif \
    expat brotli inih inih-inireader \
    libmatroska \
    wavpack \
    mariadb-connector-c \
    pugixml \
    spdlog \
    sqlite \
    sqlite-libs \
    su-exec \
    taglib \
    tini \
    tzdata \
    util-linux \
    zlib

# Copy libupnp
COPY --from=builder /usr/local/lib/libixml.so* /usr/local/lib/libupnp.so* /usr/lib/
# Copy libexiv2
COPY --from=builder /usr/local/lib/libexiv2.so* /usr/lib/
# Copy ffmpegthumbnailer
COPY --from=builder /usr/local/lib/libffmpegthumbnailer.so* /usr/lib/

# Copy Gerbera
COPY --from=builder /gerbera_build/build/gerbera /bin/gerbera
COPY --from=builder /gerbera_build/scripts/js /usr/local/share/gerbera/js
COPY --from=builder /gerbera_build/web /usr/local/share/gerbera/web
COPY --from=builder /gerbera_build/src/database/*/*.sql /gerbera_build/src/database/*/*.xml /usr/local/share/gerbera/
COPY --from=builder /gerbera_build/scripts/docker/docker-entrypoint.sh /usr/local/bin

RUN addgroup -S gerbera 2>/dev/null && \
    adduser -S -D -H -h /var/run/gerbera -s /sbin/nologin -G gerbera -g gerbera gerbera 2>/dev/null && \
    addgroup gerbera video && \
    mkdir /var/run/gerbera/ && chmod 2775 /var/run/gerbera/ && \
    mkdir /content && chmod 777 /content && ln -s /content /mnt/content && \
    mkdir -p /mnt/customization/js && mkdir -p /mnt/customization/shell && \
    chmod -R 777 /mnt/customization

EXPOSE 49494
EXPOSE 1900/udp

ENTRYPOINT ["/sbin/tini", "--", "docker-entrypoint.sh"]
CMD ["gerbera","--port", "49494", "--config", "/var/run/gerbera/config.xml"]

FROM gerbera AS with_transcoding
RUN apk add --no-cache \
    ffmpeg4 \
    ffmpeg \
    libheif-tools \
    vlc

FROM gerbera AS default
