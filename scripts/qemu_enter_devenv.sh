#!/usr/bin/env bash

echo "Entering development environment..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

devtool modify --no-overrides qemu-system-native

cd workspace/sources/qemu-system-native

# Sort of a weird hack, don't have any ideas about why things are like that
git config --local user.name a
git config --local user.email a
{
	git am --abort
} || true

cp -r "$ROOT_PROJECT_PATH"/qemu_src/* .
if [ "$(git status -s)" ]; then
	git add .
	commit_msg="SERVICE: add files from qemu_src"
	prev_commit="$(git log --oneline --grep="$commit_msg" | head -n1 | cut -d' ' -f1)"
	if [ "$prev_commit" ]; then
		git commit --fixup="$prev_commit"
		GIT_SEQUENCE_EDITOR=true git rebase -i --autosquash "${prev_commit}~"
	else
		git commit -m "$commit_msg"
	fi
fi

cd -

set +x

cat <<EOF

========================================================================
Working directory is at poky/build/workspace/sources/qemu-system-native.
Make your changes, commit them and run qemu_leave_devenv.sh.
========================================================================

EOF
