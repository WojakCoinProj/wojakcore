# Docker image for WojakCoin Core

The official multi-platform Docker image is maintained in a separate repo:

**https://github.com/WojakCoinProj/docker-wojakcoin-core**

Hub: **https://hub.docker.com/r/reallyshadydev/wojakcoin-core**

```bash
docker pull reallyshadydev/wojakcoin-core:1.21.1.0
# or
docker pull reallyshadydev/wojakcoin-core:latest
```

## What is included

| Platform | Docker image | Notes |
|----------|--------------|--------|
| Linux x86_64 | ✅ | `wojakcoind`, `wojakcoin-cli`, `wojakcoin-tx` |
| Linux aarch64 | ✅ | same |
| **macOS** | ❌ **not in Docker** | use GitHub release macOS zips / `.app` |
| **Windows** | ❌ **not in Docker** | use GitHub release Windows zip |
| **Qt GUI** | ❌ **not in Docker** | desktop builds only |

macOS users should download from:

https://github.com/WojakCoinProj/wojakcore/releases/tag/1.21.1.0

- `wojakcore-1.21.1.0-macos-arm64-qt.app.zip`
- `wojakcore-1.21.1.0-macos-arm64-bins.zip`

## Linux release assets the image needs

| Platform in image | Release zip |
|-------------------|-------------|
| `linux/amd64` | `wojakcore-1.21.1.0-linux-x86_64.zip` |
| `linux/arm64` | `wojakcore-1.21.1.0-linux-aarch64.zip` |

## Mainnet seeds

The Docker entrypoint adds mainnet `-addnode` bootstrap peers so containers
connect reliably when in-container DNS seed lookup fails. See
`docker-wojakcoin-core` README (`WOJAKCOIN_SKIP_SEEDS=1` to disable).

## Miner note (1.21.1.0)

Upgrade before mainnet height **175,000**. See
https://github.com/WojakCoinProj/wojakcore/releases/tag/1.21.1.0
