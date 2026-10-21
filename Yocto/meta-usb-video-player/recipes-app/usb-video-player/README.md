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

With Qt5/Qt6 Core and Qml development files installed on the host:

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
    -DOE_QMAKE_PATH_EXTERNAL_HOST_BINS="$native_qt/bin" \
    -DQt5Network_DIR="$native_qt/lib/cmake/Qt5Network" \
    -DQt5Qml_DIR="$BUILDDIR/tmp/sysroots-components/x86_64/qtdeclarative-native/usr/lib/cmake/Qt5Qml"
cmake --build /tmp/usb-ui-core-tests -j2
LD_LIBRARY_PATH="$native_qt/lib:$BUILDDIR/tmp/sysroots-components/x86_64/qtdeclarative-native/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
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

## Pi 3 H.264 hardware decoding

The Pi 3 image installs kernel-module-bcm2835-codec and its kernel-module
dependencies. The kernel autoloads bcm2835-codec through modules-load.d; the player
starts after systemd-modules-load.service. The kernel fragment keeps the stateful
VideoCore V4L2/MMAL driver enabled. No custom device-tree overlay or legacy OpenMAX
userspace is required for this path.

The GStreamer Good recipe explicitly enables v4l2 device probing, and the app
requires its video4linux2 plugin package. The service sets
GST_PLUGIN_FEATURE_RANK=v4l2h264dec:300 so decodebin prefers the hardware decoder.
Libav remains installed for AAC and software fallback. Fallback during autoplug
selection is not a guarantee of recovery after a decoder fails during playback.

Rebuild with bitbake usb-image and flash the resulting image. Binary-only app
deployment does not update kernel modules, plugin packages or the service unit.
On the Pi, check:

```sh
systemctl status systemd-modules-load.service --no-pager
lsmod | grep -E 'bcm2835_codec|bcm2835_mmal|vc_sm_cma'
ls /dev/video*
for node in /sys/class/video4linux/video*/name; do
    printf '%s: ' "$node"
    cat "$node"
done
gst-inspect-1.0 v4l2h264dec
systemctl show usb-video-player.service -p Environment
```

The codec device should report bcm2835-codec-decode; its numeric /dev/videoN
assignment must not be hard-coded. GStreamer registers v4l2h264dec dynamically
when a compatible accessible device is present. If inspection cannot find it,
check journalctl -b -k for codec/VCHIQ probe errors and check the modules-load log
before changing decoder ranks. A stale GStreamer registry can be regenerated by
running gst-inspect-1.0 with GST_REGISTRY=/tmp/gst-hw-check.bin; use the normal
registry again after diagnosis.

With decodebin debug enabled as described during target bring-up, play a known
H.264 8-bit yuv420p 720p file. Confirm v4l2h264dec transitions to PLAYING and exposes
its decoded source pad, rather than merely appearing as an autoplug candidate.
Then test supported 1080p material. Do not assume the previous 2560x1440 file is
supported by Pi 3 hardware. Actual Qt video rendering, seek and repeated file
switches must still be tested on the target; successful build checks cannot
establish hardware playback or zero-copy rendering.

### H.264 parser dependency

The image also installs gstreamer1.0-plugins-bad-videoparsersbad, which provides
h264parse. MP4 H.264 may need parsing and stream-format/alignment conversion before
v4l2h264dec can accept it. Decoder rank alone cannot compensate for a missing
parser. This installs the parser package, not the entire Bad plugins collection.
After updating the image, verify both elements:

```sh
gst-inspect-1.0 --no-colors h264parse
gst-inspect-1.0 --no-colors v4l2h264dec
```

Restart the player and confirm the active decoder in a fresh decodebin log with a
known supported 720p H.264 file. Parser presence alone does not prove hardware
playback succeeds.

## Font-independent UI icons

MediaIcon.qml draws playback controls, playlist markers and the header car symbol
with Qt Quick Canvas geometry. These controls do not require Unicode media glyphs,
emoji fonts, SVG plugins or an icon font package. Labels and filenames still use
the installed text fonts. Icon changes are included in qml.qrc and embedded in the
player executable, so deploy-process.sh app can update them without a new image.
Check previous/next, seek buttons, Play/Pause state changes and playlist selection
on the target after deployment.

## Automatic next track

AUTO NEXT in the transport bar controls only end-of-media behavior. It defaults
to enabled on each application start, preserving the previous playlist behavior;
the choice is not persisted to disk.

- Enabled: completion selects and plays the next file, wrapping at the end. A
  one-file playlist replays that file.
- Disabled: completion retains the selection/source and does not request playback
  or stop/clear the output. Qt5 VideoOutput.LastFrame preserves the final frame.
- Play after held completion replays from the start. Manual Next/Previous always
  selects and plays regardless of AUTO NEXT. Enabling the switch after completion
  does not start playback until the user explicitly plays or selects a file.

The shared C++ controller owns the policy for both media adapters. Final-frame
retention is configured on the production Qt5 backend; the optional Qt6 desktop
adapter has the selection policy but no equivalent frame-retention configuration
for its older supported API. Verify final-frame display on Pi with the hardware
decoder, plus end-of-list wrapping, manual controls and USB removal at completion.
The core tests exercise policy and signal behavior, not video rendering.

### Asynchronous media handoff

PlaybackCoordinator.qml serializes Play and end-of-media handling for both Qt
adapters. Completion is deferred until the old backend callback has returned.
Play remains pending during LoadingMedia and is attempted outside status callbacks
only when the requested source matches both the controller and backend and the
backend reports a playable status. The request is consumed when PlayingState is
reported. Pause, Stop, errors and source replacement invalidate queued requests.
No fixed loading delay or busy retry timer is used.

The AUTO NEXT card is anchored to the bottom-right of the control bar independently
of the centered transport buttons. It shows PLAY NEXT VIDEO or STOP AT END and a
font-independent toggle indicator.

The headless async transport test instantiates the actual QML coordinator with
mock backend objects. It covers delayed media readiness, deferred EOS, automatic
next playback intent, stale sources, cancellation and repeated requests. It does
not replace Pi checks with the real GStreamer backend: verify automatic next and
last-to-first wrap, a one-file playlist, Pause during loading, rapid manual Next,
and AUTO NEXT disabled retaining the last frame.

Qt5 adapter readiness must use MediaPlayer.Loaded, Buffered and Buffering. The
Qt6 adapter uses LoadedMedia, BufferedMedia and BufferingMedia. These enum names
are backend API differences, not interchangeable spellings. The mock coordinator
test exercises scheduling/cancellation, not Qt Multimedia enum resolution; verify
adapter constants against the pinned Qt API and test playback on the Pi.

### Layout stability

Fixed-size AUTO NEXT content and playlist delegates use anchors/positioners rather
than nested layouts. Playlist width and duration columns are bounded; long names
elide without resizing the sidebar. The wrapped empty/error message has a width
set by its containing panel, and the footer uses separate explicit regions for
progress and centered transport controls. Flexible video/list items have explicit
size hints rather than relying on their current geometry as layout hints.

After deployment, check startup, USB insert/remove, long filenames, duration and
selection changes, decoder errors and AUTO NEXT toggling. Watch the journal for
recursive rearrange warnings. Syntax/build checks do not execute the Qt Quick
layout engine; absence of runtime warnings must be confirmed on the Pi.

## Responsive screen sizing

UiMetrics.qml converts design units through u(value), using the current window
width and height. The 1920x1080 landscape and 1080x1920 portrait values are reference
coordinate systems, not fixed window sizes. The window starts at Screen.width and
Screen.height; removing the old minimum dimensions permits smaller screens.

Panel sizes use fractions of available content space: landscape is approximately
84% video / 16% playlist; portrait stacks them at 75% / 25% of available height.
Gaps are subtracted before splitting. Header status badges occupy a separate row
in portrait, and the footer reserves an extra row above the bottom-right AUTO NEXT
card so it cannot overlap the centered transport controls.

Fonts, icons, margins, radii, border widths, control padding and slider dimensions
use the shared scale. Font-independent icon paths still use their internal 24x24
vector coordinate system and are scaled by iconSize. Zero offsets, fractions,
media timestamps and animation durations are not pixel dimensions.

The responsive-metrics test loads the real UiMetrics resource and checks scale
and orientation at 480x320, 800x480, 1280x720, 1920x1080, 2560x720, 600x1024,
1080x1920 and 480x800, plus reference/double-reference sizes. These checks do not
render the complete UI. Verify real text/font rounding, clipping, portrait layout,
resize behavior and absence of layout warnings on the display. Proportional
scaling cannot guarantee readable text or comfortable touch targets on an
arbitrarily small physical screen.

The reference now matches the original compact control sizes on the Pi's 1080p
HDMI display. Relative scaling remains enabled without a fixed pixel cap: at
1920x1080 the header/footer use approximately 56/112 logical pixels and controls
retain their earlier proportions. At other resolutions all chrome scales from
that reference, while most available width is reserved for video. The playlist
fraction is 300/1920 rather than the previous 30% sidebar.
