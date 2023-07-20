# NTB/CXL Bridge for InterVM communication

## Requirements
-- [Docker](https://docs.docker.com/engine/install/)

## Build VM
To build VM use command:
```
./run_vm_build.sh
```

This command will create folder ```build_vm_image``` and build vm image, Linux kernel image and all necessary dependences in this folder.

## Run VM

To run VM use command:
```
./run_qemu.sh
```

User credentials to login vm:
- user: ```root```
- pswd: is not set

## Run pingpong demo

The `yocto_dir` directory will be used as the volume for docker containers.

Steps:
1. Build all utils: `./run_vm_build.sh yocto_dir`
2. Start the server in the separate terminal: `./run_qemu.sh server yocto_dir`
3. Start the first guest in the separate terminal: `./run_qemu.sh first yocto_dir` (login `root`, password is not set)
4. Start the second guest in the separate terminal: `./run_qemu.sh second yocto_dir` (login `root`, password is not set)
5. Run `cat /sys/kernel/debug/ntb_pingpong/0000\:00\:04.0/count ` command in one of the guest. You'll see the number of successful transmissions
6. After a while, run the previous command again and you will see that the number has increased
