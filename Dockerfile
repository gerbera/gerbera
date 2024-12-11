ARG BASE_IMAGE=alpine:3.21

FROM ${BASE_IMAGE} AS builder

RUN apk add --no-cache  \
    sudo \
    bash \
    cmake zlib-dev \
    curl-dev \
    duktape-dev \
    ffmpeg-dev \
    file-dev \
    fmt-dev \
    g++ \
    gcc \
    git \
    libebml-dev \
    libmatroska-dev \
    wavpack wavpack-dev \
    make \
    patch \
    tar \
    lsb-release \
    mariadb-connector-c-dev \
    pkgconf \
    pugixml-dev \
    spdlog-dev \
    sqlite-dev \
    taglib-dev \
    tini \
    util-linux-dev \
    autoconf \
    automake \
    libtool \
    wget \
    file

# Build ffmpegthumbnailer
WORKDIR /ffmpegthumbnailer_build
COPY scripts/install-ffmpegthumbnailer.sh scripts/versions.sh ./
COPY scripts/alpine/* ./alpine/
RUN ./install-ffmpegthumbnailer.sh

# Build libupnp
WORKDIR /libupnp_build
COPY scripts/install-pupnp.sh scripts/versions.sh ./
COPY scripts/alpine/* ./alpine/
RUN ./install-pupnp.sh

# Build libexiv2
WORKDIR /libexiv2_build
COPY scripts/install-libexiv2.sh scripts/versions.sh ./
COPY scripts/alpine/* ./alpine/
RUN ./install-libexiv2.sh

# Build libexif
WORKDIR /libexif_build
COPY scripts/install-libexif.sh scripts/versions.sh ./
COPY scripts/alpine/* ./alpine/
RUN ./install-libexif.sh

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
    sudo \
    bash \
    shadow \
    curl \
    duktape \
    ffmpeg-libavutil \
    ffmpeg-libavformat \
    ffmpeg-libavcodec \
    ffmpeg-libavfilter \
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
# Copy libexif
COPY --from=builder /usr/local/lib/libexif.so* /usr/lib/
# Copy ffmpegthumbnailer
COPY --from=builder /usr/local/lib/libffmpegthumbnailer.so* /usr/lib/

# Copy Gerbera
COPY --from=builder /gerbera_build/build/gerbera /bin/gerbera
COPY --from=builder /gerbera_build/scripts/js /usr/local/share/gerbera/js
COPY --from=builder /gerbera_build/web /usr/local/share/gerbera/web
COPY --from=builder /gerbera_build/src/database/*/*.sql /gerbera_build/src/database/*/*.xml /usr/local/share/gerbera/
COPY --from=builder /gerbera_build/scripts/docker/docker-entrypoint.sh /usr/local/bin

ARG IMAGE_USER=gerbera
ARG IMAGE_GROUP=gerbera
ARG IMAGE_UID=1042
ARG IMAGE_GID=1042
ARG IMAGE_PORT=49494

RUN addgroup -S ${IMAGE_GROUP} --gid=${IMAGE_GID} 2>/dev/null && \
    adduser -S -D -H -h /var/run/gerbera -s /sbin/nologin -G ${IMAGE_GROUP} -g ${IMAGE_GROUP} --uid=${IMAGE_UID} ${IMAGE_USER} 2>/dev/null && \
    addgroup ${IMAGE_USER} video && \
    mkdir /var/run/gerbera/ && chmod 2775 /var/run/gerbera/ && \
    mkdir /content && chmod 777 /content && ln -s /content /mnt/content && \
    mkdir -p /mnt/customization/js && mkdir -p /mnt/customization/shell && \
    chmod -R 777 /mnt/customization

# Update entrypoint
RUN chmod 0755 /usr/local/bin/docker-entrypoint.sh \
 && sed "s/\$IMAGE_UID/$IMAGE_UID/g" -i /usr/local/bin/docker-entrypoint.sh \
 && sed "s/\$IMAGE_GID/$IMAGE_GID/g" -i /usr/local/bin/docker-entrypoint.sh \
 && sed "s/\$IMAGE_USER/$IMAGE_USER/g" -i /usr/local/bin/docker-entrypoint.sh \
 && sed "s/\$IMAGE_GROUP/$IMAGE_GROUP/g" -i /usr/local/bin/docker-entrypoint.sh

EXPOSE ${IMAGE_PORT}
EXPOSE 1900/udp

ENTRYPOINT ["/sbin/tini", "--", "docker-entrypoint.sh"]
CMD ["gerbera", "--port", "${IMAGE_PORT}", "--config", "/var/run/gerbera/config.xml"]

FROM gerbera AS with_transcoding
RUN apk add --no-cache \
    ffmpeg \
    libheif-tools \
    vlc

FROM gerbera AS default
