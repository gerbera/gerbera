FROM ubuntu:18.04

RUN apt-get update &&\
    apt-get install -y uuid-dev libexpat1-dev libsqlite3-dev zlib1g-dev\
    libmagic-dev libexif-dev libcurl4-openssl-dev libavutil-dev libavcodec-dev \
    libavformat-dev libavdevice-dev libavfilter-dev libavresample-dev \
    libswscale-dev libswresample-dev libpostproc-dev g++ cmake \
    wget autoconf libtool xz-utils &&\
    rm -rf /var/lib/{apt,dpkg,cache,log}/

WORKDIR /app

COPY . .

RUN mkdir build &&\
    cd build &&\
    sh ../scripts/install-pupnp18.sh &&\
    sh ../scripts/install-duktape.sh &&\
    sh ../scripts/install-taglib111.sh &&\
    cmake ../ -DWITH_MAGIC=1 -DWITH_MYSQL=0 -DWITH_CURL=1 -DWITH_JS=1 \
        -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_FFMPEGTHUMBNAILER=0 \
        -DWITH_EXIF=1 -DWITH_LASTFM=0 -DWITH_SYSTEMD=0 &&\
    make -j4 &&\
    make install

RUN mkdir -p /root/.config/gerbera &&\
    gerbera --create-config > /root/.config/gerbera/config.xml

ARG PORT=49555
EXPOSE ${PORT}

ENV PORT=${PORT}

ENTRYPOINT /bin/gerbera -p ${PORT} 