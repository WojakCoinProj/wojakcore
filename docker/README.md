# Docker image for WojakCoin Core

The official multi-platform Docker image is maintained in a separate repo:

**https://github.com/WojakCoinProj/docker-wojakcoin-core**

Hub: **https://hub.docker.com/r/reallyshadydev/wojakcoin-core**

```bash
docker pull reallyshadydev/wojakcoin-core:1.12.2.0
# or
docker pull reallyshadydev/wojakcoin-core:latest
```

## Release assets the image needs

The Dockerfile downloads these assets from the matching
[wojakcore GitHub Release](https://github.com/WojakCoinProj/wojakcore/releases):

| Platform in image | Release zip |
|-------------------|-------------|
| `linux/amd64` | `wojakcore-linux-x86_64.zip` |
| `linux/arm64` | `wojakcore-linux-aarch64.zip` |

Each zip must contain the platform-labeled binaries:

- `wojakcoind-linux-x86_64` / `wojakcoind-linux-aarch64`
- `wojakcoin-cli-linux-x86_64` / `wojakcoin-cli-linux-aarch64`
- `wojakcoin-tx-linux-x86_64` / `wojakcoin-tx-linux-aarch64`

(GUI / macOS / Windows zips are not used by the Docker image.)

When cutting a new release, keep those two Linux zips published, then update
`WOJAKCOIN_VERSION`, `WOJAKCOIN_RELEASE`, and the zip SHA256 values in
`docker-wojakcoin-core/Dockerfile` and push that repo to rebuild Hub tags.
