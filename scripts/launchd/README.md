## Create new Launch Agent

> Use the `gerbera.io.plist` as a starting point.

## Save Launch Agent

Save to user's launch agent path -->

`~/Library/LaunchAgents/gerbera.io.plist`

## Load the Launch Agent

```
$ launchctl load ~/Library/LaunchAgents/gerbera.io.plist
```

## Start the Launch Agent

```
$ launchctl start gerbera.io
```

## Stop the Launch Agent

```
$ launchctl stop gerbera.io
```