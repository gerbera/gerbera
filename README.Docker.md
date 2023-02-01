<img src="https://github.com/gerbera/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Gerbera - UPnP Media Server

[![Current Release](https://img.shields.io/github/release/gerbera/gerbera.svg?style=for-the-badge)](https://github.com/gerbera/gerbera/releases/latest) [![Build Status](https://img.shields.io/github/actions/workflow/status/gerbera/gerbera/ci.yml?style=for-the-badge&branch=master)](https://github.com/gerbera/gerbera/actions?query=workflow%3A%22CI+validation%22+branch%3Amaster) [![Docker Version](https://img.shields.io/docker/v/gerbera/gerbera?color=teal&label=docker&logoColor=white&sort=semver&style=for-the-badge)](https://hub.docker.com/r/gerbera/gerbera/tags?name=v) [![Documentation Status](https://img.shields.io/readthedocs/gerbera?style=for-the-badge)](http://docs.gerbera.io/en/stable/?badge=stable) [![IRC](https://img.shields.io/badge/IRC-on%20freenode-orange.svg?style=for-the-badge)](https://webchat.freenode.net/?channels=#gerbera)

Gerbera is a UPnP media server which allows you to stream your digital media through your home network and consume it on a variety of UPnP compatible devices.

## Documentation
For general help on using Gerbera, head over to our documentation online at [docs.gerbera.io](https://docs.gerbera.io).

# Image Architectures
- amd64
- armv7
- arm64

# Network Setup
### Ports
Port `49494/tcp` (HTTP) and `1900/udp` (SSDP Multicast) are exposed by default.

### Multicast
UPnP relies on having clients and servers able to communicate via IP Multicast.
The default docker bridge network setup does not support multicast. The easiest way to achieve this is to use
"host networking".
Connecting Gerbera to your network via the "macvlan" driver should work, but remember you will not be
able to access the container from the docker host with this method by default.

# Transcoding Tools
Transcoding tools are made available in a separate image with the `-transcoding` suffix.
e.g. `gerbera/gerbera:1.9.2-transcoding`. Includes tools such as ffmpeg and vlc.

# Examples
## Serve some files via a volume
```console
$ docker run \
    --name some-gerbera \
    --network=host \
    -v /some/files:/content:ro \
     gerbera/gerbera:vX.X.X
```

or for those that prefer docker-compose:

```console
---
version: "2.1"
services:
  gerbera:
    image: gerbera/gerbera
    container_name: gerbera
    network_mode: host
    volumes:
      - gerbera-config:/var/run/gerbera
      - /some/files:/content:ro

volumes:
  gerbera-config:
    external: false
```

The directory `/content` is automatically scanned for content by default.
Host networking enables us to bypass issues with broadcast across docker bridges.

## Provide your own config file
```console
$ docker run \
    --name another-gerbera \
    --network=host \
    -v /some/files:/content:ro \
    -v /some/path/config.xml:/var/run/gerbera/config.xml \
     gerbera/gerbera:vX.X.X
```
