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
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"
#include "qemu/event_notifier.h"

typedef struct PCIMsiExampleState {
    PCIDevice parent_obj;
} PCIMsiExampleState;

#define MSI_EXAMPLE_PCI_DEVICE_TYPE "msi-example"

#define MSI_EXAMPLE_DEV(obj) OBJECT_CHECK(PCIMsiExampleState, (obj), MSI_EXAMPLE_PCI_DEVICE_TYPE)

static void msi_example_realize(PCIDevice *pci_dev, Error **errp)
{
    PCIMsiExampleState *d = MSI_EXAMPLE_DEV(pci_dev);
    uint8_t *pci_conf;

    pci_conf = pci_dev->config;

    pci_config_set_interrupt_pin(pci_conf, 1);

    if (msi_init(pci_dev, 0, 1, true, false, errp)) {
        return;
    }

    printf("msi-example: loaded\n");
}

static void msi_example_unrealize(PCIDevice *pdev)
{
    msi_uninit(pdev);

    printf("msi-example: unloaded\n");
}

static void msi_example_class_init(ObjectClass *class, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(class);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = msi_example_realize;
    k->exit = msi_example_unrealize;
    k->vendor_id = 0x1337;
    k->device_id = 0x0001;
    k->revision = 0x00;
    k->class_id = PCI_CLASS_OTHERS;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static void msi_example_register_types(void)
{
    const TypeInfo msi_example_info = {
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

    type_register_static(&msi_example_info);
}

type_init(msi_example_register_types)
