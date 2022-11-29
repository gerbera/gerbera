if [[ "${GERBERA_ENV-head}" == "minimum" ]]; then

    DUKTAPE="2.2.1"
    EBML="1.3.9"
    EXIV2="v0.26"
    FFMPEGTHUMBNAILER="2.2.0"
    FMT="7.1.3"
    GOOGLETEST="1.10.0"
    LASTFM="0.4.0"
    MATROSKA="1.5.2"
    PUGIXML="1.10"
    PUPNP="1.14.6"
    SPDLOG="1.8.1"
    WAVPACK="5.1.0"
    TAGLIB="1.12"

elif [[ "${GERBERA_ENV-head}" == "default" ]]; then

    DUKTAPE="2.6.0"
    EBML="1.3.9"
    EXIV2="v0.26"
    FFMPEGTHUMBNAILER="2.2.0"
    FMT="7.1.3"
    GOOGLETEST="1.10.0"
    LASTFM="0.4.0"
    MATROSKA="1.5.2"
    PUGIXML="1.10"
    PUPNP="1.14.14"
    SPDLOG="1.8.5"
    WAVPACK="5.4.0"
    TAGLIB="1.12"

else

    DUKTAPE="2.7.0"
    EBML="1.4.4"
    EXIV2="v0.27.5"
    FFMPEGTHUMBNAILER="2.2.2"
    FMT="9.1.0"
    GOOGLETEST="1.12.1"
    LASTFM="0.4.0"
    MATROSKA="1.7.1"
    PUGIXML="1.13"
    PUPNP="1.14.14"
    SPDLOG="1.11.0"
    WAVPACK="5.6.0"
    TAGLIB="1.13"

fi

