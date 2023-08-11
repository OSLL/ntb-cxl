#!/bin/bash

set -e
set -x

. "$(dirname $0)"/parse_args.sh $@

case "$COMMAND" in
	build)
		scripts/prepare_yocto.sh "$BUILD_PATH" && scripts/build_yocto.sh "$BUILD_PATH"
		;;
	enter_qemu_devenv)
		scripts/prepare_yocto.sh "$BUILD_PATH" && scripts/qemu_enter_devenv.sh "$BUILD_PATH"
		;;
	finish_qemu_devenv)
		scripts/qemu_finish_devenv.sh "$BUILD_PATH"
		;;
	shell)
		bash
		;;
	run_vm)
		scripts/run_vm.sh "$BUILD_PATH"
		;;
	run_vms)
		scripts/run_vms.sh "$BUILD_PATH"
		;;
	*)
		echo "unknown command: $COMMAND"
		exit 1
		;;
esac
