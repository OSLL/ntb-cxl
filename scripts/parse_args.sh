#!/usr/bin/env bash

help(){
    echo \
"
Usage: parse_args.sh [OPTIONS]...

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
            export IVSHMEM_COMMON_OPTIONS=$VALUE
            ;;
        --ivshmem-common-opts-override)
            export IVSHMEM_COMMON_OPTIONS_OVERRIDE=$VALUE
            ;;
        --cmdline-common)
            export CMDLINE_COMMON=$VALUE
            ;;
        --cmdline-common-override)
            export CMDLINE_COMMON_OVERRIDE=$VALUE
            ;;
        --common-opts)
            export COMMON_OPTIONS=$VALUE
            ;;
        --common-opts-override)
            export COMMON_OPTIONS_OVERRIDE=$VALUE
            ;;
        --vm1-opts)
            export VM1_OPTIONS=$VALUE
            ;;
        --vm1-opts-override)
            export VM1_OPTIONS_OVERRIDE=$VALUE
            ;;
        --vm2-opts)
            export VM2_OPTIONS=$VALUE
            ;;
        --vm2-opts-override)
            export VM2_OPTIONS_OVERRIDE=$VALUE
            ;;
        --command)
            export COMMAND=$VALUE
            ;;
        --build-dir)
            export BUILD_PATH=$VALUE
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
