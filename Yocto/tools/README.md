# FastTrack01 Yocto Tools

Scripts and configuration templates for the FastTrack01 **Yocto Scarthgap** build on Ubuntu 22.04 / WSL2. For Windows and WSL installation, see [the setup guide](../Yocto_WSL2_Setup_Guide.md).

## Directory layout

```text
FastTrack01/
└── Yocto/
    ├── meta-fasttrack/         # Project layer, included in the project repository
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
bitbake ubs-image
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

The commit variables in `init-build.sh` are the source of truth. `meta-fasttrack` is maintained in the FastTrack01 repository and is not cloned or pinned separately by this script.

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
| `bblayers.conf.template` | Defines the poky, OpenEmbedded, Raspberry Pi, Qt5 and FastTrack layer paths relative to `TOPDIR`. |
| `local.conf.template` | Defines machine, parallelism, cache paths, fetch settings, image formats and init manager. |
| `ssh.sh` | Optional convenience helper that searches the Windows ARP table for a hard-coded Pi MAC address and connects as root using SSH. Update the MAC before use. |

The SSH helper depends on Windows PowerShell interoperability, an existing ARP entry and an SSH server on the target. It does not configure networking, install SSH on the image or discover devices that are absent from the ARP table.

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

## Deploy usb-monitor over Wi-Fi

From the initialized Yocto shell, run:

```sh
bash ../tools/deploy-usb-monitor.sh --logs root@192.168.1.50
```

Replace the IP with the Pi's DHCP address. The script builds the target recipe,
verifies AArch64, uploads the executable, saves a backup and restarts the service.
An active player is stopped during the update and restarted afterward. On errors,
the script attempts rollback. Omit `--logs` to return after deployment. The Pi must
already have SSH, Wi-Fi and compatible runtime libraries installed.

## Find the Pi by MAC

The deployment script reuses `tools/ssh.sh` when its destination is omitted:

```sh
bash ../tools/deploy-usb-monitor.sh --logs
```

The default MAC is `b8:27:eb:db:99:c4`. Override it if needed:

```sh
PI_MAC="b8:27:eb:db:99:c4" bash ../tools/deploy-usb-monitor.sh --logs
```

Use `bash ../tools/ssh.sh` for an interactive SSH session, or
`bash ../tools/ssh.sh --ip` to print only the discovered address. The resolver
reads Windows' ARP cache from WSL; it does not actively scan the subnet. The Pi
must be visible in that cache. Missing or ambiguous entries fail explicitly;
you can still pass an IP directly to deploy-usb-monitor.sh.
