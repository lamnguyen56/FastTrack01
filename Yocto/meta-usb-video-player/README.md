# USB video player Yocto layer

This Scarthgap layer contains the complete app, shared headers and platform code.

```text
meta-usb-video-player/
├── conf/layer.conf
├── recipes-app/usb-video-player/
│   ├── usb-video-player_1.0.bb
│   └── files/                         # CMakeLists.txt, src/, qml/, service
├── recipes-common/common/
│   ├── common_1.0.bb
│   └── files/                         # Constants.h, Logger.h, CMake export
├── recipes-platform/                 # usb-monitor, can-daemon, host tests
├── recipes-connectivity/wifi/
├── recipes-bsp/
├── recipes-kernel/
├── recipes-qt/
└── recipes-core/images/usb-image.bb
```

## Build

From the repository's Yocto directory, initialize once with tools/init-build.sh.
For an existing build after moving the layer, apply the updated bblayers template:

```sh
source poky/oe-init-build-env build
bash ../tools/setup-bblayers.sh
bitbake usb-image
```

Individual targets are common, usb-monitor, can-daemon and usb-video-player.
The app installs /usr/bin/usb-video-player and usb-video-player.service.
All three processes declare a build dependency on common and link its
header-only CMake target Common::Headers. There is no app-local header copy.
See recipes-common/common/README.md for details.

## Existing build and target migration

The old layer meta-ubs-video-player, app ubs-video-player and image ubs-image have
been renamed. Use the new names in custom bblayers, build commands and scripts.
The repository's tools templates and active build/conf/bblayers.conf were updated.
Other custom build directories must reapply setup-bblayers.sh before running
BitBake. Download and sstate cache directories are unchanged; no cache deletion
or full clean is required.

Existing Pi installations still have the old binary/service until deployed.
Flash the newly built usb-image once to obtain the renamed app and service plus
current dependencies. A binary-only deploy cannot perform this first migration:
it expects the selected binary and service to be installed already. If migrating
manually, stop and disable the old ubs-video-player.service, install the new binary
and unit, remove any stale old unit overrides, reload systemd and enable the new
unit. Avoid running old and new apps together because they share the IPC socket.

After migration, normal development uses tools/deploy-process.sh with usb, can or
app selectors. QML is embedded in the app executable. The reorganization does not
change the CAN packet format, IPC socket names or USB mount point.

## Host tests

```sh
cmake -S meta-usb-video-player/recipes-platform/tests -B /tmp/usb-platform-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/usb-platform-tests -j2
ctest --test-dir /tmp/usb-platform-tests --output-on-failure --no-tests=error
```

Run these commands from Yocto. Standalone Qt development can open
recipes-app/usb-video-player/files/CMakeLists.txt with Qt Creator; it uses the
same shared headers under recipes-common. Install the required Qt development
modules on the host for that workflow.

## Display readiness at boot

The app service runs /usr/bin/wait-for-display.sh before starting EGLFS. It waits
up to 60 seconds for a connected DRM connector with advertised display modes and
a primary /dev/dri/cardN character device. If the display is still unavailable,
the service retries after RestartSec. It does not force a resolution or repair
EDID/cable problems, and unplugging a display after startup needs separate testing.
CAN startup is not ordered before the UI, so missing CAN hardware does not impose
its setup timeout on each UI restart. The service still wants the CAN daemon.

Validate the gate without display hardware from Yocto:

```sh
python3 meta-usb-video-player/recipes-app/usb-video-player/tests/display_test.py
```

Deploy both wait-for-display.sh and usb-video-player.service when updating an
existing Pi manually, then run systemctl daemon-reload and restart the app. The
binary-only deploy-process.sh script does not copy these files. A rebuilt
usb-image includes both automatically. No qtbase rebuild is needed for this change.
