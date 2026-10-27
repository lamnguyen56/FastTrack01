# FastTrack01 Yocto Tools

Scripts and configuration templates for the FastTrack01 **Yocto Scarthgap** build on Ubuntu 22.04 / WSL2. For Windows and WSL installation, see [the setup guide](../Yocto_WSL2_Setup_Guide.md).

## Directory layout

```text
FastTrack01/
└── Yocto/
    ├── meta-usb-video-player/  # Project layer, included in the project repository
    ├── tools/                 # Scripts and templates, included in the project repository
    ├── poky/                  # External layers managed by init-build.sh
    ├── meta-openembedded/
    ├── meta-raspberrypi/
    ├── meta-qt5/
    ├── build/                 # Generated build directory
    ├── yocto_downloads/       # Local source cache
    └── yocto_sstate/          # Local shared-state cache
```

External layers, build output and caches are excluded from the project repository. Keep the scripts and templates in this directory beside the layer directories. Project paths are derived from the location of the scripts; the username is not hard-coded.

## First-time setup

Run the host setup script as your normal Ubuntu user. It requests sudo access for package installation and locale configuration:

```bash
cd ~/projects/FastTrack01/Yocto
bash tools/setup-wsl-build-host.sh
```

Then initialize the build environment and build the image:

```bash
source tools/init-build.sh
bitbake-layers show-layers
bitbake -p
bitbake usb-image
```

Use `source`, not `bash`, for `init-build.sh`: the environment must remain available in the current shell. Successful initialization changes the current directory to the build directory and exports `BUILDDIR`.

## Layer initialization and pinned commits

`init-build.sh` clones missing external repositories, fetches existing repositories, checks origin URLs and verifies that each pinned commit belongs to `origin/scarthgap`. It attaches the local `scarthgap` branch to the pinned commit, including when HEAD was previously detached. It verifies both the branch name and commit before continuing.

| Repository | Pinned commit |
|---|---|
| poky | `4fb7016a07507683359ebd620f1951fd5ea44bfa` |
| meta-openembedded | `0f00f8b9a21950640da8c5707343e5540133f86e` |
| meta-raspberrypi | `018d44467c4f09eb63f8ea5e86e85d755250cf89` |
| meta-qt5 | `916d37e38077850ff5a0ac4fce6f2a55ba5b2a86` |

The commit variables in `init-build.sh` are the source of truth. `meta-usb-video-player` is maintained in the FastTrack01 repository and is not cloned or pinned separately by this script.

Initialization stops if an external repository has modified/untracked files, its origin URL differs from the expected URL, or its local `scarthgap` branch contains commits outside the fetched upstream history. A detached HEAD outside that history also causes an error. Preserve and reconcile local work before retrying; do not force-reset it to bypass these checks.

Fetching requires network access even when the repositories already exist. Pinning prevents upstream updates from silently changing the selected build revisions.

## Build directory

The default directory is `Yocto/build`. To use another build directory directly under `Yocto`, pass its name:

```bash
cd ~/projects/FastTrack01/Yocto
source tools/init-build.sh build-test
```

Absolute paths and nested directory names are not supported by this script because the layer template uses paths relative to the build directory's parent. Choose a dedicated build directory name; do not use an existing source or tools directory.

Whenever you open a new terminal, initialize the environment again:

```bash
cd ~/projects/FastTrack01/Yocto
source tools/init-build.sh
```

## Scripts and templates

| File | Purpose |
|---|---|
| `setup-wsl-build-host.sh` | Installs build host packages and configures the locale on Ubuntu 22.04. Run as a normal user, not with sudo. |
| `init-build.sh` | Verifies external layers, sources `oe-init-build-env`, then applies both configuration templates. |
| `setup-bblayers.sh` | Validates the configured layer directories and copies the entire `bblayers.conf.template` to the active build directory. |
| `setup-build.sh` | Adds or replaces the managed configuration block in the active `local.conf`, preserving content outside the block. Rejects incomplete, duplicated or reversed markers. |
| `bblayers.conf.template` | Defines the poky, OpenEmbedded, Raspberry Pi, Qt5 and project layer paths relative to `TOPDIR`. |
| `local.conf.template` | Defines machine, parallelism, cache paths, fetch settings, image formats and init manager. |
| `ssh.sh` | Connects as root using an explicit IPv4 address or resolves `PI_MAC` from Ubuntu's IPv4 neighbor table. Supports `--ip` and `PI_IP`. |
| `deploy-process.sh` | Builds and deploys the ARM64 executable for `usb`, `can` or `app`, with backup, service restart and attempted rollback. |

Automatic MAC lookup requires an existing entry in Ubuntu's IPv4 neighbor table; it does not scan the subnet or consult router DHCP leases. An explicit IP bypasses discovery. See [Wi-Fi provisioning and troubleshooting](../meta-usb-video-player/recipes-connectivity/wifi/README.md) for target setup.

## Updating build configuration

Edit the templates in `tools/` so configuration survives build-directory recreation. With a build environment already initialized, apply changes without fetching the layer repositories again:

```bash
bash "$BUILDDIR/../tools/setup-bblayers.sh"
bash "$BUILDDIR/../tools/setup-build.sh"
```

`setup-bblayers.sh` replaces the complete `bblayers.conf`; manual changes there will be overwritten. `setup-build.sh` replaces only the block delimited by:

```text
# >>> YOCTO_USER_CONFIG (managed by setup-build.sh) >>>
# <<< YOCTO_USER_CONFIG (managed by setup-build.sh) <<<
```

Do not edit settings inside that generated block directly. Reapplying it does not append duplicate blocks. Settings outside the block are retained, so review them for conflicting assignments.

## Local caches and disk space

No cache download, shared-cache repository or ZIP extraction is required. BitBake creates and populates these directories locally:

```bitbake
DL_DIR = "${TOPDIR}/../yocto_downloads"
SSTATE_DIR = "${TOPDIR}/../yocto_sstate"
```

The first build downloads sources and creates reusable task outputs. Later builds reuse valid local cache entries. Recreating a dedicated build directory does not remove these sibling cache directories. Do not delete existing caches merely to initialize a new build.

Check free space before and during builds:

```bash
df -h / /mnt/c
```

Also check the Windows drive containing the distro's `ext4.vhdx` if it is not on C. Free space reported inside the Linux filesystem is not a guarantee of available space on the Windows host drive.

## Build output

The image deployment directory is:

```text
Yocto/<build-directory>/tmp/deploy/images/<MACHINE>/
```

The current machine in `local.conf.template` is `raspberrypi3-64`. After a successful image build:

```bash
ls "$BUILDDIR/tmp/deploy/images/raspberrypi3-64/"
```

Installing packages in the image does not by itself configure the CAN controller, USB automount or multimedia/display backend. Those platform features require their own configuration and target validation.

## SSH access

These examples run from the repository's `Yocto` directory on Ubuntu:

```sh
cd ~/projects/FastTrack01/Yocto
bash tools/ssh.sh 192.168.1.30
```

The IP is an example; confirm the Pi's current address on its serial console with
`ip -4 addr show wlan0`. The target must already have networking and OpenSSH.
Set a root password with `passwd` on the Pi or provision an SSH public key.

To resolve the address by MAC instead:

```sh
bash tools/ssh.sh
bash tools/ssh.sh --ip
PI_MAC=b8:27:eb:db:99:c4 bash tools/ssh.sh --ip
```

The default MAC is `b8:27:eb:db:99:c4`; use the actual wlan0 MAC of your board.
`--ip` prints only the resolved address. Both colon and hyphen MAC notation are
accepted. The script reads Ubuntu's cached IPv4 neighbors with `ip neigh`; it does
not scan the subnet or consult the router's DHCP leases. If the Pi is not listed,
use its known address directly or check the router's DHCP client list.

If the Pi is missing from the neighbor table but its current IP is known, contact
it once to populate the cache, then retry. Substitute the IP shown for your Pi:

```sh
ping -c 1 192.168.1.30
bash tools/ssh.sh
```

Or bypass discovery with an explicit address or `PI_IP`:

```sh
bash tools/ssh.sh 192.168.1.30
PI_IP=192.168.1.30 bash tools/ssh.sh
```

An absent neighbor-table entry does not mean the Pi is offline; the Pi may simply
not have communicated with this computer recently.

## Deploy one process

Run from the initialized Yocto **build directory**, where BitBake is available:

```sh
cd ~/projects/FastTrack01/Yocto
source poky/oe-init-build-env build

bash ../tools/deploy-process.sh usb 192.168.1.30
bash ../tools/deploy-process.sh can 192.168.1.30
bash ../tools/deploy-process.sh app 192.168.1.30
```

Using `oe-init-build-env` here reopens an existing configured build without
fetching external layers again. Use `init-build.sh` for initial setup.

| Selector | Recipe / executable | Unit |
|---|---|---|
| `usb` | `usb-monitor` | `usb-monitor.service` |
| `can` | `can-daemon` | `can-daemon.service` |
| `app` | `usb-video-player` | `usb-video-player.service` |

Full recipe names are accepted too. A destination without a username defaults
to root. The remote account must have UID 0; the script does not invoke sudo.

Place `--logs` before the selector to follow journal output after deployment:

```sh
bash ../tools/deploy-process.sh --logs app 192.168.1.30
```

Ctrl+C at the journal-following stage exits the log view; the service stays
running. Omitting the destination uses `ssh.sh --ip` after the build:

```sh
bash ../tools/deploy-process.sh --logs usb
PI_MAC=b8:27:eb:db:99:c4 bash ../tools/deploy-process.sh can
PI_IP=192.168.1.30 bash ../tools/deploy-process.sh app
```

SSH keys avoid repeated password prompts; the scripts use standard SSH
authentication and host-key checks.

### What deployment changes

The script builds the selected recipe, resolves its output path using `bitbake -e`,
checks AArch64, uploads to a temporary file, prepares a backup and replacement on
the target filesystem, stops the selected service and renames the new executable
into place. It starts the service and checks that it is active after two seconds.

For USB only, an active player is stopped first so it can release media files,
then started again. CAN and app updates do not stop unrelated services. A CAN
update needs can0 configured and available for its service startup check to pass.
An app update needs a supported display connected and its runtime fonts/plugins.

On errors, the script attempts to restore the old binary and previous service
state. This is best-effort rollback; loss of power/network can require manual
recovery. A successful two-second check is not a functional USB/CAN/video test.
The backup is `/usr/bin/<executable>.previous`, replaced on each deployment.

Only the executable is copied. QML embedded in the app's resource file is included,
but service units, `setup_can.sh`, libraries, fonts, firmware, kernel and device tree
must be deployed separately or through a rebuilt image. The target must already
have compatible dependencies. Flashing a new image replaces manual target changes.
