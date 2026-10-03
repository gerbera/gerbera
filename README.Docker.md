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
e.g. `gerbera/gerbera:3.3.0-transcoding`. It includes tools such as ffmpeg and vlc.

# Debug build
A full debug build is available as separate image with the `-debug` suffix,
e.g. `gerbera/gerbera:3.3.0-debug`. It is building most libraries and tools based on the latest supported versions.

# Environment Variables

All environment variables are optional.

| Variable                                | Default | Description                                                                                            |
|-----------------------------------------|---------|--------------------------------------------------------------------------------------------------------|
| `PUID`                                  | `1042`  | User id gerbera runs as. Files and folders it creates are owned by this user, see [below](#overwrite-default-user-and-group-id) |
| `PGID`                                  | `1042`  | Group id gerbera runs as                                                                               |
| `TZ`                                    | `UTC`   | Time zone, e.g. `Australia/Perth`                                                                      |
| `IMAGE_PORT`                            | `49494` | Port gerbera listens on, when the default command is used                                              |
| `MARIADB_TLS_DISABLE_PEER_VERIFICATION` |         | Set to `1` to skip the certificate check for MySQL/MariaDB, see [below](#avoid-certificate-check-with-mysqlmariadb) |
| `UID`, `GID`                            |         | Deprecated names of `PUID` and `PGID`, still honoured. `PUID`/`PGID` take precedence                   |

# Examples

## Serve some files via a volume
```console
$ docker run \
    --name some-gerbera \
    --network=host \
    --env PUID=1000 \
    --env PGID=1000 \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.3.0
```

or for those that prefer docker-compose:

```yaml
services:
  gerbera:
    image: gerbera/gerbera:3.3.0
    container_name: gerbera
    network_mode: host
    environment:
      # all optional, see "Environment Variables"
      - PUID=1000
      - PGID=1000
      # - TZ=Australia/Perth
      # - IMAGE_PORT=49494
      # - MARIADB_TLS_DISABLE_PEER_VERIFICATION=1
    volumes:
      - ./gerbera-config:/var/run/gerbera
      - /some/files:/mnt/content:ro
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
     gerbera/gerbera:3.3.0
```

## Keep config, database and runtime files outside
```console
$ docker run \
    --name another-gerbera \
    --network=host \
    --env PUID=1000 \
    --env PGID=1000 \
    -v /some/files:/mnt/content:ro \
    -v /some/path:/var/run/gerbera \
     gerbera/gerbera:3.3.0
```
On startup `/some/path` and everything in it is handed to `PUID:PGID`, so set them to the host user that should own
these files.

## Overwrite default ports

In cases (e.g. running multiple gerbera containers with different versions) you can override the exported ports

```console
$ docker run \
    --name another-gerbera \
    --network=host \
    --expose <your-port>:<your-port> \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.3.0 gerbera --port <your-port> --config /var/run/gerbera/config.xml
```

## Overwrite default user and group id

In cases you want to map the gerbera user to a local user id you can set the environment variables `PUID` and `PGID`.
Run `id` on the host to find the ids of your user.

```console
$ docker run \
    --name another-gerbera \
    --network=host \
    --env PUID=<newuid> \
    --env PGID=<newgid> \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.3.0 gerbera --config /var/run/gerbera/config.xml
```

or with docker-compose:

```yaml
services:
  gerbera:
    image: gerbera/gerbera:3.3.0
    network_mode: host
    environment:
      - PUID=<newuid>
      - PGID=<newgid>
    volumes:
      - ./gerbera-config:/var/run/gerbera
      - /some/files:/mnt/content:ro
```

On startup the container changes the ids of the gerbera user and group, hands `/var/run/gerbera` to `PUID:PGID` and
then runs gerbera as that user, so everything it creates is owned by `PUID:PGID`. Ids that already exist in the image
are supported: e.g. `PGID=100` makes the existing `users` group the primary group of the gerbera user.
A `config.xml` you provide keeps its owner and is made readable for `PGID`.

This requires the container to start as root (the default). With `--user` the ids cannot be changed and gerbera runs
with the ids given there.

`UID` and `GID` are the older names of these variables. They still work but log a deprecation warning; if both are set,
`PUID` and `PGID` win.

## Avoid certificate check with MySQL/MariaDB

MariaDB connector assumes SSL/TLS encryption which might not be active on your database. To skip certificate check,
you have to set the environment variable `MARIADB_TLS_DISABLE_PEER_VERIFICATION`.

```console
$ docker run \
    --name another-gerbera \
    --network=host \
    --env MARIADB_TLS_DISABLE_PEER_VERIFICATION=1 \
    -v /some/files:/mnt/content:ro \
     gerbera/gerbera:3.3.0 gerbera --config /var/run/gerbera/config.xml
```

or with docker-compose:

```yaml
services:
  gerbera:
    image: gerbera/gerbera:3.3.0
    network_mode: host
    environment:
      - MARIADB_TLS_DISABLE_PEER_VERIFICATION=1
    volumes:
      - /some/files:/mnt/content:ro
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
