# QEMU Vhost

## What to read about vhost

* [Brief description of the vhost architecture](http://blog.vmsplice.net/2011/09/qemu-internals-vhost-architecture.html)
* [QEMU wiki page](https://wiki.qemu.org/Features/VirtioVhostUser)
* [RedHat blog about vhost-net](https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net)
* 

## Example with connection between VM and Host via socket

*[Origin example](https://gist.github.com/mcastelino/9a57d00ccf245b98de2129f0efe39857)*

### Requirements

Host and guest kernels must have support for **vhost and vsocket**

### Steps

1. Install vsock driver: `modprobe vhost_vsock`
2. Run qemu vm:
```
runqemu qemux86-64 nographic qemuparams="-device vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid=3 \
-enable-kvm -smp sockets=1,cpus=4,cores=2 -cpu host \
-netdev user,id=mynet0,hostfwd=tcp::30022-:22,hostfwd=tcp::32375-:2375 \
-device virtio-net-pci,netdev=mynet0 -global isa-debugcon.iobase=0x402"
```
3. There is a virtio socket in vm:
```
# lspci | grep socket
00:04.0 Communication controller: Red Hat, Inc. Virtio socket (rev 01)
```
4. Run *socat* in the vm: `socat - SOCKET-LISTEN:40:0:x00x00x00x04x00x00x03x00x00x00x00x00x00x00`
5. Connect from the host: `sudo socat - SOCKET-CONNECT:40:0:x00x00x00x04x00x00x03x00x00x00x00x00x00x00`

## Other links

* [Two qemu VMs connected with vhost](https://github.com/wei-w-wang/vhost-pci)
* [DPDK vhost sample application](https://doc.dpdk.org/guides-2.1/sample_app_ug/vhost.html)
