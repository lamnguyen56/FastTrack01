# Shared protocol and logging headers

common owns the canonical Constants.h and Logger.h used by usb-monitor,
can-daemon and usb-video-player. Do not copy these headers into other recipes.

Yocto consumers declare `DEPENDS += "common"`. The cmake class stages headers
under `${includedir}/common` and exports `${libdir}/cmake/Common` into each
consumer's recipe sysroot. No extra runtime library is required.

```cmake
find_package(Common CONFIG REQUIRED)
target_link_libraries(my-process PRIVATE Common::Headers)
```

The interface target supplies the include directory and C++17 requirement. Source
includes remain `#include "Constants.h"` and `#include "Logger.h"`.

Standalone consumer builds use add_subdirectory on this same source directory if
it is present. The platform host test project adds it once and shares the target
between both daemons and all tests.

Changing a common header rebuilds dependent recipes through Yocto's sysroot/task
signatures. IPC layout changes require updating all peers together; no protocol
or socket path has been changed by this directory reorganization.
