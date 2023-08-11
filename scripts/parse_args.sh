#!/usr/bin/env bash

help(){
    echo \
"
Usage: $0 [OPTIONS]...

    -h, --help : show this help
    --command : global target to execute, e.g. 'build'
    --build-dir : the absolute path to the build directory. By default,
                  the value from BUILD_PATH environment variable is used

---

QEMU options:
    --ivshmem-common-opts=ARGS : ARGS are used to define ivshmem device
    --cmdline-common=ARGS : ARGS are used to define (?)
    --common-opts=ARGS : ARGS are used to define qemu machine parameters (such as cpu, mem, etc)
    --vm1-opts=ARGS : ARGS are used to define additional arguments for VM1 only
    --vm2-opts=ARGS : ARGS are used to define additional arguments for VM2 only

All options are appended to the default parameters. To override default arguments,
use extended flags with the suffix '-override', e.g. '--vm1-opts-override'
"
}


for ARGUMENT in "$@"; do
    KEY=$(echo $ARGUMENT | cut -f1 -d=)
    KEY_LENGTH=${#KEY}
    VALUE="${ARGUMENT:KEY_LENGTH+1}"

    case $KEY in
        --ivshmem-common-opts)
            IVSHMEM_COMMON_OPTIONS=$VALUE
            ;;
        --ivshmem-common-opts-override)
            IVSHMEM_COMMON_OPTIONS_OVERRIDE=$VALUE
            ;;
        --cmdline-common)
            CMDLINE_COMMON=$VALUE
            ;;
        --cmdline-common-override)
            CMDLINE_COMMON_OVERRIDE=$VALUE
            ;;
        --common-opts)
            COMMON_OPTIONS=$VALUE
            ;;
        --common-opts-override)
            COMMON_OPTIONS_OVERRIDE=$VALUE
            ;;
        --vm1-opts)
            VM1_OPTIONS=$VALUE
            ;;
        --vm1-opts-override)
            VM1_OPTIONS_OVERRIDE=$VALUE
            ;;
        --vm2-opts)
            VM2_OPTIONS=$VALUE
            ;;
        --vm2-opts-override)
            VM2_OPTIONS_OVERRIDE=$VALUE
            ;;
        --command)
            COMMAND=$VALUE
            ;;
        --build-dir)
            BUILD_PATH=$VALUE
            ;;
        -h | --help)
            help
            exit 0
            ;;
        *)
            echo "Unknow flag $KEY, aborting"
            help
            exit 1
            ;;
    esac
done
