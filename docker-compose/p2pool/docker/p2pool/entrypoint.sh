#!/bin/bash

P2POOL_CONFIG=""

if [[ -f "/p2pool/data/config.json" ]]; then
  P2POOL_CONFIG="--config /p2pool/data/config.json"
fi

echo "Starting p2pool with config "

exec "/p2pool/p2pool" \
${P2POOL_CONFIG} \
--host 172.18.0.1 \
--rpc-port 18081 --zmq-port 18083 \
"$@"
