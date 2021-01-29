FROM alpine:3.13 AS builder

RUN apk add --no-cache tini gcc g++ pkgconf make \
	util-linux-dev sqlite-dev mariadb-connector-c-dev cmake zlib-dev fmt-dev \
	file-dev libexif-dev curl-dev ffmpeg-dev ffmpegthumbnailer-dev \
	libmatroska-dev libebml-dev taglib-dev pugixml-dev spdlog-dev \
	duktape-dev libupnp-dev git bash

WORKDIR /gerbera_build

COPY . .
RUN mkdir build && \
    cd build && \
    cmake ../ -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
        -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 \
        -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=0 -DWITH_DEBUG=1 && \
    make -j`nproc`

FROM alpine:3.13
RUN apk add --no-cache tini util-linux sqlite mariadb-connector-c zlib fmt \
	file libexif curl ffmpeg-libs ffmpegthumbnailer libmatroska libebml taglib \
	pugixml spdlog sqlite-libs libupnp duktape

# Gerbera itself
COPY --from=builder /gerbera_build/build/gerbera /bin/gerbera
COPY --from=builder /gerbera_build/scripts/js /usr/local/share/gerbera/js
COPY --from=builder /gerbera_build/web /usr/local/share/gerbera/web

RUN mkdir -p /root/.config/gerbera &&\
    gerbera --create-config > /root/.config/gerbera/config.xml &&\
    sed 's/<import hidden-files="no">/<import hidden-files="no">\n\
<autoscan use-inotify="yes">\n\
<directory location="\/root" mode="inotify" \
recursive="yes" hidden-files="no"\/>\n\
<\/autoscan>/' -i /root/.config/gerbera/config.xml

EXPOSE 49152
EXPOSE 1900/udp

ENTRYPOINT ["/sbin/tini", "--"]
CMD [ "gerbera","-p", "49152" ]
