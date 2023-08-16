#include <linux/module.h>
#include <linux/pci.h>

#define MSI_EXAMPLE_VENDOR_ID 0x1337
#define MSI_EXAMPLE_DEVICE_ID 0x0001

struct pci_device_id devid = {
	PCI_DEVICE(MSI_EXAMPLE_VENDOR_ID, MSI_EXAMPLE_DEVICE_ID)
};

static irqreturn_t msi_example_handle_irq(int irq, void* dev_id)
{
	pr_info("msi-example: got an interrupt\n");

	return IRQ_HANDLED;
}

static int msi_example_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int err;

	err = pci_enable_device(dev);
	if (err)
	{
		pr_err("msi-example: pci_enable_device failed with code %d\n", err);
		return err;
	}

	/* DMA **MUST** be enabled for MSI interrupts to work */
	err = dma_set_mask(&dev->dev, DMA_BIT_MASK(32));
	if (err)
	{
		pr_err("msi-example: dma_set_mask failed with code %d\n", err);
		return err;
	}

	/* Also required */
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
		pr_err("msi-example: pci_request_irq failed with code %d\n", err);
		return err;
	}

	pr_info("msi-example: probed a device\n");
	return 0;
}

static void msi_example_remove(struct pci_dev *dev)
{
	pci_free_irq(dev, 0, NULL);

	pci_free_irq_vectors(dev);

	pr_info("msi-example: removed a device\n");
}

struct pci_driver drv = {
	.name = "msi-example",
	.id_table = &devid,
	.probe = &msi_example_probe,
	.remove = &msi_example_remove,
};

static int __init msi_example_init(void)
{
	int err = pci_register_driver(&drv);
	if (err)
	{
		pr_err("msi-example: pci_register_driver failed with code %d\n", err);
		return err;
	}

	pr_info("msi-example: loaded\n");
	return 0;
}

static void __exit msi_example_exit(void)
{
	pci_unregister_driver(&drv);

	pr_info("msi-example: unloaded\n");
}

module_init(msi_example_init);
module_exit(msi_example_exit);
MODULE_LICENSE("GPL");
