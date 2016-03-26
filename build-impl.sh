# Use ino (http://inotool.org) to build.
#
# The --alibs="false" flag turns off the searching of libraries in the arduino lib directory, which is causing problems for me. This flag is not in the upstream version of ino.
BUILD_FULL=$1


ino build --alibs="false" -m leonardo --cxxflags="-ffunction-sections -fdata-sections -fno-exceptions -fno-threadsafe-statics -DBUILD_FULL=$BUILD_FULL" --cflags="-ffunction-sections -fdata-sections"
