#!/usr/bin/env bash

set -x
set -e

EXAMPLE_ROOT="$(readlink -f "$(dirname "$0")")"
REPO_ROOT="$EXAMPLE_ROOT"/../../..

echo "Copying sources of the example..."

cp -r "$EXAMPLE_ROOT"/qemu_src -T "$REPO_ROOT"/qemu_src
cp -r "$EXAMPLE_ROOT"/msi-example-mod -T \
	"$REPO_ROOT"/yocto_files/meta-ntb-cxl/recipes-kernel/msi-example-mod

sed -E 's/(IMAGE_INSTALL:append = ".*)"/\1 msi-example-mod"/' -i \
	"$REPO_ROOT"/yocto_files/configs/local.conf

echo "Building the example..."

cd "$REPO_ROOT"
bash run_container.sh --command=enter_qemu_devenv && \
	bash run_container.sh --command=finish_qemu_devenv && \
	bash run_container.sh --command=build

echo "Running the example..."

bash run_container.sh --command=run_vm --common-opts="-device msi-example"
