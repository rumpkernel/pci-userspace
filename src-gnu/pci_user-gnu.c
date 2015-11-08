/*-
 * Copyright (c) 2014 Antti Kantee.  All Rights Reserved.
 * Copyright (c) 2015 Robert Millan
 * Copyright (c) 2009,2010 Zheng Da
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * intrthread() is based on code from intloop() function, obtained
 * from the Debian version of the Hurd (in libddekit/interrupt.c),
 * which is written by Zheng Da and is licensed under the same terms
 * as the rest of this file. Public statement available at:
 * 
 *  https://www.freelists.org/post/rumpkernel-users/licensing-of-intloop-in-libddekitinterruptc,1
 *  https://lists.gnu.org/archive/html/bug-hurd/2015-11/msg00021.html
 *  https://lists.debian.org/debian-hurd/2015/11/msg00021.html
 */

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/io.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <rump/rumpuser_component.h>

#include <hurd.h>
#include <device/device.h>

#include <pciaccess.h>

#include "pci_user.h"
#include "experimental_U.h"
#include <device/intr.h>

#define RUMP_IRQ_PRIO		2

/* highest dev for which we've returned something sensible in config space */
static pthread_mutex_t genericmtx = PTHREAD_MUTEX_INITIALIZER;
static int highestdev = -1;

static mach_port_t master_host;
static mach_port_t master_device;

int
rumpcomp_pci_iospace_init(void)
{
	if (ioperm(0, 0x10000, 1))
		return rumpuser_component_errtrans(errno);

	return 0;
}

#define NUMDEVS	32
static struct pci_device *pci_devices[NUMDEVS];

static void
pci_userspace_init(void)
{
	/* FIXME: add a hook to make rump call this, once and only once */
	static int is_init = 0;
	if (is_init)
		return;
	is_init = 1;

	if (get_privileged_ports (&master_host, &master_device))
		err(1, "get_privileged_ports");

	pci_system_init ();
	struct pci_device_iterator *dev_iter;
	struct pci_device *pci_dev;
        dev_iter = pci_slot_match_iterator_create (NULL);
	int i = 0;
        while ((pci_dev = pci_device_next (dev_iter)) != NULL) {
		pci_devices[i++] = pci_dev;
	}
}

void *
rumpcomp_pci_map(unsigned long addr, unsigned long len)
{
	errno = rumpuser_component_errtrans(ENOSYS);
	return NULL;
}

int
rumpcomp_pci_confread(unsigned bus, unsigned dev, unsigned fun,
	int reg, unsigned int *rv)
{
	*rv = 0xffffffff;
	if (fun != 0 || bus != 0)
		return 1;

	if (dev >= NUMDEVS)
		return 1;

	pci_userspace_init();

	pci_device_cfg_read_u32(pci_devices[dev], rv, reg);

	pthread_mutex_lock(&genericmtx);
	if ((int)dev > highestdev)
		highestdev = dev;
	pthread_mutex_unlock(&genericmtx);

	return 0;
}

int
rumpcomp_pci_confwrite(unsigned bus, unsigned dev, unsigned fun,
	int reg, unsigned int v)
{
	assert(bus == 0 && fun == 0);

	if (dev >= NUMDEVS)
		return 1;

	pci_userspace_init();

	pci_device_cfg_write_u32(pci_devices[dev], v, reg);

	return 0;
}

/* this is a multifunction data structure! */
struct irq {
	unsigned magic_cookie;
	unsigned device;

	int (*handler)(void *);
	void *data;
	int intrline;

	LIST_ENTRY(irq) entries;
};
static LIST_HEAD(, irq) irqs = LIST_HEAD_INITIALIZER(&irqs);

static void *
intrthread(void *arg)
{
	struct irq *irq = arg;
	mach_port_t delivery_port;
        mach_port_t pset, psetcntl;
	int ret;
	int val;

	rumpuser_component_kthread();

	ret = mach_port_allocate (mach_task_self (), MACH_PORT_RIGHT_RECEIVE,
				&delivery_port);
	if (ret)
		err(ret, "mach_port_allocate");

	ret = thread_get_assignment (mach_thread_self (), &pset);
	if (ret)
		err(ret, "thread_get_assignment");

	ret = host_processor_set_priv (master_host, pset, &psetcntl);
	if (ret)
		err(ret, "host_processor_set_priv");

	thread_max_priority (mach_thread_self (), psetcntl, 0);
	ret = thread_priority (mach_thread_self (), RUMP_IRQ_PRIO, 0);
	if (ret)
		err(ret, "thread_priority");

	ret = device_intr_register(master_device, irq->intrline,
					0, 0x04000000, delivery_port,
					MACH_MSG_TYPE_MAKE_SEND);
	if (ret) {
		warn("device_intr_register");
		return 0;
	}

	device_intr_enable (master_device, irq->intrline, TRUE);

        int irq_server (mach_msg_header_t *inp, mach_msg_header_t *outp) {
                mach_intr_notification_t *intr_header = (mach_intr_notification_t *) inp;

                ((mig_reply_header_t *) outp)->RetCode = MIG_NO_REPLY;
                if (inp->msgh_id != MACH_INTR_NOTIFY)
                        return 0;

                /* It's an interrupt not for us. It shouldn't happen. */
                if (intr_header->line != irq->intrline) {
                        printf ("We get interrupt %d, %d is expected",
                                       intr_header->line, irq->intrline);
                        return 1;
                }

		rumpcomp_pci_confread(0, irq->device, 0, 0x04, &val);
		if (val & 0x400) {
			printf("interrupt disabled!\n");
			val &= ~0x400;
			rumpcomp_pci_confwrite(0, irq->device, 0, 0x04, val);
		}

		rumpuser_component_schedule(NULL);
		irq->handler(irq->data);
		rumpuser_component_unschedule();

                /* If the irq has been disabled by the linux device,
                 * we don't need to reenable the real one. */
                device_intr_enable (master_device, irq->intrline, TRUE);

                return 1;
        }

        mach_msg_server (irq_server, 0, delivery_port);

	return NULL;
}

int
rumpcomp_pci_irq_map(unsigned bus, unsigned device, unsigned fun,
	int intrline, unsigned cookie)
{
	struct irq *irq;

	irq = malloc(sizeof(*irq));
	if (irq == NULL)
		return ENOENT;

	irq->magic_cookie = cookie;
	irq->device = device;
	irq->intrline = intrline;

	pthread_mutex_lock(&genericmtx);
	LIST_INSERT_HEAD(&irqs, irq, entries);
	pthread_mutex_unlock(&genericmtx);

	return 0;
}

void *
rumpcomp_pci_irq_establish(unsigned cookie, int (*handler)(void *), void *data)
{
	struct irq *irq;
	pthread_t pt;

	pthread_mutex_lock(&genericmtx);
	LIST_FOREACH(irq, &irqs, entries) {
		if (irq->magic_cookie == cookie)
			break;
	}
	pthread_mutex_unlock(&genericmtx);
	if (!irq)
		return NULL;

	irq->handler = handler;
	irq->data = data;

	if (pthread_create(&pt, NULL, intrthread, irq) != 0) {
		warn("interrupt thread create");
		free(irq);
		return NULL;
	}

	return irq;
}

struct virt_to_mach {
	unsigned long pa;
	unsigned long va;

        LIST_ENTRY(virt_to_mach) entries;
};
static LIST_HEAD(, virt_to_mach) virt_to_mach_list = LIST_HEAD_INITIALIZER(&virt_to_mach_list);

/*
 * Allocate physically contiguous memory.  We could be slightly more
 * efficient here and implement an allocator on top of the
 * hugepages to ensure that they get used more efficiently.  TODO4u
 */
int
rumpcomp_pci_dmalloc(size_t size, size_t align,
	unsigned long *pap, unsigned long *vap)
{
	struct virt_to_mach *virt_to_mach;
	const size_t pagesize = getpagesize();

	if (align > pagesize) {
		warnx("requested alignment (%x) is larger than page size (%x)", align, pagesize);
		return 1;
	}

	pci_userspace_init();

	if (vm_allocate_contiguous (master_host, mach_task_self(), vap, pap, size)) {
		warn("vm_allocate_contiguous");
		return 1;
	}

	assert(*pap);

	virt_to_mach = malloc(sizeof(*virt_to_mach));
	if (virt_to_mach == NULL)
		return errno;

	virt_to_mach->pa = *pap;
	virt_to_mach->va = *vap;

        LIST_INSERT_HEAD(&virt_to_mach_list, virt_to_mach, entries);

	return 0;
}

void
rumpcomp_pci_dmafree(unsigned long vap, size_t size)
{
	void *v = (void *) vap;
	struct virt_to_mach *virt_to_mach;

	munmap(v, size);

	LIST_FOREACH(virt_to_mach, &virt_to_mach_list, entries) {
		if (virt_to_mach->va == (uintptr_t)vap) {
			LIST_REMOVE(virt_to_mach, entries);
			break;
		}
	}
}

/*
 * "maps" dma memory into virtual address space.  For now, we just
 * rely on it already being mapped.  This means that support for
 * >1 segs is not supported.  We could call mremap() ...
 */
int
rumpcomp_pci_dmamem_map(struct rumpcomp_pci_dmaseg *dss, size_t nseg,
	size_t totlen, void **vap)
{
	if (nseg > 1) {
		printf("dmamem_map for >1 seg currently not supported");
		return ENOTSUP;
	}

	*vap = (void *)dss[0].ds_vacookie;
	return 0;
}

/*
 * Finds the physical address for the given virtual address.
 */
unsigned long
rumpcomp_pci_virt_to_mach(void *virt)
{
	struct virt_to_mach *virt_to_mach;

	/* FIXME: find a Mach API to do this properly, and drop the bookkeeping */

	LIST_FOREACH(virt_to_mach, &virt_to_mach_list, entries) {
		if (virt_to_mach->va == (uintptr_t)virt)
			return virt_to_mach->pa;
	}
	return 0;
}
