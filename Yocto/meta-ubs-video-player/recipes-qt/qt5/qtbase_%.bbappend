# The player runs directly on DRM/KMS without an X server.
# Enable EGLFS even when the distro's default x11 feature selects desktop GL.
PACKAGECONFIG_GL:raspberrypi3-64 = "gles2 eglfs kms gbm"
