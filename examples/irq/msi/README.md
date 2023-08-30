This example consists of a QEMU device and a kernel module.

To start it, do the following:

1. `./run.sh`

2. To ensure it works, run `modprobe msi-example && dmesg -w` in the VM shell.
   The `got an interrupt` message should appear every second.

3. To restore the repository state, run `git restore . && git clean -fd`.
