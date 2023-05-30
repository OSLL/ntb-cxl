#/bin/bash

set -x

bitbake -c kernel_configme -f virtual/kernel && \
bitbake -c compile -f virtual/kernel && \
bitbake -c deploy virtual/kernel && \
bitbake core-image-full-cmdline
