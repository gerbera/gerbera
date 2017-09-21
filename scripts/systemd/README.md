# Load Gerbera as a System Daemon

This readme outlines how to add the Gerbera runtime
as a system daemon using the `systemd`.

### Prerequisites

You installed `gerbera` to `/usr/local/bin/gerbera`

> If you don't know the path try `which gerbera`


### Create a Gerbera System User

We should run Gerbera as a separate user to avoid vulnerabilities in
exposing root access.

Here is a way to create a system user in linux command line

```
$ sudo useradd --system gerbera
```

Verify that the `gerbera` user was created

```
$ id -u gerbera
```
> Returns the user id of the user


### Create a Gerbera Configuration File

Create a valid Gerbera `config.xml` in the destination
of your choice.  Store this file accessible to the gerbera user

Here we will use the default location `/etc/gerbera`.

```
$ sudo mkdir /etc/gerbera
$ sudo touch /etc/gerbera/config.xml
$ sudo chown -Rv gerbera:gerbera /etc/gerbera
```

### Enable Systemd Daemon

1. Copy the `systemd` script to `/etc/systemd/system`

     ```
     $ sudo cp /gerbera/scripts/systemd/gerbera.service /etc/systemd/system/
     ```
2. Set the proper run permissions on the daemon file

     ```
     $ sudo chmod 664 /etc/systemd/system/gerbera.service
     ```
3. Modify the `gerbera.service` file to suit your system setup

     - Change the path of the `gerbera` binary to match your system

     For Example

     ```
     [Unit]
     Description=Gerbera Media Server
     After=network.target

     [Service]
     Type=simple
     User=gerbera
     Group=gerbera
     ExecStart=/usr/local/bin/gerbera -c /etc/gerbera/config.xml
     Restart=on-failure
     RestartSec=5

     [Install]
     WantedBy=multi-user.target
     ```
     > Ensure the `User` and `Group` value exist within your system

4. Notify `systemd` that a new gerbera.service file exists by executing the following command:

     ```
     $ sudo systemctl daemon-reload
     ```
5. Start up the daemon

    ```
    $ sudo systemctl start gerbera
    ```

### Success

Check the status of gerbera.  You should see success similar to below

```
$ sudo systemctl status gerbera

● gerbera.service - Gerbera Media Server
   Loaded: loaded (/etc/systemd/system/gerbera.service; disabled; vendor preset: disabled)
   Active: active (running) since Wed 2017-09-20 19:48:44 EDT; 47s ago
 Main PID: 4818 (gerbera)
    Tasks: 12 (limit: 4915)
   CGroup: /system.slice/gerbera.service
           └─4818 /usr/local/bin/gerbera -c /etc/gerbera/config.xml
```


### Troubleshooting

If for some reason the service fails to start.  You can troubleshoot the behavior
by starting gerbera from the shell

```
$ su gerbera
Password:
bash-4.4$  /usr/local/bin/gerbera -c /etc/gerbera/config.xml

2017-09-20 19:54:47    INFO: Gerbera UPnP Server version 1.1.0_alpha - http://gerbera.io/
2017-09-20 19:54:47    INFO: ===============================================================================
2017-09-20 19:54:47    INFO: Gerbera is free software, covered by the GNU General Public License version 2
2017-09-20 19:54:47    INFO: Copyright 2016-2017 Gerbera Contributors.
2017-09-20 19:54:47    INFO: Gerbera is based on MediaTomb: Copyright 2005-2010 Gena Batsyan, Sergey Bostandzhyan, Leonhard Wimmer.
2017-09-20 19:54:47    INFO: ===============================================================================
2017-09-20 19:54:47    INFO: Loading configuration from: /etc/gerbera/config.xml
2017-09-20 19:54:47    INFO: Checking configuration...

```

