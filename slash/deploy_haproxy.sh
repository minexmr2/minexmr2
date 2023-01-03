#!/bin/bash

scp -P 26265 ./etc/haproxy/haproxy.cfg root@172.17.0.1:/etc/haproxy/haproxy.cfg
ssh -p 26265 root@172.17.0.1 'bash -s' < ./remote_haproxy.sh
