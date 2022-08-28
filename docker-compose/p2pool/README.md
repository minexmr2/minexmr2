Ubuntu 20.0.4 LTS
*****************

sudo apt install nano

cp example.env .env
nano .env

sudo apt install screen

screen -S dc-p2pool
Ctrl+A+D
screen -ls
screen -r dc-p2pool

sudo apt install docker.io
sudo apt install docker-compose

sudo docker-compose build --force-rm --no-cache
sudo docker-compose up
Ctrl+C
sudo docker-compose down

Ctrl+A+D
