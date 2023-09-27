#include <linux/module.h>
#include <linux/pci.h>

#define INTX_EXAMPLE_VENDOR_ID 0x1b36 // IDs was taken from https://github.com/qemu/qemu/blob/494a6a2cf7f775d2c20fd6df9601e30606cc2014/include/hw/pci/pci.h (PCI_VENDOR_ID_REDHAT and PCI_DEVICE_ID_REDHAT_TEST)
#define INTX_EXAMPLE_DEVICE_ID 0x0005

struct pci_device_id devid = {
	PCI_DEVICE(INTX_EXAMPLE_VENDOR_ID, INTX_EXAMPLE_DEVICE_ID)
};

/*
 static irqreturn_t msi_example_handle_irq(int irq, void* dev_id)
{
	pr_info("msi-example: got an interrupt\n");

	return IRQ_HANDLED;
}
*/

static int intx_example_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int err;

	err = pci_enable_device(dev);
	if (err)
	{
		pr_err("int_x-example: pci_enable_device failed with code %d\n", err);
		return err;
	}
/*
	// DMA **MUST** be enabled for MSI interrupts to work 
	err = dma_set_mask(&dev->dev, DMA_BIT_MASK(32));
	if (err)
	{
		pr_err("msi-example: dma_set_mask failed with code %d\n", err);
		return err;
	}

	// Also required 
	pci_set_master(dev);

	err = pci_alloc_irq_vectors(dev, 1, 1, PCI_IRQ_MSI);
	if (err < 0)
	{
		pr_err("msi-example: pci_alloc_irq_vectors failed with code %d\n", err);
		return err;
	}

	err = pci_request_irq(dev, 0, &msi_example_handle_irq, NULL, &devid, "msi-example-handler");
	if (err)
	{
		pci_free_irq_vectors(dev);
		pr_err("msi-example: pci_request_irq failed with code %d\n", err);
		return err;
	}
*/
	pr_info("int_x-example: probed a device\n");
	return 0;
}

static void intx_example_remove(struct pci_dev *dev)
{
//	pci_free_irq(dev, 0, NULL);

//	pci_free_irq_vectors(dev);

	pr_info("msi-example: removed a device\n");
}


struct pci_driver drv = {
	.name = "int_x-example",
	.id_table = &devid,
	.probe = &intx_example_probe,
	.remove = &intx_example_remove,
};

static int __init intx_example_init(void)
{
	int err = pci_register_driver(&drv);
	if (err)
	{
		pr_err("int_x-example: pci_register_driver failed with code %d\n", err);
		return err;
	}

	pr_info("int_x-example: loaded\n");
	return 0;
}

static void __exit intx_example_exit(void)
{
	pci_unregister_driver(&drv);

	pr_info("int_x-example: unloaded\n");
}

module_init(intx_example_init);
module_exit(intx_example_exit);
MODULE_LICENSE("GPL");
