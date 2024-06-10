[Unit]
Description=${SYSTEMD_DESCRIPTION}
After=${SYSTEMD_AFTER_TARGET}
Wants=${SYSTEMD_WANTS_TARGET}

[Service]
Type=simple
User=gerbera
Group=gerbera
ExecStart=${CMAKE_INSTALL_PREFIX}/bin/gerbera -c /etc/gerbera/config.xml --syslog
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
