#!/bin/bash

cd ./var/www/
rm -f mx2_html.tar
tar cf mx2_html.tar ./html/
scp -P 26265 ./mx2_html.tar root@172.17.0.1:/var/www/mx2_html.tar
cd ../../
ssh -p 26265 root@172.17.0.1 'bash -s' < ./remote_www_pre.sh
ssh -p 26265 root@172.17.0.1 'sudo -u www-data -s' < ./remote_www.sh
ssh -p 26265 root@172.17.0.1 'bash -s' < ./remote_www_post.sh
