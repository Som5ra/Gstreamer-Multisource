# GStreamer Install
## Ubuntu
```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
```

### Hardware Accleration
- Verify hardware accleration after installation
- Output will be empty if there is no hardware accleration
```bash
gst-inspect-1.0 | grep 'nvenc\|nvdec\|nvcodec'
```

- For Ubuntu <= 20.04, the Gstreamer installed from the apt channel does not include hardware accleration. Building from source is needed.


## Include Gstreamer in CmakeLists.txt
```cmake
message(STATUS "Using gstreamer installed in system")
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GSTREAMER)
```