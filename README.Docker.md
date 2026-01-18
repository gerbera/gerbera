<img src="https://github.com/gerbera/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Gerbera - UPnP Media Server

[![Current Release](https://img.shields.io/github/release/gerbera/gerbera.svg?style=for-the-badge)](https://github.com/gerbera/gerbera/releases/latest) [![Build Status](https://img.shields.io/github/actions/workflow/status/gerbera/gerbera/ci.yml?style=for-the-badge&branch=master)](https://github.com/gerbera/gerbera/actions?query=workflow%3A%22CI+validation%22+branch%3Amaster) [![Docker Version](https://img.shields.io/docker/v/gerbera/gerbera?color=teal&label=docker&logoColor=white&sort=semver&style=for-the-badge)](https://hub.docker.com/r/gerbera/gerbera/tags?name=3.) [![Documentation Status](https://img.shields.io/readthedocs/gerbera?style=for-the-badge)](http://docs.gerbera.io/en/stable/?badge=stable) [![IRC](https://img.shields.io/badge/IRC-on%20freenode-orange.svg?style=for-the-badge)](https://webchat.freenode.net/?channels=#gerbera)

[![Packaging status](https://repology.org/badge/tiny-repos/gerbera.svg?header=PACKAGES&style=for-the-badge)](https://repology.org/metapackage/gerbera/versions)

Gerbera is a UPnP media server which allows you to stream your digital media through your home network and consume it on a variety of UPnP compatible devices.

## Documentation
For general help on using Gerbera, head over to our documentation online at [docs.gerbera.io](https://docs.gerbera.io).

# Image Architectures
- amd64
- armv7
- arm64

# Network Setup

## Ports
Port `49494/tcp` (HTTP, also set as gerbera port via command line) and `1900/udp` (SSDP Multicast) are exposed by default.

## Multicast
UPnP relies on having clients and servers able to communicate via IP Multicast.
The default docker bridge network setup does not support multicast. The easiest way to achieve this is to use
"host networking".
Connecting Gerbera to your network via the "macvlan" driver should work, but remember you will not be
able to access the container from the docker host with this method by default.

# Transcoding Tools
Transcoding tools are made available in a separate image with the `-transcoding` suffix,
e.g. `gerbera/gerbera:3.1.1-transcoding`. It includes tools such as ffmpeg and vlc.

# Debug build
A full debug build is available as separate image whit the `-debug` suffix,
e.g. `gerbera/gerbera:3.1.1-debug`. It is building most libraries and tools based on the latest supported versions.

# Examples

## Serve some files via a volume
```console
$ docker run \
    --name some-gerbera \
    --network=host \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.1.1
```

or for those that prefer docker-compose:

```console
---
version: "3.1.1"
services:
  gerbera:
    image: gerbera/gerbera
    container_name: gerbera
    network_mode: host
    volumes:
      - ./gerbera-config:/var/run/gerbera
      - /some/files:/mnt/content:ro

volumes:
  gerbera-config:
    external: false
```

The directory `/mnt/content` is automatically scanned for content by default.
Host networking enables us to bypass issues with broadcast across docker bridges.

You may place custom JavaScript files in the directory `/mnt/customization/js`.
Every time Gerbera creates `/var/run/gerbera/config.xml`, the shell script
`/mnt/customization/shell/gerbera_config.sh` (if existing) will be executed.

## Provide your own config file
```console
$ docker run \
    --name another-gerbera \
    --network=host \
    -v /some/files:/mnt/content:ro \
    -v /some/path/config.xml:/var/run/gerbera/config.xml \
     gerbera/gerbera:3.1.1
```

## Keep config, database and runtime files outside
```console
$ docker run \
    --name another-gerbera \
    --network=host \
    -v /some/files:/mnt/content:ro \
    -v /some/path:/var/run/gerbera \
     gerbera/gerbera:3.1.1
```
Make sure that the container user has write access the the config path.

## Overwrite default ports

In cases (e.g. running multiple gerbera containers with different versions) you can override the exported ports

```console
$ docker run \
    --name another-gerbera \
    --network=host \
    --expose <your-port>:<your-port> \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.1.1 gerbera --port <your-port> --config /var/run/gerbera/config.xml
```

## Overwrite default user and group id

In cases (e.g. running multiple gerbera containers with different versions) you can override the exported ports

```console
$ docker run \
    --name another-gerbera \
    --network=host \
    --env UID=<newuid> \
    --env GID=<newgid> \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.1.1 gerbera --config /var/run/gerbera/config.xml
```

# Build Variables

There are some variables in Dockerfile which allow overwriting the defaults if you build the container by yourself

```console
$ cd /src
$ git clone https://github.com/gerbera/gerbera.git
$ cd /src/gerbera
$ docker build \
    --build-arg IMAGE_USER=grbr1 \
    --build-arg IMAGE_GROUP=grbr1 \
    --build-arg IMAGE_UID=1969 \
    --build-arg IMAGE_GID=1969 \
    --build-arg IMAGE_PORT=50500 \
    -t gerbera .
```

## BASE_IMAGE

Use a different base image for container. Changing this may lead to build problems if the required packages are not available.

- Default: alpine:3.20

## IMAGE_USER, IMAGE_GROUP

Set a different user/group name in the image to match user/group names on your host

- Default: gerbera

## IMAGE_UID, IMAGE_GID

Set a different user/group id in the image to match user/group ids on your host

- Default: 1042

## IMAGE_PORT

Change the port of gerbera in the image so you don't have to overwrite the port settings on startup.

- Default: 49494
