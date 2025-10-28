#!/bin/bash
set -e

if [ ! "$RSAKEY" == '' ]; then
    mkdir -p /home/bouncer/.ssh/
    echo "ssh-rsa $RSAKEY" > /home/bouncer/.ssh/authorized_keys
    #sed -i 's/Port 22\n/Port 22123\n/1' /etc/ssh/sshd_config
    sed -i "s/^.*PasswordAuthentication.*$/PasswordAuthentication no/" /etc/ssh/sshd_config \
    && sed -i "s/^.*X11Forwarding.*$/X11Forwarding yes/" /etc/ssh/sshd_config \
    && sed -i "s/^.*X11UseLocalhost.*$/X11UseLocalhost no/" /etc/ssh/sshd_config \
    && grep "^X11UseLocalhost" /etc/ssh/sshd_config || echo "X11UseLocalhost no" >> /etc/ssh/sshd_config
    service ssh start
fi

if [ "$1" = 'bouncer' ]; then

    echo 'Starting init script'
    #printenv
    if [ "$APP_EFS_ROOT" = '' ]; then
        export APP_EFS_ROOT=/efs
    fi

    if [ "$CONFDIR" = '' ]; then
        export CONFDIR=/etc/confd
    fi

    if [ ! -d "$APP_EFS_ROOT/models" ]; then
            chown 1101:1102 $APP_EFS_ROOT
            echo 'creating $APP_EFS_ROOT/models'
            mkdir -p $APP_EFS_ROOT/models
            chmod -R g+rws $APP_EFS_ROOT/models
    fi

    if [ ! -d "$APP_EFS_ROOT/queue" ]; then
            echo 'creating $APP_EFS_ROOT/queue'
            chown 1101:1102 $APP_EFS_ROOT
            mkdir -p $APP_EFS_ROOT/queue
            chmod -R g+rws $APP_EFS_ROOT/queue
    fi
    
    if [ ! -d "$APP_EFS_ROOT/config" ]; then
            echo 'creating $APP_EFS_ROOT/config'
            mkdir -p $APP_EFS_ROOT/config/3drepobouncer
    fi

    exec gosu bouncer bash -c 'cd ~/3drepobouncer/tools/bouncer_worker && yarn run-worker'
fi

exec "$@"
