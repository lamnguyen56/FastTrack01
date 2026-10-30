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
| `MediaSessionLifecycle` | C++ session teardown/recreation with obsolete-callback rejection |
| `PlaybackCoordinator` | C++ pending Play, readiness checks, cancellation and deferred EOS |
| `VideoPlaybackBridge` | C++ signal/slot routing, backend report validation and error policy |
| `PlayerOverlayState` / `UsbNoticeState` | C++ overlay feedback and USB notification lifetime |
| `PlayerApplication` | Register C++ QML types and wire USB, CAN, notification state and the view |
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
CAN commands and UI controls use the same controller. CAN IPC readiness remains available through process logs.

## C++ and QML boundary

The `UsbPlayer 1.0` module exposes C++ QObject types through `Q_PROPERTY` and
notification signals. Controller signals drive the C++ media bridge; QML forwards
media callbacks and user actions to invokable C++ methods. Session recreation,
Play/EOS scheduling, errors and notification timers run in C++ on the GUI event
thread. QML retains MediaPlayer/VideoOutput ownership, layout bindings, animations
and icon drawing. No worker thread or direct GStreamer pipeline is introduced.

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

1. Start with USB absent: empty-media message and no playlist rows.
2. Insert FAT32 USB containing videos: USB mounts; opening the playlist shows filenames/count.
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
Opening the playlist should show the real USB filenames/count. Updating only
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

`PlaybackCoordinator` (C++) serializes Play and end-of-media handling for both Qt
adapters. Completion is deferred until the old backend callback has returned.
Play remains pending during LoadingMedia and is attempted outside status callbacks
only when the requested source matches both the controller and backend and the
backend reports a playable status. The request is consumed when PlayingState is
reported. Pause, Stop, errors and source replacement invalidate queued requests.
No fixed loading delay or busy retry timer is used.

The AUTO NEXT card is anchored to the bottom-right of the control bar independently
of the centered transport buttons. It shows PLAY NEXT VIDEO or STOP AT END and a
font-independent toggle indicator.

The headless async transport test instantiates the registered C++ coordinator with
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

`PlayerOverlayState` (C++) separates empty/ready/loading/error feedback from video
metadata. A pending Play or initial media load shows LOADING VIDEO with a busy
indicator. After the backend reports PlayingState for that source, instructional
text stays hidden through pause, buffering and held end-of-media. Changing source
resets this state. Decoder errors and USB loss take precedence and remain visible.
The view no longer decides whether to show PRESS PLAY solely from playlist count
or hasVideo metadata, which can be unavailable during media negotiation.

The overlay-state test covers absent USB, empty/ready playlist, loading, started
playback, pause/held-frame semantics, next selection, errors and unplug priority.
It validates the C++ presentation state model; real video/frame timing requires a Pi test.

## Qt5 hardware-decoder session handoff

The production Qt5 adapter now recreates its MediaPlayer and VideoOutput per
selection, including same-file AUTO NEXT. MediaSessionLifecycle first requests
Stop and clears the old source, deactivates the session Loader, then creates only
the latest requested session on a later event-loop turn. Rapid Next or unplug
invalidates queued replacements. The persistent PlaybackCoordinator keeps Play
intent while there is no backend and starts only the matching ready session.

The new player starts with an empty source. Its source is assigned only after the
session component completes, so VideoOutput is attached before media loading and
preroll. Callbacks during teardown cannot overwrite the new controller state.
AUTO NEXT disabled does not replace the session at EOS, preserving LastFrame;
Pause/Resume and manual seeking stay within the active session. The optional Qt6
desktop adapter retains its existing lifecycle; this change targets Qt5 on Pi.

This addresses reuse/initialization risks identified in code for intermittent
black video after a hardware-decoded track change. It is not proof of the device's
root cause. No seek-forward workaround, software-decoder downgrade or fixed sleep
was added. [MediaSession] logs record session creation, PlayingState and output
format geometry; geometry and PlayingState do not themselves prove a displayed
frame. Test repeated auto/manual transitions between different resolutions,
one-file repeat, held EOS, pause/resume, seek and unplug on the Pi. If black video
persists, collect these logs with GStreamer v4l2/decodebin diagnostics.

The session-handoff test executes the actual lifecycle QML for teardown boundaries,
rapid replacements, same-file replay and unplug; transport tests cover Play intent
across a missing backend. These headless tests cannot exercise V4L2 or rendering.

## Hardware-only video policy

HardwareDecodePolicy applies before the Qt5 engine loads media. It lowers all
video/image decoder factories except v4l2h264dec to rank NONE and sets that factory
to rank 300. It applies to both existing registry features and late plugin feature
registration. Audio decoder, parser and demuxer ranks are unchanged. This is local
to the app process; gst-inspect run from a shell can still list software decoders.
It prevents automatic software video fallback rather than deleting plugin files.

When the hardware decoder rejects a stream, the Qt5 adapter stops the complete
media session, cancels pending Play/EOS and marks the selection blocked. The UI
shows Unsupported resolution or video format, optional reported dimensions and a
suggestion to use H.264 720p. Resource/I/O errors retain their backend message;
they are not presented as resolution errors. The user can select another file;
unsupported files are not silently skipped and Play remains disabled until a new
selection resets the failure. No new ffprobe/discoverer executable is required.

This policy is compiled into the binary; deploy-process.sh app updates it even if
the installed service still has the previous rank preference. A later image also
installs the updated service setting. Confirm the startup Video decode policy log,
720p using v4l2h264dec, 1440p rejection without avdec_h264 entering PLAYING, AV1
rejection, no audio-only playback after video rejection, recovery on selecting a
supported file and preservation of the AUTO NEXT/USB removal behavior on the Pi.
Headless tests check failure messages and UI state, not hardware/GStreamer runtime.

## Compact playback UI

The normal view has no header or continuous CAN-status animation. Video fills
the viewport; the transport bar overlays it. The playlist starts closed: use the bottom-left
Playlist button or drag inward from the left edge to open it. Close with the
same button, Escape, or an outward swipe. Clicking the exposed video area also closes the drawer and consumes that click. The footer toggle handles its click independently to avoid closing and immediately reopening the drawer. Selecting a track
requests playback and closes the drawer; USB removal closes it as well.

The compact, translucent drawer opens on the left, above the Playlist button, without resizing the media viewport. File names stay opaque; long names are elided and longer lists scroll. The list remains loaded while closed, and filename width is measured only when
the model or font changes. Labels use native text rendering. Rows show numbers,
filenames and a selection icon without per-row timeline or hover animation. Open/close transitions are
disabled to reduce rendering work; interactive dragging remains available.
Verify 1080p playback with the drawer closed/open, selection, edge dragging and
portrait/landscape layouts on the Pi.

Replay after a held end-of-media frame replaces the drained media session when
Play is pressed, then starts the same source from zero. The final frame remains
until that request. Pause/resume and ordinary seeking keep their existing paths.
The playlist Drawer uses zero-duration position transitions: removing transition
objects entirely on Qt5 can leave an opened drawer off-screen.


### Auto-hidden playback controls

During playback, the complete bottom bar (including Playlist and Auto Next) is
hidden unless the pointer is over the bottom region. Pausing, reaching the end or dragging the seek slider keeps controls visible.
Opening or dragging the playlist does not force the controls to appear. Move
away from the bottom region to hide them again while playing. Clicking the video
toggles Play/Pause; when the playlist is open, the first video click only closes
it. Touch users can tap the video to pause and reveal controls.

Controls have a transparent bar background and overlay the video without resizing its viewport. Hidden timeline labels
and progress detach from position updates; visibility changes have no animation.


Playlist width follows the widest filename measured in the display font, including
space for row numbers, selection icons and padding. The header sets a minimum;
the responsive drawer limit caps very long names, which remain elided.


### Qt5 OpenGL video path

The layer patches Qt Multimedia to specify the actual GL API, share Qt's
native display with GStreamer and negotiate Qt-compatible 2D textures. An
opt-in renderer change snapshots the retained last frame only on flush instead
of reading it from GPU to CPU on every frame. For this path, the player waits for VideoOutput's
`GLContext` property before loading the source; stale loads are cancelled when
selection changes. The normal non-GL loading path remains immediate.

The default service enables this path. It requires the matching patched `libQt5MultimediaGstTools` and
`libQt5MultimediaQuick` libraries **and** the updated player, plus these service environment settings:

```ini
Environment="QT_GSTREAMER_USE_OPENGL_PLUGIN=1"
Environment="QT_MULTIMEDIA_DEFER_FRAME_SNAPSHOT=1"
Environment="GST_GL_PLATFORM=egl"
Environment="GST_GL_API=gles2"
```

A full image build includes the patched libraries, the updated player and the
required GStreamer OpenGL plugin. Binary-only player deployment is not sufficient
when updating Qt libraries. Verify `video/x-raw(memory:GLMemory)` in
the negotiated pipeline and test replay, seeking and track changes on the board.
