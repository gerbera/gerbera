FROM alpine:3.11

RUN apk add --no-cache tini gcc g++ pkgconf make automake autoconf libtool \
	util-linux-dev sqlite-dev mariadb-connector-c-dev cmake zlib-dev fmt-dev \
	file-dev libexif-dev curl-dev ffmpeg-dev ffmpegthumbnailer-dev wget xz \
	libmatroska-dev libebml-dev taglib-dev

WORKDIR /gerbera_build

COPY . .

RUN mkdir build && \
    cd build && \
    sh ../scripts/install-pugixml.sh && \
    sh ../scripts/install-pupnp.sh && \
    sh ../scripts/install-duktape.sh && \
    sh ../scripts/install-spdlog.sh && \
    cmake ../ -DWITH_MAGIC=1 -DWITH_MYSQL=1 -DWITH_CURL=1 -DWITH_JS=1 \
        -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=1 \
        -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=0 -DWITH_DEBUG=1 && \
    make -j`nproc` && \
    make install && \
    rm -rf /gerbera_build

RUN mkdir -p /root/.config/gerbera &&\
    gerbera --create-config > /root/.config/gerbera/config.xml &&\
    sed 's/<import hidden-files="no">/<import hidden-files="no">\n\
<autoscan use-inotify="yes">\n\
<directory location="\/root" mode="inotify" level="full" \
recursive="yes" hidden-files="no"\/>\n\
<\/autoscan>/' -i /root/.config/gerbera/config.xml

EXPOSE 49152
EXPOSE 1900/udp

ENTRYPOINT ["/sbin/tini", "--"]
CMD [ "gerbera","-p", "49152" ]
