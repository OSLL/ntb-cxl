#!/usr/bin/env bash

echo "Entering development environment..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

devtool modify --no-overrides qemu-system-native

set +x

cat <<EOF

========================================================================
Working directory is at poky/build/workspace/sources/qemu-system-native.
Make your changes, commit them and run qemu_leave_devenv.sh.
========================================================================

EOF
