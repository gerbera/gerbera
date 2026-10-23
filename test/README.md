# Gerbera Test

`PlaylistUpdateIntegration` additionally exercises a real Gerbera process on Linux
when JavaScript, inotify, Python 3 and TagLib or FFmpeg are available. It uses temporary generated WAV
files and M3U/M3U8/PLS/ASX playlists, checks both native playlist views through
ContentDirectory and downloads their complete resources. The sequence covers
read-only/writable close, media edits/replacement, playlist edits/repetitions,
rescans, restart, addition and deletion, with both `mt` and `grb` imports.
It does not use an existing library or configuration. A usable IPv4 network
interface is needed for the fixture's UPnP listener.

For a focused run with retained diagnostic files:

```sh
python3 test/content/test_playlist_updates.py --binary build/gerbera \
    --source-root "$PWD" --work-directory /tmp/grb-pl --debug
```

Use a fresh work directory for each run. Omit `--work-directory` for automatic
temporary-file cleanup.

### Integration process readiness and diagnostics

`test/content/integration_runtime.py` owns process start, restart, shutdown,
condition polling, HTTP transport and temporary files. Gerbera selects its own
port (`--port 0`); the test does not release a guessed port and race the server
to bind it. Each start uses a fresh log and endpoint. The helper waits for
`Service status: READY=1`, the existing lifecycle notification emitted after
`server->init()` and `server->run()` complete, also when systemd is unavailable.
It then verifies the fixture's UDN, a valid `GetSystemUpdateID` response and a
root Browse response. An empty catalog is a valid ready server.

READY means the server has started, not that background imports are complete.
The database and ContentDirectory setup precede it, but filesystem monitoring
and import jobs are asynchronous, some scan configuration errors are logged
without aborting startup, and CLI server actions run after the notification.
Content checks retain the original playlist waiter's retry policy:
`OSError` (including HTTP errors), `ValueError`, `AssertionError` and XML parse
errors may converge within the existing 20-second content deadline. This
matters when an asynchronous scan invalidates a just-listed object or resource.
Every retry reads and validates the expected views again; successful startup
alone never satisfies a content check. A persistent error fails at the deadline
with its last error and server diagnostics. Process exits and unexpected
programming errors fail immediately. There are no whole-test retries, skipped
content assertions or fixed extra sleeps to guess startup completion. The
readiness change does not alter Gerbera's UPnP error mapping.
A response interrupted before its declared Content-Length raises
`http.client.IncompleteRead`. Content polling retries this specific additional
case within the same deadline, then validates the complete response and expected
content again. Persistent truncation fails with diagnostics. Other HTTP protocol
exceptions are not added to the retry policy. A real HTTP regression closes the
connection before the declared body is sent and verifies recovery and timeout.
`--startup-timeout SECONDS` changes the default 30-second lifecycle deadline;
individual HTTP requests have a five-second timeout. A timeout or early exit
includes the endpoint and server log. Owned process groups are stopped and
reaped before a restart, and stale readiness from an earlier start is ignored.

Before an inotify scenario changes any files, it additionally waits for the
owned process's kernel watches on the source, audio and playlist directories.
Linux `/proc/PID/fdinfo` must report their device/inode identities and required
close-write, move, create and delete event masks. This covers asynchronous
watch registration after READY; it does not wait for the entire import queue
to become idle. Timed-scan tests retain their separate scan/result assertions.

The shared HTTP client binds a source port distinct from the destination
before connecting. This prevents Linux TCP self-connections during a server's
startup or exit window, rather than catching the resulting invalid HTTP reply.

Successful automatic workspaces are removed. Failed workspaces retain logs,
generated configuration, synthetic files and fixture database; their path and
log tails are printed in CTest output. Set `GRB_TEST_ARTIFACTS` to choose their
parent directory. The compiler CI jobs upload retained failure evidence.
Explicit `--work-directory` directories are never overwritten or removed.

The standard-library regression suite is registered as `IntegrationRuntime`
with CTest and can also run without a Gerbera installation:

```sh
python3 test/content/test_integration_runtime.py
```

It covers delayed readiness after port binding, early exit, missing readiness,
wrong identity, empty catalogs, callback failures, restart generations,
self-connection prevention and failure evidence retention. This infrastructure
does not promise to eliminate arbitrary application races or future filesystem
events; each feature test must still assert its required result.

The general regression cases use only the Python standard library. Process
fixtures require POSIX; kernel-watch cases require Linux. Their unittest skip
reasons are explicit. CMake reports when Python is unavailable. The real
`PlaylistUpdateIntegration` remains restricted to Linux with JavaScript,
inotify and a supported media metadata reader; it is not enabled on platforms
without those features. Compiler/Conan, container, Alpine and native OS CI jobs
must exercise their applicable subset in their own execution environment.
Passing a development-container run alone does not certify the other runners.

This file outlines how the Gerbera system uses the **GoogleTest**
framework to provide unit testing for the C++ code-base.

## Required Software

- CMake ^3.9
- GoogleTest

## Enable Testing

The CMake build enables testing using the `WITH_TESTS` flag

```text
$ cmake ../gerbera -DWITH_TESTS=1
```

## Running All Tests

Build the project and run all the tests

```
$ make && make test
```

The tests output looks similar to below:

```
Running tests...
Test project /development/gerbera/build
    Start 1: testdictionary
1/2 Test #1: testdictionary ...................   Passed    0.02 sec
    Start 2: testruntime
2/2 Test #2: testruntime ......................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.05 sec

```

## Running Specific Test

You can run specific tests by using `ctest`. Below is an example of running the
**testdictionary** test only.


```
$ ctest -R testdictionary
Test project /development/gerbera/build
    Start 1: testdictionary
1/1 Test #1: testdictionary ...................   Passed    0.02 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.03 sec
```

## Creating a New Test

Adding a new test to Gerbera is easy.  The process amounts to a few steps:

1. Create a new feature test folder similar to `/test/test_myfeature`
2. Create `CMakeLists.txt` within your new folder.
3. Create a `main.cc` Google Test file
4. Add your files to your `CMakeLists.txt` within your `/test/test_myfeature` folder
4. Add sub-directory `test_myfeature` to the parent `/test/CMakeLists.txt` file

## XML playlist integration

Native `XmlPlaylistTest` cases check WPL/XSPF structure, namespace resolution,
URI handling and the explicit fixture expectations. `XmlPlaylistIntegration`
uses temporary media and real Gerbera processes in both import modes; it checks
both playlist views, complete resources, legacy formats, rescans and restart.
The integration case requires Linux, JavaScript, FFmpeg support and Python 3.
It uses its own database, webroot and SQL paths; installation is not required.

```
ctest --test-dir build -R XmlPlaylist --output-on-failure
python3 test/content/test_xml_playlists.py --binary build/gerbera --source-root . --work-directory /tmp/grb-x
```

The optional work directory must be new. Keep it after a manual run to inspect
server logs and fixture files. HTTP fixture URLs use example.invalid and are
never fetched. Real music libraries and server configuration are not modified.
