# Hi there! Now minexmr2.com pool uses decentralized p2pool as a hashrate-liquidity provider 👋

This is [minexmr2.com](https://minexmr2.com) Monero mining pool implementation based
on [jtgrassie monero-pool](https://github.com/jtgrassie/monero-pool).
Note though, nearly 50% of source code in added/rewritten.
Thus all the build instructions are there.
Just instead of 'make release', type 'make release_p2pool', or see build instructions below.

- 🔭 FULL support of [SChernykh p2pool](https://github.com/SChernykh/p2pool) has been implemented!
Now minexmr2.com pool uses decentralized p2pool as a hashrate-liquidity provider.
If you mine with us, all your hashrate is contributed to p2pool, helping to empower decentralization.
But all the statistics, billing and fault-tolerant logic remains in minexmr2.com. You have nothing to build, deploy, upgrade and maintain
like in your own p2pool instance!

## Latest news

⚡ Oct 18 2022: [minexmr2.com](https://minexmr2.com) pool is rebuilt against
[Monero Fluorine Fermi, Point Release 1.2](https://github.com/monero-project/monero/releases/tag/v0.18.1.2)
and [p2pool v2.4](https://github.com/SChernykh/p2pool/releases/tag/v2.4).

⚡ Sep 22 2022: [minexmr2.com](https://minexmr2.com) status online and fully operational; blocks are being found by local
p2pool instance, approx. 1.62 blocks/hour. Detailed info at [minexmr2.com](https://minexmr2.com).

## Compiling from source

### Dependencies

The build system requires the Monero source tree to be cloned and compiled.
Follow the
[instructions](https://github.com/monero-project/monero#compiling-monero-from-source)
for compiling Monero, then export the following variable:

```bash
export MONERO_ROOT=/path/to/cloned/monero
```

Replacing the path appropriately.

Beyond the Monero dependencies, the following extra libraries are also required
to build the pool:

- liblmdb
- libevent
- json-c
- uuid

As an example, on Ubuntu, these dependencies can be installed with the following
command:

```
sudo apt-get install liblmdb-dev libevent-dev libjson-c-dev uuid-dev
```

P2pool should be compiled too, of course! Follow the [instructions](https://github.com/SChernykh/p2pool#build-instructions).

### Compile

After installing all the dependencies as described above, to compile the pool as
a release build, run:

```
make release_p2pool
```

The application will be built in `build/release_p2pool/`.

Optionally you can compile a debug build by simply running:

```
make debug_p2pool
```

Debug builds are output in `build/debug_p2pool/`.

## Configuration

During compilation, a copy of [pool.conf](./pool.conf) is placed in the output
build directory. Edit this file as you see fit. When running, the pool will first look for this file in the same directory as the
pool binary, then in the current users home directory. The configuration options
should all be self explanatory.

It is highly recommended if you run separate UPSTREAM and TRUSTED instances of minexmr2 pool under different user accounts of your Linux OS
or even different hardware Linux servers to maintain good security.

Note, all the networking between instances or servers is performed via [OpenVPN](https://github.com/OpenVPN/openvpn) tunnel!

UPSTREAM instance communicates with monerod and p2pool servers, while TRUSTED instance communicates with monerod and monero-wallet-rpc servers.
UPSTREAM instance's purpose is to listen for miner connections and redirect them to p2pool server, while recording billing info.
Billing info recorded is being sent to TRUSTED instance that performs all the payment processing via calls to monero-wallet-rpc server.

[pool-edge.conf](./pool-edge.conf) is a typical configuration file for UPSTREAM instance. Don't forget to rename it to pool.conf in your instance's folder.

[pool-trusted.conf](./pool-trusted.conf) is a typical configuration file for UPSTREAM instance. Don't forget to rename it to pool.conf in your instance's folder.

Important! Your pool's wallet must contain some XMR balance, for example 0.05XMR, and the whole line in configuration file id dedicated to that:

```
p2pool-safe-balance = 0.05
```

If something goes wrong, a built-in watchdog routine preserves wallet balance to decrease below 0.05XMR (in this example).

### You must start executables in the following order

UPSTREAM

1. monerod
2. p2pool
3. monero-pool as Linux user/server named "monero-pool-upstream"

TRUSTED

4. monero-wallet-rpc
5. monero-pool as Linux user/server named "monero-pool-trusted"

Shutdown should be performed in reversed order.

## Donations

You donations help to evacuate my family from danger near-war zone of Ukraine.

Donate XMR: 4AoKL73JHbJWSws7exQyxMYwgEvqkrpxX6hiLwLA8YXyESLHVCk7r9djR8fmeaDMSABCryvu1PjUGDYmQnkBNHum9NhDpbW

Donate BTC: bc1qmg0jc8lnsqx82096cknfn9a8q6e723gm3kw23t

<!--
**minexmr2/minexmr2** is a ✨ _special_ ✨ repository because its `README.md` (this file) appears on your GitHub profile.

Here are some ideas to get you started:

- 🔭 I’m currently working on ...
- 🌱 I’m currently learning ...
- 👯 I’m looking to collaborate on ...
- 🤔 I’m looking for help with ...
- 💬 Ask me about ...
- 📫 How to reach me: ...
- 😄 Pronouns: ...
- ⚡ Fun fact: ...
-->
