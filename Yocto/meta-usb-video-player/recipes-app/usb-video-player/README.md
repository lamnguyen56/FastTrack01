# USB playlist and Qt playback integration

```text
USB storage -> usb-monitor -> /tmp/usb_playlist.txt
                                   |
                            UsbPlaylistReader
                                   |
                            UsbPlaylistModel -> QML playlist
                                   |
CAN IPC -> CanIpcClient -> PlayerController <- UI buttons
                                   |
                         Qt Multimedia / VideoOutput
```

## Modules

| Module | Responsibility |
|---|---|
| `UsbPlaylistReader` | Observe file/parent-directory changes, recover through polling, query mount state and validate snapshots |
| `UsbPlaylistModel` | Expose ordered filename/path roles and count through QAbstractListModel |
| `PlayerController` | Selection, bounded seek requests, transport commands, actual timeline/state and errors |
| `PlayerApplication` | Wire USB, CAN and the QML view; keep main.cpp small |
| `VideoSurfaceQt5.qml` | Qt5 MediaPlayer and VideoOutput used on the Pi |
| `VideoSurfaceQt6.qml` | Separate Qt6 desktop adapter with audio/video output APIs |

The IPC file has one absolute UTF-8 path per LF-terminated line. The common header
owns its path and the mount path. Newline characters in filenames are not supported
by this format. Paths must resolve to readable regular media files inside the USB
mount; duplicates, symlinks, outside paths and unsupported extensions are ignored.
The supported extension list follows usb-monitor, but extension matching does not
guarantee a decoder exists for a given file.

The reader loads an already inserted drive at startup, watches the file and its
parent directory, rearms watches after atomic replacement, and polls every second
as a fallback. USB removal clears the model, stops playback and clears its source.
A mounted USB with no files remains distinguishable from an absent USB. USB mount
and scanning remain owned by usb-monitor; the app does not mount or recursively
rescan the drive. The publisher updates on USB insertion/removal, not arbitrary
file changes made while the drive remains mounted.

The first row is selected without autoplay when a new playlist arrives. Selecting
a row or pressing Play starts playback. Next/Previous wrap at the actual count;
EndOfMedia advances to the next track and wraps. Seek uses real milliseconds and
is disabled until duration and seekability are reported by the media engine. A
surviving selected path keeps its playback position when the list is reordered.
CAN commands and UI controls use the same controller. The CAN badge describes
local IPC readiness, not physical CAN-bus connectivity.

## Build and deploy

Run from an initialized Yocto shell. For example, from Yocto/tools:

```sh
./deploy-process.sh usb 192.168.1.30
./deploy-process.sh --logs app 192.168.1.30
```

Deploy both processes for the atomic publisher/reader pair. Qt5/QML/Multimedia,
fonts and an appropriate decoder must already be installed on the Pi. The existing
app recipe supplies Qt runtime packages. Binary deployment does not install new
runtime dependencies or service changes. Alternatively build and flash usb-image.
No Qt source changes are needed for this integration.

## Test on the Pi

Connect and power the HDMI display before starting the app. On the serial console
or a separate SSH session:

```sh
journalctl -u usb-monitor.service -f -o cat
journalctl -u usb-video-player.service -f -o cat
```

Use separate terminals for the two follow commands. Test these cases:

1. Start with USB absent: zero rows, disconnected badge, empty-media message.
2. Insert FAT32 USB containing videos: filenames/count appear and USB is mounted.
3. Click a filename and Play: video renders, real duration/position update.
4. Pause/resume, next/previous and seek; test a one-file drive too.
5. Unplug during playback: video stops and the playlist/source clear.
6. Reinsert or restart the app with USB already present: playlist recovers.
7. Try an unsupported codec: the UI shows the media backend error.

Check the publisher data with `cat /tmp/usb_playlist.txt`. Logs include mount
changes, accepted media count, selected path and playback errors. Successful model
and cross-build tests do not establish hardware decoding or audio/video playback
on the Pi; those require the target checks above.

## Headless integration test

With Qt5/Qt6 Core development files installed on the host:

```sh
cmake -S meta-usb-video-player/recipes-app/usb-video-player/tests -B /tmp/usb-ui-core-tests
cmake --build /tmp/usb-ui-core-tests -j2
ctest --test-dir /tmp/usb-ui-core-tests --output-on-failure --no-tests=error
```

Run these from Yocto. The test uses temporary files and an injected mount probe;
it does not require USB hardware or root. It covers startup data, absent mounts,
Unicode/spaces, filtering/deduplication, live atomic replacement, file recreation,
selection preservation, removal, empty models, count-based navigation and seeking.

The Yocto-native QtCore can also run the test without installing host Qt. From the
initialized build shell, use:

```sh
native_qt="$BUILDDIR/tmp/sysroots-components/x86_64/qtbase-native/usr"
cmake -S "$BUILDDIR/../meta-usb-video-player/recipes-app/usb-video-player/tests" \
    -B /tmp/usb-ui-core-tests \
    -DCMAKE_PREFIX_PATH="$native_qt" \
    -DOE_QMAKE_PATH_EXTERNAL_HOST_BINS="$native_qt/bin"
cmake --build /tmp/usb-ui-core-tests -j2
ctest --test-dir /tmp/usb-ui-core-tests --output-on-failure --no-tests=error
```

The separate platform test suite also exercises atomic PlaylistWriter publication.
QML syntax is checked with qmllint. Qt6 desktop rendering is not covered by the
headless core test; the production target remains Qt5.

## QML cache during binary-only deployment

Qt5 uses source URLs and timestamps to validate disk-compiled QML. Yocto RCC embeds
SOURCE_DATE_EPOCH as the resource timestamp, which can stay unchanged across local
source edits within one Git revision. A replaced executable can therefore run new
C++ while still loading an old compiled QML view from disk. This was reproduced
with native Qt5: changing a resource property from revision 1 to 2 at a fixed RCC
timestamp still returned 1 until the disk cache was bypassed.

QmlCache now hashes the app's QML/JS resource content before any QQmlEngine is
constructed. It selects a separate QML_DISK_CACHE_PATH per fingerprint under the
app cache directory, retaining caching for unchanged UI content. Resources also
use the application-specific /ui prefix. If source hashing or cache directory
creation fails, disk caching is disabled for that process. Old cache directories
are left intact; there is no broad deletion of user cache files.

After deploying the fixed app, the journal should include `QML UI fingerprint:`.
The view should show the real USB filenames/count and mounted badge. Updating only
the app binary is sufficient for this cache fix; no service change or Qt rebuild
is required. A diagnostic run can use QML_DISABLE_DISK_CACHE=1 to bypass disk cache,
but the normal fixed application uses content-isolated caching.

## H.264 and AAC playback

The app depends on gstreamer1.0-libav for FFmpeg software decoders, including
avdec_h264 and avdec_aac. The tools/local.conf.template accepts the specific
commercial_gstreamer1.0-libav and commercial_ffmpeg recipe flags; setup-build.sh
applies this configuration to the active build.

From the initialized Yocto build shell:

```sh
bash "$BUILDDIR/../tools/setup-build.sh"
bitbake usb-image
```

Install the resulting image before testing playback. Deploying only the app
executable does not install these new plugins and shared-library dependencies.
On the Pi, verify decoder availability with:

```sh
gst-inspect-1.0 avdec_h264
gst-inspect-1.0 avdec_aac
journalctl -u usb-video-player.service -f -o cat
```

These are CPU decoders; smooth playback on Pi 3 depends on resolution, bitrate,
and CPU load and must be measured on hardware. EGLFS renders the UI but does not
by itself provide a video decoder. A successful image dry-run checks dependency
resolution, not actual video playback.
