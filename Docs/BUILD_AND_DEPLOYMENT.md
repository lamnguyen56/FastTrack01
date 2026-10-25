# Build and Deployment

## Build host

Use Ubuntu 22.04/WSL2 and keep source/build files in the Linux filesystem. See the [Yocto WSL2 Setup Guide](../Yocto/Yocto_WSL2_Setup_Guide.md) for host installation.

Commands apply to the `main` branch and assume the repository is at `~/projects/FastTrack01`. Record the commit used for each build and test.

## First build

Run on WSL/Linux:

```sh
cd ~/projects/FastTrack01/Yocto
bash tools/setup-wsl-build-host.sh
source tools/init-build.sh
bitbake usb-image
```

`init-build.sh` prepares the pinned Scarthgap layers, configures the build and changes the shell directory to `build`. The target is `raspberrypi3-64`; the image inherits `core-image-minimal`.

Downloads and sstate are local caches populated during builds. A separately downloaded or shared cache is not required.

## Reopen an existing build

```sh
cd ~/projects/FastTrack01/Yocto
source poky/oe-init-build-env build
bitbake usb-image
```

Images are under `tmp/deploy/images/raspberrypi3-64/` as `.wic` or `.wic.gz`. Flash `usb-image`, rather than an older `core-image-minimal` output.

See [Yocto/tools/README.md](../Yocto/tools/README.md) for script options and configuration. Package lists and pinned revisions remain in their source files.

## Update one process

Run in the initialized build shell. Replace the example address with the Pi's current IP:

```sh
bitbake -C unpack usb-video-player
bash ../tools/deploy-process.sh app 192.168.1.30
```

`-C unpack` rebuilds the recipe's deployment artifact from the current source. This avoids copying a stale binary left in the staging directory after source revisions change.

Use selector `usb` for `usb-monitor` or `can` for `can-daemon`. Run `bitbake -C unpack <recipe>` for the matching recipe before deployment. Add `--logs` before the selector to follow the journal.

The script copies only the executable and restarts its service. Library, service-unit, script, kernel and device-tree changes require the corresponding files/packages or a rebuilt image.

## Verify on the Pi

```sh
systemctl status usb-monitor can-daemon usb-video-player --no-pager
journalctl -u usb-video-player.service -b -n 40 --no-pager -o cat
```

A running service does not prove that CAN or video works. Follow the [test checklist](SYSTEM_TESTING.md) after deployment.

See [wifi/README.md](../Yocto/meta-usb-video-player/recipes-connectivity/wifi/README.md) for Wi-Fi/SSH setup. Resetting Git does not restore boot configuration edited directly on the Pi; flashing an image can replace it.
