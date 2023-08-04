/*
 * MSI interrupts PCI device example
 *
 * Copyright (c) 2014 Levente Kurusa <levex@linux.com>
 * Copyright (c) 2023 Maxim Karasev <mxkrsv@disroot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"

#include "hw/hw.h"
#include "hw/pci/pci.h"
#include "qemu/event_notifier.h"

typedef struct PCIMsiExampleState {
    PCIDevice parent_obj;

    int pos;
    char *buf;
    int buflen;

    MemoryRegion mmio;
    MemoryRegion portio;
} PCIMsiExampleState;

#define MSI_EXAMPLE_PCI_DEVICE_TYPE "msi-example"

#define MSI_EXAMPLE_DEV(obj) OBJECT_CHECK(PCIMsiExampleState, (obj), MSI_EXAMPLE_PCI_DEVICE_TYPE)

static uint64_t msi_example_read(void *opaque, hwaddr addr, unsigned size)
{
    PCIMsiExampleState *d = opaque;

    if (addr == 0)
        return d->buf[d->pos++];
    else
        return d->buflen;
}

static void msi_example_mmio_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{

    PCIMsiExampleState *d = opaque;

    switch (addr) {
    case 0:
        /* write byte */
        if (!d->buf)
            break;
        if (d->pos >= d->buflen)
            break;
        d->buf[d->pos++] = (uint8_t)val;
        break;
    case 1:
        /* reset pos */
        d->pos = 0;
        break;
    case 2:
        /* set buffer length */
        d->buflen = val + 1;
        g_free(d->buf);
        d->buf = g_malloc(d->buflen);
        break;
    }

    return;
}

static const MemoryRegionOps msi_example_mmio_ops = {
    .read = msi_example_read,
    .write = msi_example_mmio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl =
        {
            .min_access_size = 1,
            .max_access_size = 1,
        },
};

static int msi_example_realize(PCIDevice *pci_dev, Error **errp)
{
    PCIMsiExampleState *d = MSI_EXAMPLE_DEV(pci_dev);
    uint8_t *pci_conf;

    pci_conf = pci_dev->config;

    pci_conf[PCI_INTERRUPT_PIN] = 0; /* no interrupt pin */

    memory_region_init_io(&d->mmio, OBJECT(d), &msi_example_mmio_ops, d, "msi-example-mmio", 128);
    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mmio);

    d->pos = 0;
    d->buf = g_malloc(14);
    memcpy(d->buf, "Hello, world!\n", 14);
    d->buflen = 14;
    printf("Loaded MSI pci device example\n");

    return 0;
}

static void msi_example_unrealize(PCIDevice *dev)
{
    // PCIMsiExampleState *d = MSI_EXAMPLE_DEV(dev);
    printf("Unloaded MSI pci device example\n");
}

static void qdev_msi_example_reset(DeviceState *dev)
{
    // PCIMsiExampleState *d = MSI_EXAMPLE_DEV(dev);
}

static void msi_example_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->realize = msi_example_realize;
    k->exit = msi_example_unrealize;
    k->vendor_id = 0x1337;
    k->device_id = 0x0001;
    k->revision = 0x00;
    k->class_id = PCI_CLASS_OTHERS;
    dc->desc = "Example PCI device that uses MSI interrupts";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    dc->reset = qdev_msi_example_reset;
}

static const TypeInfo msi_example_info = {
    .name = MSI_EXAMPLE_PCI_DEVICE_TYPE,
    .parent = TYPE_PCI_DEVICE,
    .instance_size = sizeof(PCIMsiExampleState),
    .class_init = msi_example_class_init,
    .interfaces =
        (InterfaceInfo[]){
            {INTERFACE_CONVENTIONAL_PCI_DEVICE},
            {},
        },
};

static void msi_example_register_types(void)
{
    type_register_static(&msi_example_info);
}

type_init(msi_example_register_types)
