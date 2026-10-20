# Yocto Build Environment Setup Guide

## 1. Install WSL2 with Ubuntu 22.04

Open PowerShell on Windows and run:

```powershell
wsl --install -d Ubuntu-22.04
```

Check the WSL version:

```powershell
wsl -l -v
```

Make sure Ubuntu-22.04 uses **WSL version 2**.

## 2. Create Projects Folder

Open an Ubuntu terminal and create the `projects` folder:

```bash
mkdir -p ~/projects
cd ~/projects
```

Example path:

```text
/home/minhlcn/projects
```

## 3. Clone Source Code Repository

Clone the repository into the `projects` folder:

```bash
cd ~/projects
git clone https://github.com/lamnguyen56/FastTrack01.git
```

The repository contains project scripts and the custom layer. The initialization script downloads external layers and checks them out at pinned commits on the `scarthgap` branch.

After cloning, verify that the following directory exists:

```text
~/projects/FastTrack01/Yocto/
```

## 4. Setup WSL Build Host

Go to the `tools` folder:

```bash
cd ~/projects/FastTrack01/Yocto/tools
```

Run the build host setup script:

```bash
./setup-wsl-build-host.sh
```

Wait for the script to finish.

## 5. Local Cache and Disk Space

You do not need to download caches from GitHub, copy caches from another machine, or extract ZIP files. BitBake creates and uses local caches according to `tools/local.conf.template`:

| Variable | Default directory | Contents |
|---|---|---|
| `DL_DIR` | `Yocto/yocto_downloads/` | Source files downloaded from upstream |
| `SSTATE_DIR` | `Yocto/yocto_sstate/` | Reusable task outputs |

The first build requires Internet access to download sources and usually takes longer. Subsequent builds reuse valid cache entries. Keep any existing caches; you do not need to delete them to follow this guide. The cache and build directories are excluded by `.gitignore` and are not stored in the project repository.

Before building, check disk space in Ubuntu:

```bash
df -h / /mnt/c
```

If the distribution's `ext4.vhdx` file is stored on another Windows drive, check that drive as well. Free space reported for `/` does not guarantee enough physical space on the Windows drive containing the VHDX. Monitor disk space during the build to avoid running out of storage.

## 6. Initialize Yocto Build Environment

Go to the `tools` folder:

```bash
cd ~/projects/FastTrack01/Yocto/tools
```

Source the build environment initialization script:

```bash
source init-build.sh
```

The script downloads or verifies layers at their pinned commits, initializes `Yocto/build`, and applies the templates for `bblayers.conf` and `local.conf`. Use `source` to preserve the BitBake environment in the current terminal.

If a layer contains modified files, untracked files, or local commits that fail the script's checks, preserve and resolve those changes before retrying. Do not reset or delete changes just to bypass an error.

## 7. Build Yocto Image

After the build environment has been initialized successfully, run:

```bash
bitbake usb-image
```

Wait for the build to finish. Build time depends on the host hardware and the available caches.

**Expected result:** BitBake successfully builds the `usb-image` image.

Output images are located in `Yocto/build/tmp/deploy/images/${MACHINE}/`. With the current machine configuration, `raspberrypi3-64`, check:

```bash
ls "$BUILDDIR/tmp/deploy/images/raspberrypi3-64/"
```

Whenever you open a new terminal, initialize the environment again before running BitBake:

```bash
cd ~/projects/FastTrack01/Yocto
source tools/init-build.sh
bitbake usb-image
```
