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
                         GstPlayerBackend / playbin -> glupload -> glcolorconvert -> qmlglsink
```

## Modules

| Module | Responsibility |
|---|---|
| `UsbPlaylistReader` | Observe file/parent-directory changes, recover through polling, query mount state and validate snapshots |
| `UsbPlaylistModel` | Expose ordered filename/path roles and count through QAbstractListModel |
| `PlayerController` | Selection, bounded seek requests, transport commands, actual timeline/state and errors |
| `PlayerApplication` | Wire USB, CAN and the QML view; keep main.cpp small |
| `GstPlayerBackend` | GUI-only transport intent, latest-selection coalescing and immutable worker snapshots |
| `GstPipelineSession` | Pipeline ownership, reset/flush/state/seek and bus/timeline queries on GstControl |
| `GstCommandWorker` | Single-operation executor with GUI watchdog and serialized shutdown |
| `VideoSurfaceQt5.qml` | GstGLVideoItem composited by Qt Quick on the Pi |
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

Deploy both processes for the atomic publisher/reader pair. Qt5/QML and the GStreamer GL plugins,
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

These are CPU decoders; the production Qt5 policy now disables their video
factories during app playback while retaining audio decoding. EGLFS renders the UI but does not
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
GST_PLUGIN_FEATURE_RANK=v4l2h264dec:300,avdec_h264:0. The Qt5 application also
enforces v4l2h264dec as the only automatic video decoder in its GStreamer registry.
Libav remains installed for AAC, but software video fallback is disabled.

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
  or stop/clear the output. The Qt5 GStreamer pipeline retains its final sink frame.
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

On Pi, GstPlayerBackend initializes the GL sink using a cancellable render job,
then applies the latest Play/Pause intent. Bus EOS messages call the shared
controller; no fixed loading delay is used. The Qt6 desktop adapter continues to
use PlaybackCoordinator.qml. Its native mock tests cover transport scheduling;
they do not validate the new GStreamer backend or hardware rendering.

The AUTO NEXT card remains anchored to the bottom-right of the control bar.
Verify auto-next, one-file repeat, pause during preroll, rapid Next and held EOS
on the Pi with the real backend.
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

## USB removal interaction

Removal is handled as source loss, not a confirmation-dependent operation:

- The reader emits disconnected once on a mounted-to-absent transition. An absent
  startup and repeated absent polls do not produce a notification.
- The existing playlist invalidation immediately stops playback, clears the media
  source and metadata, and releases open media handles. Qt5 clears video output;
  AUTO NEXT does not keep a stale frame after source loss.
- The view shows NO USB CONNECTED, an empty playlist and disabled transport/seek
  controls. A nonmodal USB disconnected notice appears above the control bar.
- The notice disappears after six seconds or Dismiss. Dismiss changes only the
  notice; cleanup has already completed. Reconnection closes a stale notice.
- Reconnection reloads a fresh playlist and waits for explicit user playback.
  AUTO NEXT continues to apply to successful end-of-media, not device removal.

This is the chosen project UX, inspired by nonblocking snackbar feedback; it is
not a claim that every vehicle/device uses identical behavior. See Material 3
https://m3.material.io/components/snackbar/overview . The earlier proposal to hold
the current frame until OK is intentionally replaced by this interaction.

Core tests cover startup, repeated polling, removal, reconnect and a second
removal, plus existing source/playlist clearing. Target checks must cover unplug
while playing/paused/at end, rapid reinsertion, no repeated notices, timeout and
Dismiss, empty startup and transport being disabled without media. Actual popup
rendering and GStreamer handle release require Pi testing.

## Video overlay state

PlayerOverlayState.qml separates empty/ready/loading/error feedback from video
metadata. A pending Play or initial media load shows LOADING VIDEO with a busy
indicator. After the backend reports PlayingState for that source, instructional
text stays hidden through pause, buffering and held end-of-media. Changing source
resets this state. Decoder errors and USB loss take precedence and remain visible.
The view no longer decides whether to show PRESS PLAY solely from playlist count
or hasVideo metadata, which can be unavailable during media negotiation.

The overlay-state test covers absent USB, empty/ready playlist, loading, started
playback, pause/held-frame semantics, next selection, errors and unplug priority.
It validates the QML state model; real video/frame timing requires a Pi test.

## Hardware-only video policy

HardwareDecodePolicy applies before the Qt5 engine loads media. It lowers all
video/image decoder factories except v4l2h264dec to rank NONE and sets that factory
to rank 300. It applies to both existing registry features and late plugin feature
registration. Audio decoder, parser and demuxer ranks are unchanged. This is local
to the app process; gst-inspect run from a shell can still list software decoders.
It prevents automatic software video fallback rather than deleting plugin files.

When the hardware decoder rejects a stream, the backend stops the session and
publishes the GStreamer error. Missing video decoder messages are also fatal,
even when playbin could otherwise continue with audio only. Unsupported media is
not automatically skipped. Resource/I/O errors retain their backend messages.
Select a supported file to recover. No ffprobe executable is required on target.
This policy is compiled into the binary; deploy-process.sh app updates it even if
the installed service still has the previous rank preference. A later image also
installs the updated service setting. Confirm the startup Video decode policy log,
720p using v4l2h264dec, 1440p rejection without avdec_h264 entering PLAYING, AV1
rejection, no audio-only playback after video rejection, recovery on selecting a
supported file and preservation of the AUTO NEXT/USB removal behavior on the Pi.
Headless tests check failure messages and UI state, not hardware/GStreamer runtime.

## Experimental direct GStreamer backend (Qt5 / Pi)

The Pi adapter no longer uses Qt Multimedia. `GstPipelineSession` owns `playbin`
with a custom video sink bin: `glupload ! glcolorconvert ! qmlglsink`.
Audio stays in playbin, sharing its clock with video. HardwareDecodePolicy still
allows only v4l2h264dec for video; unsupported video does not fall back to CPU.
Qt6 desktop development retains its existing Qt Multimedia adapter.

The sink plugin is loaded before QML to register GstGLVideoItem. Each URI
initializes qmlglsink to READY on the Qt render thread before starting playbin,
so GL elements receive Qt's display/context. Revision checks reject obsolete
selection completions. Bus and timeline queries run on the GstControl worker;
EOS goes through PlayerController for auto-next, while errors stop the session.
USB removal releases the stream/file while retaining the pipeline, and hides the old video item. At EOS with
auto-next off, the pipeline is retained to hold the last frame until replay or
selection. Position updates are limited to ten per second.

This path enables GL texture sharing; it does not guarantee DMA-BUF/zero-copy.
That must be verified from negotiated caps and measurements on the Pi.

### Build and first deployment

```sh
bitbake usb-video-player
```

The new runtime packages are `gstreamer1.0-plugins-good-qmlgl` and
`gstreamer1.0-plugins-base-opengl`. For the first test, install these packages
and their dependencies with the new app, or build/flash `usb-image`. Deploying
only the executable with deploy-process.sh is insufficient if qmlglsink is absent.
If the Pi already has glupload/glcolorconvert and compatible Qt/GStreamer
libraries, a development-only alternative is to copy just the new plugin:

```sh
# Initialized Yocto build shell on the laptop:
find "$BUILDDIR/tmp/work" -path '*/gstreamer1.0-plugins-good/*/image/usr/lib/gstreamer-1.0/libgstqmlgl.so'
# Copy the resulting ARM64 file to root@PI_IP:/tmp/libgstqmlgl.so.
```

On Pi, run `/lib/ld-linux-aarch64.so.1 --list /tmp/libgstqmlgl.so` to check dependencies.
Then stop the player, copy the plugin with
`cp /tmp/libgstqmlgl.so /usr/lib/gstreamer-1.0/libgstqmlgl.so` and set mode 0755 with chmod, then
verify `gst-inspect-1.0 qmlglsink` before deploying the new executable.
This assumes matching libraries; otherwise deploy the built packages/dependencies
or use the image. Manual plugin copies are replaced on the next image flash.

After the runtime plugins are installed, normal binary-only app updates work.
Do not force GST_GL_WINDOW or GST_GL_DISPLAY: Qt provides the GL context.

On the Pi:

```sh
gst-inspect-1.0 qmlglsink
gst-inspect-1.0 glupload
gst-inspect-1.0 glcolorconvert
systemctl restart usb-video-player.service
journalctl -u usb-video-player.service -n 50 --no-pager -o cat
```

Expect `Video backend: direct GStreamer playbin + qmlglsink` at startup.
Verify the same 720p and 1080p files, pause/resume, seek, rapid track changes,
auto-next on/off, replay after EOS, unsupported media, unplug and reconnect USB.
Compare CPU and visible dropped frames with verbose GST_DEBUG disabled. Native
controller/QML tests do not exercise EGL, the real sink, audio or hardware decode;
those require the Pi and a connected display.
### Replay and transition hardening

Normal selection, same-file repeat and replay after held EOS now reset playbin to
READY and set its URI again. The playbin/video sink bin are retained instead of
being destroyed and reparented per track. This releases the old file and decoder
stream, flushes stale bus messages and avoids flush-seeking an EOS-drained V4L2
decoder immediately followed by Play. Paused/resumed playback stays in its session.
The last frame is retained at EOS until the user presses Play or changes media.

playbin can internally take its sink chain to NULL while resetting to READY, so
qmlglsink is prepared on the Qt render thread before every new preroll. A reset
waits for render-job completion through queued callbacks, without locking the GUI;
obsolete completions cannot start playback. Final shutdown cancels queued jobs.
Error logs include the originating GStreamer element to distinguish a filesrc
I/O failure from a decoder failure. These changes address lifecycle risks; a
reported native SIGSEGV still requires a backtrace if it recurs on the Pi.

Retest held EOS -> Play several times, one-file auto-next, alternating 720p/1080p,
rapid Next/Previous, pause during loading and USB removal. Native tests cover the
controller replay signal order, not GPU/decoder execution or the native crash.
### First-frame startup and diagnostics

The direct backend now explicitly prerolls playbin in PAUSED and waits for a
completed asynchronous state change (queried without blocking). Only then does
it honor a pending Play request. Pause/Stop/selection during preroll still replace
that intent. A file without a supported video stream is rejected rather than
silently playing audio only.

qmlglsink has a finite 50 ms late-frame budget instead of its 5 ms default;
clock synchronization and QoS remain enabled. This allows small scheduling jitter
without disabling frame dropping or audio/video sync. It is a targeted tuning
change, not proof of the random black-screen root cause.

GstVideoMonitor logs negotiated GL caps after preroll, the first rendered sink
buffer and a rate-limited warning after three seconds without frame progress
while PLAYING. The UI no longer equates PLAYING with video delivery; it keeps
loading feedback until rendered counters advance for the current URI. Counters
are scoped per URI so previous-track frames cannot clear the next loading state.
A rendered sink buffer means it was handed to the Qt video item; it does not
prove correct pixels were composited onto the physical display.

On Pi, test first playback, rapid selection, auto-next, held EOS/replay and
pause during loading. If black video recurs, capture the compact log rather than
enabling verbose decodebin output:

```sh
journalctl -u usb-video-player.service -n 100 --no-pager -o cat
```

Look for `Video preroll complete`, `Video sink negotiated`,
`Video sink rendered first buffer` and `Video sink has no frame progress`.
If sink counters advance but the picture is black, investigate Qt/GL composition;
if no buffers render and dropped rises, investigate decoder/timing upstream.
The native frame-progress test covers retained counters, resets, first delivery
and pause/EOS state retention. It cannot validate hardware rendering.
### Rapid Next/Previous and blocked V4L2 shutdown

A captured hang showed the GUI inside gst_pad_stop_task during a V4L2 state
change, while the decoder task remained in gst_poll_wait/buffer acquisition.
Normal pipeline operations no longer run on the GUI thread. GstCommandWorker
runs one operation at a time; GstPipelineSession owns the persistent pipeline.
The Qt facade only updates transport intent and receives immutable snapshots.
An ASYNC state-change return is awaited on the worker for up to five seconds
before another operation can reuse the decoder. This wait never runs on the GUI.
GL sink READY remains a Qt render-thread job, with no worker operation running
concurrently. The GUI does not wait on a render-job mutex during selection.

Stop/Source/Play from one UI event are deferred together. LatestMediaRequest
retains one pending source, replacing earlier clicks during an in-flight reset.
Revision checks prevent an obsolete reset, poll or render completion from
starting the wrong track. Pending seek is scoped to the selected URI.

Reset sends FLUSH_START before transitioning the used stream to READY, allowing
V4L2 event handling to unlock output/capture polls before joining source tasks.
FLUSH_STOP follows the stopped transition; stale bus messages are discarded.
This ordering must still be validated with the actual driver on the Pi.

A 15-second watchdog covers each worker operation, including reset, seek and
queries. On timeout the process logs the operation and exits with failure so
systemd can restart it. It never terminates a QThread or frees a still-used
pipeline. This is bounded recovery, not a kernel/firmware reset; MMAL/VCHIQ may
still require reboot if the driver itself remains wedged. Final shutdown drains
one cleanup operation after any current operation and has the same bounded wait.

The headless worker test runs the actual executor, checks that GUI events continue
during a deliberately slow operation, rejects concurrent submissions, verifies
latest-source revision behavior and cleanup, and tests timeout exit in a child
process. Hardware flush/stop behavior and EGL rendering still require Pi tests.

Deploy only the new app executable; the existing qmlglsink plugin is unchanged.
Verify `serialized GstControl worker` in startup logs, then test rapid Next/Prev,
720p/1080p switching, Pause during loading, held EOS/replay, auto-next and unplug.
If `GStreamer operation timed out` appears, capture that label and the kernel log;
do not treat automatic service restart as proof that decoder teardown is fixed.
