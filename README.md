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