#!/bin/sh

case "$1" in
	build)
		scripts/prepare_yocto.sh "$BUILD_PATH" && scripts/build_yocto.sh "$BUILD_PATH"
		;;
	enter_qemu_devenv)
		scripts/prepare_yocto.sh "$BUILD_PATH" && scripts/qemu_enter_devenv.sh "$BUILD_PATH"
		;;
	finish_qemu_devenv)
		scripts/qemu_finish_devenv.sh "$BUILD_PATH"
		;;
	*)
		echo "unknown command: $1"
		exit 1
		;;
esac
