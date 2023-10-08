/*
 * IDT NTB 89HPES24NT6AG2 device over shared memory
 *
 * Authors:
 * - Tinyakov Sergey
 * - Gavrilov Andrei
 *
 * Based On: ivshmem.c
 *          Copyright (c) 20?? Cam Macdonell <cam@cs.ualberta.ca>
 *
 *
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "qemu/cutils.h"
#include "hw/pci/pci.h"
#include "hw/qdev-properties.h"
#include "hw/qdev-properties-system.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"
#include "sysemu/kvm.h"
#include "migration/blocker.h"
#include "migration/vmstate.h"
#include "qemu/error-report.h"
#include "qemu/event_notifier.h"
#include "qemu/module.h"
#include "qom/object_interfaces.h"
#include "chardev/char-fe.h"
#include "sysemu/hostmem.h"
#include "qapi/visitor.h"

#include "hw/misc/ivshmem.h"
#include "qom/object.h"

#define PCI_VENDOR_ID_IVSHMEM   0x111D  // IDT Vendor ID
#define PCI_DEVICE_ID_IVSHMEM   0x8091  // PCI_DEVICE_ID_IDT_89HPES24NT6AG2 `from ntb_hw_idt.h`

#define IVSHMEM_MAX_PEERS UINT16_MAX
#define IVSHMEM_IOEVENTFD   0
#define IVSHMEM_MSI     1

#define IVSHMEM_REG_BAR_SIZE 0x1000

#define IDT_BAR_APERTURE_POWER 28
#define IDT_BAR_APERTURE_SIZE (1U << IDT_BAR_APERTURE_POWER) /* 256MiB */

#define IVSHMEM_DEBUG 1
#define IVSHMEM_DPRINTF(fmt, ...)                       \
    do {                                                \
        if (IVSHMEM_DEBUG) {                            \
            fprintf(stderr, "IVSHMEM: " fmt, ## __VA_ARGS__);    \
        }                                               \
    } while (0)

typedef struct IVShmemState IVShmemState;

#define TYPE_IVSHMEM_NTB_IDT "idt-ntb-ivshmem"
DECLARE_INSTANCE_CHECKER(IVShmemState, IVSHMEM_NTB_IDT,
                         TYPE_IVSHMEM_NTB_IDT)

typedef struct Peer {
    int nb_eventfds;
    EventNotifier *eventfds;
} Peer;

typedef struct MSIVector {
    PCIDevice *pdev;
    int virq;
    bool unmasked;
} MSIVector;

typedef struct BARConfig {
    uint32_t setup;
    uint32_t limit;
    uint32_t ltbase;
    uint32_t utbase;
} BARConfig;

#pragma pack(push, 1)
typedef struct idt_global_registers {
    uint32_t segsigsts;
    uint32_t segsigmsk;
} idt_global_registers;

typedef struct idt_shared_registers {
    uint32_t db; /* Inbound doorbell (outbound from the peer's view) (related to patrition 0?) */
    uint32_t db_mask; /* Inbound doorbell interrupt mask */

    uint64_t ntctl;

    uint64_t msgsts; /* Inbound/outbound messages status */
    uint64_t msgsts_mask; /* Message status interrupt mask */
    uint64_t msg[4]; /* Inbound messages (outbound from the peer's view) */

    uint64_t intsts; /* Which interrupt we just received? */

    /* Currently only first value is used */
    uint64_t part0_msg_control[4];  /* Control for inbound and outbound messages */
} idt_shared_registers;
#pragma pack(pop)

typedef struct idt_local_registers {
    uint64_t gasaaddr;
    uint64_t nt_mtb_addr;

    /* Not implemented */
    /* uint8_t srcbound[4]; */  /* Src partition of messages */
} idt_local_registers;

struct IVShmemState {
    /*< private >*/
    PCIDevice parent_obj;
    /*< public >*/

    uint32_t features;

    /* exactly one of these two may be set */
    HostMemoryBackend *hostmem; /* with interrupts */
    CharBackend server_chr; /* without interrupts */

    /* ivshmem-specific BARs */
    MemoryRegion *ivshmem_bar2; /* BAR 2 (shared memory) */
    MemoryRegion server_bar2;   /* used with server_chr */

    /* IDT device BARs */
    MemoryRegion bars[6]; /* BAR 0 maps configuration space, others are general purpose */

    /* interrupt support */
    Peer *peers;
    int nb_peers;               /* space in @peers[] */
    uint32_t vectors;
    MSIVector *msi_vectors;
    uint64_t msg_buf;           /* buffer for receiving server messages */
    int msg_buffered_bytes;     /* #bytes in @msg_buf */

    /* migration stuff */
    OnOffAuto master;
    Error *migration_blocker;

    /* IDT interconnect */
    uint32_t self_number;
    int vm_id;
    int other_vm_id;

    /* Not related to IDT device directly */
    uint32_t *vm_id_shared;
    uint32_t *other_vm_id_shared;

    /* Registers that are writeable from peer QEMU instance */
    idt_shared_registers *regs;

    /* Registers that don't need to be accessed from peer QEMU instance */
    idt_local_registers lregs;

    /* Peer's shared registers */
    idt_shared_registers *peer_regs;

    /* Global switch registers */
    idt_global_registers *gregs;

    /* BAR states */
    BARConfig *bar_config;

    /* Peer physical memory */
    void *peer_mem;
};

enum idt_global_config_registers {
    /* Port 0 */
    IDT_SW_NTP0_PCIECMDSTS = 0x1004U,
    IDT_SW_NTP0_NTCTL      = 0x1400U,

    IDT_SW_NTP0_BARSETUP0  = 0x1470U,
    IDT_SW_NTP0_BARSETUP1  = 0x1480U,
    IDT_SW_NTP0_BARSETUP2  = 0x1490U,
    IDT_SW_NTP0_BARSETUP3  = 0x14A0U,
    IDT_SW_NTP0_BARSETUP4  = 0x14B0U,
    IDT_SW_NTP0_BARSETUP5  = 0x14C0U,

    IDT_SW_SWPORT0STS      = 0x3E204U,

    /* Port 2 */
    IDT_SW_NTP2_PCIECMDSTS = 0x5004U,
    IDT_SW_NTP2_NTCTL      = 0x5400U,

    IDT_SW_NTP2_BARSETUP0  = 0x5470U,
    IDT_SW_NTP2_BARSETUP1  = 0x5480U,
    IDT_SW_NTP2_BARSETUP2  = 0x5490U,
    IDT_SW_NTP2_BARSETUP3  = 0x54A0U,
    IDT_SW_NTP2_BARSETUP4  = 0x54B0U,
    IDT_SW_NTP2_BARSETUP5  = 0x54C0U,

    IDT_SW_SWPORT2STS      = 0x3E244U,

    /* Partition 0 */
    IDT_SW_SWPART0STS      = 0x3E104U,
    IDT_SW_SWP0MSGCTL0     = 0x3EE00U,

    /* Switch event registers */
    IDT_SW_SEGSIGSTS       = 0x3EC30U,
    IDT_SW_SEGSIGMSK       = 0x3EC34U,
};

enum idt_register_values {
    VALUE_SWPORT0STS = (0x1U << 4) | /* Link up, data link layer is 'DL_up' */ \
                       (0x3U << 6) | /* Partition is 0x0 */ \
                       (0x0U << 10),  /* NT Function is enabled */
    VALUE_SWPORT2STS = VALUE_SWPORT0STS,
    VALUE_SWPART0STS = (0x1U << 5), /* Partition is enabled */
    VALUE_NTP0_PCIECMDSTS = (0x1U << 2), /* Bus Master is enabled */
    VALUE_NTP2_PCIECMDSTS = VALUE_NTP0_PCIECMDSTS,
};

enum idt_default_register_values {
    VALUE_NTP0_NTCTL = (0x1 << 1), /* Completion is enabled (CPEN flag) */
    VALUE_NTP2_NTCTL = VALUE_NTP0_NTCTL,
};

enum idt_config_registers {
    IDT_NT_PCICMDSTS   = 0x4U,
    IDT_NT_PCIELCAP    = 0x4CU,
    IDT_NT_PCIELCTLSTS = 0x50U,
    IDT_NT_NTCTL       = 0x400U,
    IDT_NT_NTINTSTS    = 0x404U,
    IDT_NT_NTGSIGNAL   = 0x410U,
    IDT_NT_OUTDBELLSET = 0x420U,
    IDT_NT_INDBELLSTS  = 0x428U,
    IDT_NT_INDBELLMSK  = 0x42CU,
    IDT_NT_OUTMSG0     = 0x430U,
    IDT_NT_OUTMSG1     = 0x434U,
    IDT_NT_OUTMSG2     = 0x438U,
    IDT_NT_OUTMSG3     = 0x43CU,
    IDT_NT_INMSG0      = 0x440U,
    IDT_NT_INMSG1      = 0x444U,
    IDT_NT_INMSG2      = 0x448U,
    IDT_NT_INMSG3      = 0x44CU,
    IDT_NT_MSGSTS      = 0x460U,
    IDT_NT_MSGSTSMSK   = 0x464U,
    IDT_NT_NTMTBLADDR  = 0x4D0U,
    IDT_NT_NTMTBLDATA  = 0x4D8U,
    GASAADDR           = 0xFF8U,
    GASADATA           = 0xFFCU,

    IDT_NT_BARSETUP0  = 0x470U,
    IDT_NT_BARLIMIT0  = 0x474U,
    IDT_NT_BARLTBASE0 = 0x478U,
    IDT_NT_BARUTBASE0 = 0x47CU,
    IDT_NT_BARSETUP1  = 0x480U,
    IDT_NT_BARLIMIT1  = 0x484U,
    IDT_NT_BARLTBASE1 = 0x488U,
    IDT_NT_BARUTBASE1 = 0x48CU,
    IDT_NT_BARSETUP2  = 0x490U,
    IDT_NT_BARLIMIT2  = 0x494U,
    IDT_NT_BARLTBASE2 = 0x498U,
    IDT_NT_BARUTBASE2 = 0x49CU,
    IDT_NT_BARSETUP3  = 0x4A0U,
    IDT_NT_BARLIMIT3  = 0x4A4U,
    IDT_NT_BARLTBASE3 = 0x4A8U,
    IDT_NT_BARUTBASE3 = 0x4ACU,
    IDT_NT_BARSETUP4  = 0x4B0U,
    IDT_NT_BARLIMIT4  = 0x4B4U,
    IDT_NT_BARLTBASE4 = 0x4B8U,
    IDT_NT_BARUTBASE4 = 0x4BCU,
    IDT_NT_BARSETUP5  = 0x4C0U,
    IDT_NT_BARLIMIT5  = 0x4C4U,
    IDT_NT_BARLTBASE5 = 0x4C8U,
    IDT_NT_BARUTBASE5 = 0x4CCU,
};

enum msgsts_fields {
    IDT_FLD_OUTMSGSTS0 = 0,
    IDT_FLD_OUTMSGSTS1,
    IDT_FLD_OUTMSGSTS2,
    IDT_FLD_OUTMSGSTS3,
    IDT_FLD_INMSGSTS0 = 16,
    IDT_FLD_INMSGSTS1,
    IDT_FLD_INMSGSTS2,
    IDT_FLD_INMSGSTS3,
};

enum barsetup_fields {
    IDT_FLD_BARSETUP_TYPE = 1,
    IDT_FLD_BARSETUP_SIZE = 4,
    IDT_FLD_BARSETUP_MODE = 10,
    IDT_FLD_BARSETUP_ENABLE = 31,
};

enum idt_config_registers_values {
    VALUE_NT_PCIELCAP_VM1 = (0x0U << 24), /* Local port number is 0x0 */
    VALUE_NT_PCIELCAP_VM2 = (0x2U << 24),
    VALUE_NT_PCIELCTLSTS = (0x1U << 16) | (0b100U << 20), /* Let it be a 4-lane gen1 */
    VALUE_NT_PCICMDSTS = (0x1U << 2), /* Bus Master is enabled */
};

enum idt_default_config_register_values {
    VALUE_NT_BARSETUP0 = (0x0U << IDT_FLD_BARSETUP_TYPE) | /* 32-bit addressing */ \
                         (0xCU << IDT_FLD_BARSETUP_SIZE) | /* Address Space Size = 4 KiB */ \
                         (0x1U << IDT_FLD_BARSETUP_MODE) | /* This BAR isn't a window (NT Configuration Space) */ \
                         (0x1U << IDT_FLD_BARSETUP_ENABLE), /* This BAR is enabled */

    VALUE_NT_BARSETUP2 = (IDT_BAR_APERTURE_POWER << IDT_FLD_BARSETUP_SIZE) |
                         (0x1U << IDT_FLD_BARSETUP_ENABLE),
    VALUE_NT_BARSETUP3 = VALUE_NT_BARSETUP2,
    VALUE_NT_BARSETUP4 = VALUE_NT_BARSETUP2,
    VALUE_NT_BARSETUP5 = VALUE_NT_BARSETUP2,
};

enum idt_ivshmem_eventfds {
    EVENTFD_VM_ID = 0,
    EVENTFD_INTERRUPT_HOST,
};

#pragma pack(push, 1)
struct idt_ivshmem_vm_shm_storage {
    uint32_t id;
    idt_shared_registers regs;
    BARConfig bar_config[6];
};

struct idt_ivshmem_shm_storage {
    struct idt_global_registers gregs;
    struct idt_ivshmem_vm_shm_storage vm1;
    struct idt_ivshmem_vm_shm_storage vm2;
};
#pragma pack(pop)

enum idt_interrupts {
    IDT_NTINTSTS_MSG = 0x1U,
    IDT_NTINTSTS_DBELL = 0x2U,
    IDT_NTINTSTS_SEVENT = 0x8U,
};

static inline uint32_t ivshmem_has_feature(IVShmemState *ivs, unsigned int feature)
{
    return (ivs->features & (1 << feature));
}

static inline bool ivshmem_is_master(IVShmemState *s)
{
    assert(s->master != ON_OFF_AUTO_AUTO);
    return s->master == ON_OFF_AUTO_ON;
}

static void interrupt_notify(IVShmemState *s, unsigned int vector)
{
    /* Use different types of notification:
     * - Wired (LEGACY) (Not implemented)
     * - MSI
     * - MSI-X (Not implemented)
     * IDT NTB supports only MSI and Wired (LEGACY) interrupts.
     */
    /* MSI */
    msi_notify(&s->parent_obj, vector);
    /* MSI-X */
    /* msix_notify(pdev, vector); */
}

static void write_outbound_msg(IVShmemState *s, int index, uint64_t val)
{
    if ((index < 0) || (index > 3))
    {
        error_report("idt-ntb-ivshmem: invalid msg register index");
        return;
    }

    if (s->peer_regs->msgsts & (0x1U << (IDT_FLD_INMSGSTS0 + index)))
    {
        IVSHMEM_DPRINTF("Refusing to write to msg register, INMSGSTS%d is non-zero (vm%d 0x%lx)\n",
                index, s->vm_id + 1, s->peer_regs->msgsts);
        s->regs->msgsts |= (0x1U << (IDT_FLD_OUTMSGSTS0 + index)); /* Set the error status */
        s->regs->intsts = IDT_NTINTSTS_MSG;
        interrupt_notify(s, 0);
        return;
    }

    s->peer_regs->msg[index] = val;
    IVSHMEM_DPRINTF("Wrote value 0x%lx to the outbound message register %d\n", val, index);

    if (s->other_vm_id != -1)
    {
        s->peer_regs->msgsts |= (0x1U << (IDT_FLD_INMSGSTS0 + index));
        s->peer_regs->intsts |= IDT_NTINTSTS_MSG;
        event_notifier_set(&s->peers[s->other_vm_id].eventfds[EVENTFD_INTERRUPT_HOST]);
        IVSHMEM_DPRINTF("Sent message register update notification (vm%d -> vm%d)\n",
                s->vm_id, s->other_vm_id);
    }
}

static uint64_t get_gasadata(IVShmemState *s)
{
    uint64_t ret;
    struct idt_ivshmem_shm_storage *shm_storage =
        (struct idt_ivshmem_shm_storage *)memory_region_get_ram_ptr(s->ivshmem_bar2);

    switch (s->lregs.gasaaddr) {
        case IDT_SW_SWPORT0STS:
            ret = VALUE_SWPORT0STS;
            IVSHMEM_DPRINTF("GAS: Read switch port 0 status: 0x%lx\n", ret);
            break;
        case IDT_SW_SWPORT2STS:
            ret = VALUE_SWPORT2STS;
            IVSHMEM_DPRINTF("GAS: Read switch port 2 status: 0x%lx\n", ret);
            break;
        case IDT_SW_SWPART0STS:
            ret = VALUE_SWPART0STS;
            IVSHMEM_DPRINTF("GAS: Read switch partition 0 status: 0x%lx\n", ret);
            break;
        case IDT_SW_NTP0_PCIECMDSTS:
            ret = VALUE_NTP0_PCIECMDSTS;
            IVSHMEM_DPRINTF("GAS: Read switch port 0 PCIECMDSTS: 0x%lx\n", ret);
            break;
        case IDT_SW_NTP0_NTCTL:
            ret = shm_storage->vm1.regs.ntctl;
            IVSHMEM_DPRINTF("GAS: Read switch port 0 NTCTL: 0x%lx\n", ret);
            break;
        case IDT_SW_NTP2_PCIECMDSTS:
            IVSHMEM_DPRINTF("GAS: Read switch port 2 PCIECMDSTS: 0x%lx\n", ret);
            ret = VALUE_NTP2_PCIECMDSTS;
            break;
        case IDT_SW_NTP2_NTCTL:
            ret = shm_storage->vm2.regs.ntctl;
            IVSHMEM_DPRINTF("GAS: Read switch port 2 NTCTL: 0x%lx\n", ret);
            break;

#define READ_BARSETUP(vmidx, portidx, baridx) case IDT_SW_NTP ## portidx ## _BARSETUP ## baridx: \
            ret = shm_storage->vm ## vmidx .bar_config[baridx].setup; \
            IVSHMEM_DPRINTF("GAS: Read the NTP%d_BARSETUP%d from GAS: 0x%lx\n", portidx, baridx, ret); \
            break;
#define READ_BARSETUPS(vmidx, portidx) READ_BARSETUP(vmidx, portidx, 0) \
            READ_BARSETUP(vmidx, portidx, 1) \
            READ_BARSETUP(vmidx, portidx, 2) \
            READ_BARSETUP(vmidx, portidx, 3) \
            READ_BARSETUP(vmidx, portidx, 4) \
            READ_BARSETUP(vmidx, portidx, 5)

        READ_BARSETUPS(1, 0)
        READ_BARSETUPS(2, 2)

#undef READ_BARSETUPS
#undef READ_BARSETUP

        default:
            IVSHMEM_DPRINTF("GAS: Not implemented gasadata read on reg 0x%lx\n", s->lregs.gasaaddr);
            ret = 0;
    }
    return ret;
}

static void write_gasadata(IVShmemState *s, uint64_t val)
{
    switch (s->lregs.gasaaddr){
        case IDT_SW_SWP0MSGCTL0:
            s->regs->part0_msg_control[0] = val;
            IVSHMEM_DPRINTF("GAS: Set global SWP0MSGCTL0: 0x%lx\n", val);
            break;
        case IDT_SW_SEGSIGSTS:
            s->gregs->segsigsts &= ~val; /* Substraction with a module of 2 */
            IVSHMEM_DPRINTF("GAS: Cleared the SEGSIGSTS, new value: 0x%x (vm%d)\n",
                    s->gregs->segsigsts, s->vm_id);
            break;
        default:
            IVSHMEM_DPRINTF("GAS: Not implemented gasadata write on reg 0x%lx\n", s->lregs.gasaaddr);
    }
}

static uint64_t get_nt_mtb_data(IVShmemState *s)
{
    uint64_t ret;
    switch (s->lregs.nt_mtb_addr){
        case 0x0U:  // Partion 0
            ret = 0;
            ret |= 0x1;  // Set VALID field
            IVSHMEM_DPRINTF("Reading partition 0 MTB data: 0x%lx\n", ret);
            break;
        default:
            IVSHMEM_DPRINTF("Not implemented nt_mtb_addr on value 0x%lx\n", s->lregs.nt_mtb_addr);
            ret = 0;
    }
    return ret;
}

/*
 *****************************************
 **** Funciton for connecting two VMs ****
 *****************************************
 */

static void init_vm_ids(IVShmemState *s)
{
    *s->vm_id_shared = s->vm_id;
    if (s->self_number) {
        /* Second vm */
        s->other_vm_id = *s->other_vm_id_shared;
        event_notifier_set(&s->peers[s->other_vm_id].eventfds[EVENTFD_VM_ID]);
    }
    IVSHMEM_DPRINTF("Started vm with self_number=%d and vm_id=%d\n", s->self_number, s->vm_id);
}

/*****************************************/

static void ivshmem_io_write(void *opaque, hwaddr addr,
                             uint64_t val, unsigned size)
{
    IVShmemState *s = opaque;

    switch (addr) {
        case GASAADDR:
            s->lregs.gasaaddr = val;
            IVSHMEM_DPRINTF("Set GASAADDR: 0x%lx\n", val);
            break;
        case GASADATA:
            IVSHMEM_DPRINTF("Handling GASADATA write: 0x%lx\n", val);
            write_gasadata(s, val);
            break;
        case IDT_NT_NTCTL:
            s->regs->ntctl = val;
            IVSHMEM_DPRINTF("Set local NTCTL: 0x%lx\n", val);
            break;
        case IDT_NT_NTMTBLADDR:
            s->lregs.nt_mtb_addr = val & (0x7FU);  // Set partion number to interact with mapping table (First six bits)
            IVSHMEM_DPRINTF("Set NTMTBLADDR: 0x%lx\n", s->lregs.nt_mtb_addr);
            break;
        case IDT_NT_OUTMSG0:
            IVSHMEM_DPRINTF("Handling write to OUTMSG0: 0x%lx\n", val);
            write_outbound_msg(s, 0, val);
            break;
        case IDT_NT_OUTMSG1:
            IVSHMEM_DPRINTF("Handling write to OUTMSG1: 0x%lx\n", val);
            write_outbound_msg(s, 1, val);
            break;
        case IDT_NT_OUTMSG2:
            IVSHMEM_DPRINTF("Handling write to OUTMSG2: 0x%lx\n", val);
            write_outbound_msg(s, 2, val);
            break;
        case IDT_NT_OUTMSG3:
            IVSHMEM_DPRINTF("Handling write to OUTMSG3: 0x%lx\n", val);
            write_outbound_msg(s, 3, val);
            break;
        case IDT_NT_OUTDBELLSET:
            s->peer_regs->db = val;
            IVSHMEM_DPRINTF("Wrote value 0x%lx to the outbound doorbell\n", val);

            if (s->other_vm_id != -1)
            {
                s->peer_regs->intsts |= IDT_NTINTSTS_DBELL;
                event_notifier_set(&s->peers[s->other_vm_id].eventfds[EVENTFD_INTERRUPT_HOST]);
                IVSHMEM_DPRINTF("Sent doorbell register update notification (vm%d -> vm%d)\n",
                        s->vm_id, s->other_vm_id);
            }
            break;
        case IDT_NT_INDBELLSTS:
            s->regs->db &= ~val; /* Substraction with a module of 2 */
            IVSHMEM_DPRINTF("Cleared the inbound doorbell, new value: 0x%x\n", s->regs->db);
            break;
        case IDT_NT_INDBELLMSK:
            s->regs->db_mask = val;
            IVSHMEM_DPRINTF("Set the inbound doorbell mask to value 0x%lx\n", val);
            break;
        case IDT_NT_MSGSTS:
            s->regs->msgsts &= ~val; /* Substraction with a module of 2 */
            IVSHMEM_DPRINTF("Cleared the local MSGSTS, new value: 0x%lx (vm%d)\n",
                    s->regs->msgsts, s->vm_id + 1);
            break;
        case IDT_NT_MSGSTSMSK:
            s->regs->msgsts_mask = val;
            IVSHMEM_DPRINTF("Set the local MSGSTSMSK to value 0x%lx\n", val);
            break;
        case IDT_NT_NTGSIGNAL:
            assert(val == 1ULL); /* Constrained by device spec */
            s->gregs->segsigsts |= (1UL << 0); /* Assume partition 0 */
            /* TODO: honor SEGSIGMSK */
            s->peer_regs->intsts |= IDT_NTINTSTS_SEVENT;
            event_notifier_set(&s->peers[s->other_vm_id].eventfds[EVENTFD_INTERRUPT_HOST]);
            IVSHMEM_DPRINTF("Write to NTGSIGNAL: set flag in SEGSIGSTS and sent an interrupt (vm%d -> vm%d)\n",
                    s->vm_id, s->other_vm_id);
            break;

#define WRITE_BARREG(reg, fld, ind) case IDT_NT_ ## reg ## ind: \
            s->bar_config[ind].fld = (uint32_t)val; \
            IVSHMEM_DPRINTF("Set the local %s%d to value 0x%x\n", #reg, ind, (uint32_t)val); \
            break;
#define WRITE_BARREGS(reg, fld) WRITE_BARREG(reg, fld, 0) \
            WRITE_BARREG(reg, fld, 1) \
            WRITE_BARREG(reg, fld, 2) \
            WRITE_BARREG(reg, fld, 3) \
            WRITE_BARREG(reg, fld, 4) \
            WRITE_BARREG(reg, fld, 5)

        WRITE_BARREGS(BARSETUP, setup)
        WRITE_BARREGS(BARLIMIT, limit)
        WRITE_BARREGS(BARLTBASE, ltbase)
        WRITE_BARREGS(BARUTBASE, utbase)

#undef WRITE_BARREGS
#undef WRITE_BARREG

        default:
            IVSHMEM_DPRINTF("Invalid write at addr " HWADDR_FMT_plx  " for config space\n", addr);
    }
}

static uint64_t ivshmem_io_read(void *opaque, hwaddr addr,
                                unsigned size)
{

    IVShmemState *s = opaque;
    uint64_t ret;

    switch (addr)
    {
        case GASADATA:
            ret = get_gasadata(s);
            IVSHMEM_DPRINTF("Handled GASADATA read: 0x%lx\n", ret);
            break;
        case IDT_NT_PCIELCAP:
            ret = (s->self_number == 0 ? VALUE_NT_PCIELCAP_VM1 : VALUE_NT_PCIELCAP_VM2);
            IVSHMEM_DPRINTF("Read PCIELCAP: 0x%lx\n", ret);
            break;
        case IDT_NT_PCIELCTLSTS:
            ret = VALUE_NT_PCIELCTLSTS;
            IVSHMEM_DPRINTF("Read PCIELCTLSTS: 0x%lx\n", ret);
            break;
        case IDT_NT_PCICMDSTS:
            ret = VALUE_NT_PCICMDSTS;
            IVSHMEM_DPRINTF("Read PCICMDSTS: 0x%lx\n", ret);
            break;
        case IDT_NT_NTCTL:
            ret = s->regs->ntctl;
            IVSHMEM_DPRINTF("Read local NTCTL: 0x%lx\n", ret);
            break;
        case IDT_NT_NTMTBLDATA:
            ret = get_nt_mtb_data(s);
            IVSHMEM_DPRINTF("Handled NTMTBLDATA read: 0x%lx\n", ret);
            break;
        case IDT_NT_NTINTSTS:  /* used when handling interrupts */
            ret = s->regs->intsts;
            IVSHMEM_DPRINTF("Read local NTINTSTS: 0x%lx\n", ret);
            break;
        case IDT_NT_INMSG0:
            ret = s->regs->msg[0];
            IVSHMEM_DPRINTF("Read value 0x%lx from the inbound message register %d\n", ret, 0);
            break;
        case IDT_NT_INMSG1:
            IVSHMEM_DPRINTF("Read value 0x%lx from the inbound message register %d\n", ret, 1);
            ret = s->regs->msg[1];
            break;
        case IDT_NT_INMSG2:
            IVSHMEM_DPRINTF("Read value 0x%lx from the inbound message register %d\n", ret, 2);
            ret = s->regs->msg[2];
            break;
        case IDT_NT_INMSG3:
            IVSHMEM_DPRINTF("Read value 0x%lx from the inbound message register %d\n", ret, 3);
            ret = s->regs->msg[3];
            break;
        case IDT_NT_INDBELLSTS:
            ret = s->regs->db;
            IVSHMEM_DPRINTF("Read the inbound doorbell: 0x%lx\n", ret);
            break;
        case IDT_NT_INDBELLMSK:
            ret = s->regs->db_mask;
            IVSHMEM_DPRINTF("Read the inbound doorbell mask: 0x%lx\n", ret);
            break;
        case IDT_NT_MSGSTS:
            ret = s->regs->msgsts;
            IVSHMEM_DPRINTF("Read the local MSGSTS: 0x%lx\n", ret);
            break;
        case IDT_NT_MSGSTSMSK:
            ret = s->regs->msgsts_mask;
            IVSHMEM_DPRINTF("Read the local MSGSTSMSK: 0x%lx\n", ret);
            break;

#define READ_BARREG(reg, fld, ind) case IDT_NT_ ## reg ## ind: \
            ret = s->bar_config[ind].fld; \
            IVSHMEM_DPRINTF("Read the local %s%d: 0x%lx\n", #reg, ind, ret); \
            break;
#define READ_BARREGS(reg, fld) READ_BARREG(reg, fld, 0) \
            READ_BARREG(reg, fld, 1) \
            READ_BARREG(reg, fld, 2) \
            READ_BARREG(reg, fld, 3) \
            READ_BARREG(reg, fld, 4) \
            READ_BARREG(reg, fld, 5)

        READ_BARREGS(BARSETUP, setup)
        READ_BARREGS(BARLIMIT, limit)
        READ_BARREGS(BARLTBASE, ltbase)
        READ_BARREGS(BARUTBASE, utbase)

#undef READ_BARREGS
#undef READ_BARREG

        default:
            IVSHMEM_DPRINTF("Invalid read at addr " HWADDR_FMT_plx " for config space\n", addr);
            ret = 0;
    }

    return ret;
}

static const MemoryRegionOps ivshmem_mmio_ops = {
    .read = ivshmem_io_read,
    .write = ivshmem_io_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void *map_peer_shm(uint32_t self_number) {
        int fd;
        struct stat statbuf;
        void *ret;

        IVSHMEM_DPRINTF("Trying to map peer physical memory\n");

        /* Map peer physical memory */
        /* fd = shm_open(s->self_number == 0 ? "qemu2" : "qemu1", O_RDWR, 0); - requires -lrt */
        fd = open(self_number == 0 ? "/dev/shm/qemu2" : "/dev/shm/qemu1", O_RDWR);
        if (fd == -1) {
            error_report("idt-ntb-ivshmem: can't open shm, MW access failed\n");
            return NULL;
        }

        if (fstat(fd, &statbuf) == -1) {
            perror("mapping shm failed: fstat");
            return NULL;
        }

        ret = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ret == MAP_FAILED) {
            perror("mapping shm failed: mmap");
            return NULL;
        }

        return ret;
}

static uint64_t idt_bar_read(void *opaque, hwaddr addr,
                             unsigned size, int idx)
{
    IVShmemState *s = opaque;
    BARConfig c = s->bar_config[idx];
    uint64_t ret;

    if (!c.limit) {
        error_report("idt-ntb-ivshmem: memory window is inactive, reading 0");
        return 0;
    }

    if ((s->bars[idx].addr + addr) > c.limit) {
        error_report("idt-ntb-ivshmem: BAR read past limit addr, reading 0 "
                "(bar: 0x%lx, offset: 0x%lx)", s->bars[idx].addr, addr);
        return 0;
    }

    if (s->peer_mem == NULL) {
        if ((s->peer_mem = map_peer_shm(s->self_number)) == NULL) {
            return 0;
        }
    }

    // TODO: honor size
    ret = *(uint64_t *)(s->peer_mem + ((uint64_t)c.utbase << 32) + c.ltbase + addr);

    IVSHMEM_DPRINTF("BAR%d region read: value 0x%lx at offset 0x%lx (bar is at 0x%lx)\n",
            idx, ret, addr, s->bars[idx].addr);

    return ret;
}

static void idt_bar_write(void *opaque, hwaddr addr,
                          uint64_t value, unsigned size, int idx)
{
    IVShmemState *s = opaque;
    BARConfig c = s->bar_config[idx];

    if (!c.limit) {
        error_report("idt-ntb-ivshmem: memory window is inactive, refusing to write");
        return;
    }

    if ((s->bars[idx].addr + addr) > c.limit) {
        error_report("idt-ntb-ivshmem: BAR write past limit addr, refusing to write "
                "(bar: 0x%lx, offset: 0x%lx)", s->bars[idx].addr, addr);
        return;
    }

    if (s->peer_mem == NULL) {
        if ((s->peer_mem = map_peer_shm(s->self_number)) == NULL) {
            return;
        }
    }

    IVSHMEM_DPRINTF("BAR%d region write: value 0x%lx at offset 0x%lx (bar is at 0x%lx)\n",
            idx, value, addr, s->bars[idx].addr);

    // TODO: honor size
    *(uint64_t *)(s->peer_mem + ((uint64_t)c.utbase << 32) + c.ltbase + addr) = value;
}

#define BAR_READ_FN(n) static uint64_t idt_bar ## n ## _read(void *opaque, \
        hwaddr addr, unsigned size) \
    { \
        return idt_bar_read(opaque, addr, size, n); \
    }
#define BAR_WRITE_FN(n) static void idt_bar ## n ## _write(void *opaque, \
        hwaddr addr, uint64_t value, unsigned size) \
    { \
        return idt_bar_write(opaque, addr, value, size, n); \
    }
#define BAR_FNS(n) BAR_READ_FN(n) \
    BAR_WRITE_FN(n)

BAR_FNS(2)
BAR_FNS(3)
BAR_FNS(4)
BAR_FNS(5)

#undef BAR_FNS
#undef BAR_WRITE_FN
#undef BAR_READ_FN

#define BAR_OPS(n) static const MemoryRegionOps idt_bar ## n ## _ops = { \
    .read = idt_bar ## n ## _read, \
    .write = idt_bar ## n ## _write, \
    .endianness = DEVICE_LITTLE_ENDIAN, \
    .impl = { \
        .min_access_size = 8, \
        .max_access_size = 8, \
    }, \
}

BAR_OPS(2);
BAR_OPS(3);
BAR_OPS(4);
BAR_OPS(5);

#undef BAR_OPS

static void ivshmem_vector_notify(void *opaque)
{
    MSIVector *entry = opaque;
    PCIDevice *pdev = entry->pdev;
    IVShmemState *s = IVSHMEM_NTB_IDT(pdev);
    int vector = entry - s->msi_vectors;
    EventNotifier *n = &s->peers[s->vm_id].eventfds[vector];

    if (!event_notifier_test_and_clear(n)) {
        IVSHMEM_DPRINTF("Event notifier error\n");
        return;
    }

    /* TODO: honor interrupt masks */
    IVSHMEM_DPRINTF("interrupt on vector %p %d (self_number is %d)\n", pdev, vector, s->self_number);
    switch (vector) {
        case EVENTFD_VM_ID:
            s->other_vm_id = *s->other_vm_id_shared;
            break;
        case EVENTFD_INTERRUPT_HOST:
            IVSHMEM_DPRINTF("Passing interrupt to the host (vm%d)\n", s->vm_id);
            interrupt_notify(s, 0);
            break;
        default:
            error_report("idt-ntb-ivshmem: event interrupt on unknown vector %d", vector);
            break;
    }
}

static int ivshmem_vector_unmask(PCIDevice *dev, unsigned vector,
                                 MSIMessage msg)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(dev);
    EventNotifier *n = &s->peers[s->vm_id].eventfds[vector];
    MSIVector *v = &s->msi_vectors[vector];
    int ret;

    IVSHMEM_DPRINTF("vector unmask %p %d\n", dev, vector);
    if (!v->pdev) {
        error_report("ivshmem: vector %d route does not exist", vector);
        return -EINVAL;
    }
    assert(!v->unmasked);

    ret = kvm_irqchip_update_msi_route(kvm_state, v->virq, msg, dev);
    if (ret < 0) {
        return ret;
    }
    kvm_irqchip_commit_routes(kvm_state);

    ret = kvm_irqchip_add_irqfd_notifier_gsi(kvm_state, n, NULL, v->virq);
    if (ret < 0) {
        return ret;
    }
    v->unmasked = true;

    return 0;
}

static void ivshmem_vector_mask(PCIDevice *dev, unsigned vector)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(dev);
    EventNotifier *n = &s->peers[s->vm_id].eventfds[vector];
    MSIVector *v = &s->msi_vectors[vector];
    int ret;

    IVSHMEM_DPRINTF("vector mask %p %d\n", dev, vector);
    if (!v->pdev) {
        error_report("ivshmem: vector %d route does not exist", vector);
        return;
    }
    assert(v->unmasked);

    ret = kvm_irqchip_remove_irqfd_notifier_gsi(kvm_state, n, v->virq);
    if (ret < 0) {
        error_report("remove_irqfd_notifier_gsi failed");
        return;
    }
    v->unmasked = false;
}

static void ivshmem_vector_poll(PCIDevice *dev,
                                unsigned int vector_start,
                                unsigned int vector_end)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(dev);
    unsigned int vector;

    IVSHMEM_DPRINTF("vector poll %p %d-%d\n", dev, vector_start, vector_end);

    vector_end = MIN(vector_end, s->vectors);

    for (vector = vector_start; vector < vector_end; vector++) {
        EventNotifier *notifier = &s->peers[s->vm_id].eventfds[vector];

        if (!msix_is_masked(dev, vector)) {
            continue;
        }

        if (event_notifier_test_and_clear(notifier)) {
            msix_set_pending(dev, vector);
        }
    }
}

static void watch_vector_notifier(IVShmemState *s, EventNotifier *n,
                                 int vector)
{
    int eventfd = event_notifier_get_fd(n);

    assert(!s->msi_vectors[vector].pdev);
    s->msi_vectors[vector].pdev = PCI_DEVICE(s);

    qemu_set_fd_handler(eventfd, ivshmem_vector_notify,
                        NULL, &s->msi_vectors[vector]);
}

static void ivshmem_add_eventfd(IVShmemState *s, int posn, int i)
{
    /* TODO: Used with KVM. Is it needed? */
    /* memory_region_add_eventfd(&s->ivshmem_mmio,
                              DOORBELL,
                              4,
                              true,
                              (posn << 16) | i,
                              &s->peers[posn].eventfds[i]);
    */
}

static void ivshmem_del_eventfd(IVShmemState *s, int posn, int i)
{
    /* TODO: Used with KVM. Is it needed? */
    /* memory_region_del_eventfd(&s->ivshmem_mmio,
                              DOORBELL,
                              4,
                              true,
                              (posn << 16) | i,
                              &s->peers[posn].eventfds[i]);
    */
}

static void close_peer_eventfds(IVShmemState *s, int posn)
{
    int i, n;

    assert(posn >= 0 && posn < s->nb_peers);
    n = s->peers[posn].nb_eventfds;

    if (ivshmem_has_feature(s, IVSHMEM_IOEVENTFD)) {
        memory_region_transaction_begin();
        for (i = 0; i < n; i++) {
            ivshmem_del_eventfd(s, posn, i);
        }
        memory_region_transaction_commit();
    }

    for (i = 0; i < n; i++) {
        event_notifier_cleanup(&s->peers[posn].eventfds[i]);
    }

    g_free(s->peers[posn].eventfds);
    s->peers[posn].nb_eventfds = 0;
}

static void resize_peers(IVShmemState *s, int nb_peers)
{
    int old_nb_peers = s->nb_peers;
    int i;

    assert(nb_peers > old_nb_peers);
    IVSHMEM_DPRINTF("bumping storage to %d peers\n", nb_peers);

    s->peers = g_renew(Peer, s->peers, nb_peers);
    s->nb_peers = nb_peers;

    for (i = old_nb_peers; i < nb_peers; i++) {
        s->peers[i].eventfds = g_new0(EventNotifier, s->vectors);
        s->peers[i].nb_eventfds = 0;
    }
}

static void ivshmem_add_kvm_msi_virq(IVShmemState *s, int vector,
                                     Error **errp)
{
    PCIDevice *pdev = PCI_DEVICE(s);
    KVMRouteChange c;
    int ret;

    IVSHMEM_DPRINTF("ivshmem_add_kvm_msi_virq vector:%d\n", vector);
    assert(!s->msi_vectors[vector].pdev);

    c = kvm_irqchip_begin_route_changes(kvm_state);
    ret = kvm_irqchip_add_msi_route(&c, vector, pdev);
    if (ret < 0) {
        error_setg(errp, "kvm_irqchip_add_msi_route failed");
        return;
    }
    kvm_irqchip_commit_route_changes(&c);

    s->msi_vectors[vector].virq = ret;
    s->msi_vectors[vector].pdev = pdev;
}

static void setup_interrupt(IVShmemState *s, int vector, Error **errp)
{
    EventNotifier *n = &s->peers[s->vm_id].eventfds[vector];
    bool with_irqfd = kvm_msi_via_irqfd_enabled() &&
        ivshmem_has_feature(s, IVSHMEM_MSI);
    PCIDevice *pdev = PCI_DEVICE(s);
    Error *err = NULL;

    IVSHMEM_DPRINTF("setting up interrupt for vector: %d\n", vector);

    if (!with_irqfd) {
        IVSHMEM_DPRINTF("with eventfd\n");
        watch_vector_notifier(s, n, vector);
    } else if (msix_enabled(pdev)) {
        IVSHMEM_DPRINTF("with irqfd\n");
        ivshmem_add_kvm_msi_virq(s, vector, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }

        if (!msix_is_masked(pdev, vector)) {
            kvm_irqchip_add_irqfd_notifier_gsi(kvm_state, n, NULL,
                                               s->msi_vectors[vector].virq);
            /* TODO handle error */
        }
    } else {
        /* it will be delayed until msix is enabled, in write_config */
        IVSHMEM_DPRINTF("with irqfd, delayed until msix enabled\n");
    }
    if (vector == 1){
        init_vm_ids(s);
    }
}

static void process_msg_shmem(IVShmemState *s, int fd, Error **errp)
{
    Error *local_err = NULL;
    struct stat buf;
    size_t size;

    if (s->ivshmem_bar2) {
        error_setg(errp, "server sent unexpected shared memory message");
        close(fd);
        return;
    }

    if (fstat(fd, &buf) < 0) {
        error_setg_errno(errp, errno,
            "can't determine size of shared memory sent by server");
        close(fd);
        return;
    }

    size = buf.st_size;

    /* mmap the region and map into the BAR2 */
    memory_region_init_ram_from_fd(&s->server_bar2, OBJECT(s), "ivshmem.bar2",
                                   size, RAM_SHARED, fd, 0, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    s->ivshmem_bar2 = &s->server_bar2;
}

static void process_msg_disconnect(IVShmemState *s, uint16_t posn,
                                   Error **errp)
{
    IVSHMEM_DPRINTF("posn %d has gone away\n", posn);
    if (posn >= s->nb_peers || posn == s->vm_id) {
        error_setg(errp, "invalid peer %d", posn);
        return;
    }
    close_peer_eventfds(s, posn);
}

static void process_msg_connect(IVShmemState *s, uint16_t posn, int fd,
                                Error **errp)
{
    Peer *peer = &s->peers[posn];
    int vector;

    /*
     * The N-th connect message for this peer comes with the file
     * descriptor for vector N-1.  Count messages to find the vector.
     */
    if (peer->nb_eventfds >= s->vectors) {
        error_setg(errp, "Too many eventfd received, device has %d vectors",
                   s->vectors);
        close(fd);
        return;
    }
    vector = peer->nb_eventfds++;

    IVSHMEM_DPRINTF("eventfds[%d][%d] = %d\n", posn, vector, fd);
    event_notifier_init_fd(&peer->eventfds[vector], fd);
    g_unix_set_fd_nonblocking(fd, true, NULL); /* msix/irqfd poll non block */

    if (posn == s->vm_id) {
        setup_interrupt(s, vector, errp);
        /* TODO do we need to handle the error? */
    }

    if (ivshmem_has_feature(s, IVSHMEM_IOEVENTFD)) {
        ivshmem_add_eventfd(s, posn, vector);
    }
}

static void process_msg(IVShmemState *s, int64_t msg, int fd, Error **errp)
{
    IVSHMEM_DPRINTF("posn is %" PRId64 ", fd is %d\n", msg, fd);

    if (msg < -1 || msg > IVSHMEM_MAX_PEERS) {
        error_setg(errp, "server sent invalid message %" PRId64, msg);
        close(fd);
        return;
    }

    if (msg == -1) {
        process_msg_shmem(s, fd, errp);
        return;
    }

    if (msg >= s->nb_peers) {
        resize_peers(s, msg + 1);
    }

    if (fd >= 0) {
        process_msg_connect(s, msg, fd, errp);
    } else {
        process_msg_disconnect(s, msg, errp);
    }
}

static int ivshmem_can_receive(void *opaque)
{
    IVShmemState *s = opaque;

    assert(s->msg_buffered_bytes < sizeof(s->msg_buf));
    return sizeof(s->msg_buf) - s->msg_buffered_bytes;
}

static void ivshmem_read(void *opaque, const uint8_t *buf, int size)
{
    IVShmemState *s = opaque;
    Error *err = NULL;
    int fd;
    int64_t msg;

    assert(size >= 0 && s->msg_buffered_bytes + size <= sizeof(s->msg_buf));
    memcpy((unsigned char *)&s->msg_buf + s->msg_buffered_bytes, buf, size);
    s->msg_buffered_bytes += size;
    if (s->msg_buffered_bytes < sizeof(s->msg_buf)) {
        return;
    }
    msg = le64_to_cpu(s->msg_buf);
    s->msg_buffered_bytes = 0;

    fd = qemu_chr_fe_get_msgfd(&s->server_chr);

    process_msg(s, msg, fd, &err);
    if (err) {
        error_report_err(err);
    }
}

static int64_t ivshmem_recv_msg(IVShmemState *s, int *pfd, Error **errp)
{
    int64_t msg;
    int n, ret;

    n = 0;
    do {
        ret = qemu_chr_fe_read_all(&s->server_chr, (uint8_t *)&msg + n,
                                   sizeof(msg) - n);
        if (ret < 0) {
            if (ret == -EINTR) {
                continue;
            }
            error_setg_errno(errp, -ret, "read from server failed");
            return INT64_MIN;
        }
        n += ret;
    } while (n < sizeof(msg));

    *pfd = qemu_chr_fe_get_msgfd(&s->server_chr);
    return le64_to_cpu(msg);
}

static void ivshmem_recv_setup(IVShmemState *s, Error **errp)
{
    Error *err = NULL;
    int64_t msg;
    int fd;

    msg = ivshmem_recv_msg(s, &fd, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    if (msg != IVSHMEM_PROTOCOL_VERSION) {
        error_setg(errp, "server sent version %" PRId64 ", expecting %d",
                   msg, IVSHMEM_PROTOCOL_VERSION);
        return;
    }
    if (fd != -1) {
        error_setg(errp, "server sent invalid version message");
        return;
    }

    /*
     * ivshmem-server sends the remaining initial messages in a fixed
     * order, but the device has always accepted them in any order.
     * Stay as compatible as practical, just in case people use
     * servers that behave differently.
     */

    /*
     * ivshmem_device_spec.txt has always required the ID message
     * right here, and ivshmem-server has always complied.  However,
     * older versions of the device accepted it out of order, but
     * broke when an interrupt setup message arrived before it.
     */
    msg = ivshmem_recv_msg(s, &fd, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    if (fd != -1 || msg < 0 || msg > IVSHMEM_MAX_PEERS) {
        error_setg(errp, "server sent invalid ID message");
        return;
    }
    s->vm_id = msg;

    /*
     * Receive more messages until we got shared memory.
     */
    do {
        msg = ivshmem_recv_msg(s, &fd, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
        process_msg(s, msg, fd, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }
    } while (msg != -1);

    /*
     * This function must either map the shared memory or fail.  The
     * loop above ensures that: it terminates normally only after it
     * successfully processed the server's shared memory message.
     * Assert that actually mapped the shared memory:
     */
    assert(s->ivshmem_bar2);
}

/* Select the MSI-X vectors used by device.
 * ivshmem maps events to vectors statically, so
 * we just enable all vectors on init and after reset. */
static void ivshmem_msix_vector_use(IVShmemState *s)
{
    PCIDevice *d = PCI_DEVICE(s);
    int i;

    for (i = 0; i < s->vectors; i++) {
        msix_vector_use(d, i);
    }
}

static void ivshmem_disable_irqfd(IVShmemState *s);

static void ivshmem_reset(DeviceState *d)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(d);

    ivshmem_disable_irqfd(s);

    if (ivshmem_has_feature(s, IVSHMEM_MSI)) {
        ivshmem_msix_vector_use(s);
    }
}

static int ivshmem_setup_interrupts(IVShmemState *s, Error **errp)
{
    /* allocate QEMU callback data for receiving interrupts */
    s->msi_vectors = g_new0(MSIVector, s->vectors);

    if (ivshmem_has_feature(s, IVSHMEM_MSI)) {
        if (msix_init_exclusive_bar(PCI_DEVICE(s), s->vectors, 1, errp)) {
            return -1;
        }

        IVSHMEM_DPRINTF("msix initialized (%d vectors)\n", s->vectors);
        ivshmem_msix_vector_use(s);
    }

    return 0;
}

static void ivshmem_remove_kvm_msi_virq(IVShmemState *s, int vector)
{
    IVSHMEM_DPRINTF("ivshmem_remove_kvm_msi_virq vector:%d\n", vector);

    if (s->msi_vectors[vector].pdev == NULL) {
        return;
    }

    /* it was cleaned when masked in the frontend. */
    kvm_irqchip_release_virq(kvm_state, s->msi_vectors[vector].virq);

    s->msi_vectors[vector].pdev = NULL;
}

static void ivshmem_enable_irqfd(IVShmemState *s)
{
    PCIDevice *pdev = PCI_DEVICE(s);
    int i;

    for (i = 0; i < s->peers[s->vm_id].nb_eventfds; i++) {
        Error *err = NULL;

        ivshmem_add_kvm_msi_virq(s, i, &err);
        if (err) {
            error_report_err(err);
            goto undo;
        }
    }

    if (msix_set_vector_notifiers(pdev,
                                  ivshmem_vector_unmask,
                                  ivshmem_vector_mask,
                                  ivshmem_vector_poll)) {
        error_report("ivshmem: msix_set_vector_notifiers failed");
        goto undo;
    }
    return;

undo:
    while (--i >= 0) {
        ivshmem_remove_kvm_msi_virq(s, i);
    }
}

static void ivshmem_disable_irqfd(IVShmemState *s)
{
    PCIDevice *pdev = PCI_DEVICE(s);
    int i;

    if (!pdev->msix_vector_use_notifier) {
        return;
    }

    msix_unset_vector_notifiers(pdev);

    for (i = 0; i < s->peers[s->vm_id].nb_eventfds; i++) {
        /*
         * MSI-X is already disabled here so msix_unset_vector_notifiers()
         * didn't call our release notifier.  Do it now to keep our masks and
         * unmasks balanced.
         */
        if (s->msi_vectors[i].unmasked) {
            ivshmem_vector_mask(pdev, i);
        }
        ivshmem_remove_kvm_msi_virq(s, i);
    }

}

static void ivshmem_write_config(PCIDevice *pdev, uint32_t address,
                                 uint32_t val, int len)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(pdev);
    IVSHMEM_DPRINTF("Writing config value 0x%x to address 0x%x\n", val, address);
    int is_enabled, was_enabled = msix_enabled(pdev);

    pci_default_write_config(pdev, address, val, len);
    is_enabled = msix_enabled(pdev);

    if (kvm_msi_via_irqfd_enabled()) {
        if (!was_enabled && is_enabled) {
            ivshmem_enable_irqfd(s);
        } else if (was_enabled && !is_enabled) {
            ivshmem_disable_irqfd(s);
        }
    }
}

static uint32_t ivshmem_read_config(PCIDevice *pdev, uint32_t address,
                                 int len)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(pdev);
    uint32_t ret;
    ret = pci_default_read_config(pdev, address, len);
    IVSHMEM_DPRINTF("Default config value at address 0x%x is 0x%x\n", address, ret);
    switch(address){
        case IDT_NT_BARSETUP0:
            ret = s->bar_config[0].setup;
            break;
        default:
            ret = pci_default_read_config(pdev, address, len);
    };
    IVSHMEM_DPRINTF("Reading config value 0x%x at address 0x%x\n", ret, address);
    return ret;
}

static void ivshmem_common_realize(PCIDevice *dev, Error **errp)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(dev);
    Error *err = NULL;
    uint8_t *pci_conf;
    struct idt_ivshmem_shm_storage *shm_storage;

    /* IRQFD requires MSI */
    if (ivshmem_has_feature(s, IVSHMEM_IOEVENTFD) &&
        !ivshmem_has_feature(s, IVSHMEM_MSI)) {
        error_setg(errp, "ioeventfd/irqfd requires MSI");
        return;
    }

    pci_conf = dev->config;
    pci_conf[PCI_COMMAND] = PCI_COMMAND_IO | PCI_COMMAND_MEMORY;

    memory_region_init_io(&s->bars[0], OBJECT(s), &ivshmem_mmio_ops, s,
                          "idt-mmio", IVSHMEM_REG_BAR_SIZE);

    memory_region_init_io(&s->bars[2], OBJECT(s), &idt_bar2_ops, s,
                          "idt-bar2", IDT_BAR_APERTURE_SIZE);
    memory_region_init_io(&s->bars[3], OBJECT(s), &idt_bar3_ops, s,
                          "idt-bar3", IDT_BAR_APERTURE_SIZE);
    memory_region_init_io(&s->bars[4], OBJECT(s), &idt_bar4_ops, s,
                          "idt-bar4", IDT_BAR_APERTURE_SIZE);
    //memory_region_init_io(&s->bars[5], OBJECT(s), &idt_bar5_ops, s,
    //                      "idt-bar5", IDT_BAR_APERTURE_SIZE);

    /* NT Configuration Space */
    pci_register_bar(dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &s->bars[0]);

    pci_register_bar(dev, 2, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &s->bars[2]);
    pci_register_bar(dev, 3, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &s->bars[3]);
    pci_register_bar(dev, 4, PCI_BASE_ADDRESS_SPACE_MEMORY,
                     &s->bars[4]);
    //pci_register_bar(dev, 5, PCI_BASE_ADDRESS_SPACE_MEMORY,
    //                 &s->bars[5]);

    if (s->hostmem != NULL) {
        IVSHMEM_DPRINTF("using hostmem\n");

        s->ivshmem_bar2 = host_memory_backend_get_memory(s->hostmem);
        host_memory_backend_set_mapped(s->hostmem, true);
    } else {
        Chardev *chr = qemu_chr_fe_get_driver(&s->server_chr);
        assert(chr);

        IVSHMEM_DPRINTF("using shared memory server (socket = %s)\n",
                        chr->filename);

        /* we allocate enough space for 16 peers and grow as needed */
        resize_peers(s, 16);

        /*
         * Receive setup messages from server synchronously.
         * Older versions did it asynchronously, but that creates a
         * number of entertaining race conditions.
         */
        ivshmem_recv_setup(s, &err);
        if (err) {
            error_propagate(errp, err);
            return;
        }

        if (s->master == ON_OFF_AUTO_ON && s->vm_id != 0) {
            error_setg(errp,
                       "master must connect to the server before any peers");
            return;
        }

        qemu_chr_fe_set_handlers(&s->server_chr, ivshmem_can_receive,
                                 ivshmem_read, NULL, NULL, s, NULL, true);

        if (ivshmem_setup_interrupts(s, errp) < 0) {
            error_prepend(errp, "Failed to initialize interrupts: ");
            return;
        }
        if (msi_init(dev, 0, 1, false, false, errp)) {  /* Setup device for msi interrupts, see pci/msi.h */
            error_prepend(errp, "Failed to initialize MSI interrupts: ");
            return;
        }
    }

    if (s->master == ON_OFF_AUTO_AUTO) {
        s->master = s->vm_id == 0 ? ON_OFF_AUTO_ON : ON_OFF_AUTO_OFF;
    }

    if (!ivshmem_is_master(s)) {
        error_setg(&s->migration_blocker,
                   "Migration is disabled when using feature 'peer mode' in device 'ivshmem'");
        if (migrate_add_blocker(s->migration_blocker, errp) < 0) {
            error_free(s->migration_blocker);
            return;
        }
    }

    vmstate_register_ram(s->ivshmem_bar2, DEVICE(s));
    //pci_register_bar(PCI_DEVICE(s), 2,
    //                 PCI_BASE_ADDRESS_SPACE_MEMORY |
    //                 PCI_BASE_ADDRESS_MEM_PREFETCH |
    //                 PCI_BASE_ADDRESS_MEM_TYPE_64,
    //                 s->ivshmem_bar2);

    // Initialize pointers to various registers
    shm_storage = (struct idt_ivshmem_shm_storage*)memory_region_get_ram_ptr(s->ivshmem_bar2);

    s->gregs = &shm_storage->gregs;

    if (s->self_number == 0) {
        s->vm_id_shared = &shm_storage->vm1.id;
        s->other_vm_id_shared = &shm_storage->vm2.id;

        s->regs = &shm_storage->vm1.regs;
        s->peer_regs = &shm_storage->vm2.regs;

        s->regs->ntctl = VALUE_NTP0_NTCTL;

        /* Initialize BAR register configurations */
        s->bar_config = shm_storage->vm1.bar_config;
    } else {
        s->vm_id_shared = &shm_storage->vm2.id;
        s->other_vm_id_shared = &shm_storage->vm1.id;

        s->regs = &shm_storage->vm2.regs;
        s->peer_regs = &shm_storage->vm1.regs;

        s->regs->ntctl = VALUE_NTP2_NTCTL;

        /* Initialize BAR register configurations */
        s->bar_config = shm_storage->vm2.bar_config;
    }

    s->bar_config[0].setup = VALUE_NT_BARSETUP0;

    s->bar_config[2].setup = VALUE_NT_BARSETUP2;
    s->bar_config[3].setup = VALUE_NT_BARSETUP3;
    s->bar_config[4].setup = VALUE_NT_BARSETUP4;
    //s->bar_config[5].setup = VALUE_NT_BARSETUP5;

    /* Pre-initialize peer BARSETUPs so that port scan results are always correct */
    {
        BARConfig *peer_bar_config =
            s->self_number == 0 ? shm_storage->vm2.bar_config : shm_storage->vm1.bar_config;
        if (!peer_bar_config[0].setup) {
            for (int i = 0; i < 6; i++) {
                peer_bar_config[i] = s->bar_config[i];
            }
        }
    }
}

static void ivshmem_exit(PCIDevice *dev)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(dev);
    int i;

    if (s->migration_blocker) {
        migrate_del_blocker(s->migration_blocker);
        error_free(s->migration_blocker);
    }

    if (memory_region_is_mapped(s->ivshmem_bar2)) {
        if (!s->hostmem) {
            void *addr = memory_region_get_ram_ptr(s->ivshmem_bar2);
            int fd;

            if (munmap(addr, memory_region_size(s->ivshmem_bar2) == -1)) {
                error_report("Failed to munmap shared memory %s",
                             strerror(errno));
            }

            fd = memory_region_get_fd(s->ivshmem_bar2);
            close(fd);
        }

        vmstate_unregister_ram(s->ivshmem_bar2, DEVICE(dev));
    }

    if (s->hostmem) {
        host_memory_backend_set_mapped(s->hostmem, false);
    }

    if (s->peers) {
        for (i = 0; i < s->nb_peers; i++) {
            close_peer_eventfds(s, i);
        }
        g_free(s->peers);
    }

    if (ivshmem_has_feature(s, IVSHMEM_MSI)) {
        msix_uninit_exclusive_bar(dev);
    }

    g_free(s->msi_vectors);
}

static int ivshmem_pre_load(void *opaque)
{
    IVShmemState *s = opaque;

    if (!ivshmem_is_master(s)) {
        error_report("'peer' devices are not migratable");
        return -EINVAL;
    }

    return 0;
}

static int ivshmem_post_load(void *opaque, int version_id)
{
    IVShmemState *s = opaque;

    if (ivshmem_has_feature(s, IVSHMEM_MSI)) {
        ivshmem_msix_vector_use(s);
    }
    return 0;
}

static const VMStateDescription ivshmem_idt_ntb_vmsd = {
    .name = TYPE_IVSHMEM_NTB_IDT,
    .version_id = 0,
    .minimum_version_id = 0,
    .pre_load = ivshmem_pre_load,
    .post_load = ivshmem_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(parent_obj, IVShmemState),
        VMSTATE_MSIX(parent_obj, IVShmemState),
        VMSTATE_END_OF_LIST()
    },
};

static Property ivshmem_idt_ntb_properties[] = {
    DEFINE_PROP_CHR("chardev", IVShmemState, server_chr),
    DEFINE_PROP_UINT32("vectors", IVShmemState, vectors, 1),
    DEFINE_PROP_UINT32("number", IVShmemState, self_number, 0),
    DEFINE_PROP_BIT("ioeventfd", IVShmemState, features, IVSHMEM_IOEVENTFD,
                    true),
    DEFINE_PROP_ON_OFF_AUTO("master", IVShmemState, master, ON_OFF_AUTO_OFF),
    DEFINE_PROP_END_OF_LIST(),
};

static void ivshmem_ntb_idt_init(Object *obj)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(obj);

    s->features |= (1 << IVSHMEM_MSI);
    s->other_vm_id = -1;
}

static void ivshmem_ntb_idt_realize(PCIDevice *dev, Error **errp)
{
    IVShmemState *s = IVSHMEM_NTB_IDT(dev);

    if (!qemu_chr_fe_backend_connected(&s->server_chr)) {
        error_setg(errp, "You must specify a 'chardev'");
        return;
    }

    ivshmem_common_realize(dev, errp);
}

static void ivshmem_ntb_idt_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->realize = ivshmem_ntb_idt_realize;
    k->exit = ivshmem_exit;
    k->config_write = ivshmem_write_config;
    k->config_read = ivshmem_read_config;
    k->vendor_id = PCI_VENDOR_ID_IVSHMEM;
    k->device_id = PCI_DEVICE_ID_IVSHMEM;
    k->class_id = PCI_CLASS_BRIDGE_OTHER;
    k->revision = 0;
    dc->reset = ivshmem_reset;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    device_class_set_props(dc, ivshmem_idt_ntb_properties);
    dc->desc = "IDT NTB over shared memory";
    dc->vmsd = &ivshmem_idt_ntb_vmsd;
}

static const TypeInfo ivshmem_ntb_idt_info = {
    .name          = TYPE_IVSHMEM_NTB_IDT,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(IVShmemState),
    .class_init    = ivshmem_ntb_idt_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_PCIE_DEVICE },
        { },
    },
    .instance_init = ivshmem_ntb_idt_init,
};

static void ivshmem_register_types(void)
{
    type_register_static(&ivshmem_ntb_idt_info);
}

type_init(ivshmem_register_types)
