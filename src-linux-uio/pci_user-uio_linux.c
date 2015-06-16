/*-
 * Copyright (c) 2014 Antti Kantee.  All Rights Reserved.
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

#include "pci_user.h"

#include <rump/rumpuser_component.h>

/* highest dev for which we've returned something sensible in config space */
static pthread_mutex_t genericmtx = PTHREAD_MUTEX_INITIALIZER;
static int highestdev = -1;

int
rumpcomp_pci_iospace_init(void)
{

	if (iopl(3) == -1)
		return rumpuser_component_errtrans(errno);

	return 0;
}

void *
rumpcomp_pci_map(unsigned long addr, unsigned long len)
{
	char path[128];
	unsigned long resa;
	FILE *res;
	void *mem;
	int uioidx, residx;
	int myhighestdev;
	int fd;

	pthread_mutex_lock(&genericmtx);
	myhighestdev = highestdev;
	pthread_mutex_unlock(&genericmtx);

	/*
	 * We search /sys/class/uio/uio<n>/device/resource for the
	 * address.  Since this routine is run only during init,
	 * don't bother with caching the results.
	 */
	for (uioidx = 0; uioidx < myhighestdev+1; uioidx++) {
		snprintf(path, sizeof(path),
		    "/sys/class/uio/uio%d/device/resource", uioidx);
		if ((res = fopen(path, "r")) == NULL)
			continue;

		for (residx = 0;
		    fscanf(res, "%lx %*x %*x\n", &resa) > 0;
		    residx++) {
			if (resa == addr)
				goto found;
		}
	}
	return NULL;

 found:
	snprintf(path, sizeof(path),
	    "/sys/class/uio/uio%d/device/resource%d", uioidx, residx);
	fd = open(path, O_RDWR);
	if (fd == -1)
		return NULL;

	mem = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FILE, fd, 0);
	close(fd);
	if (mem == MAP_FAILED)
		return NULL;

	return mem;
}

static int
openconf(unsigned dev, int mode)
{
	char path[128];
	int fd;

	if (snprintf(path, sizeof(path),
	    "/sys/class/uio/uio%d/device/config",dev) >=(ssize_t)sizeof(path)) {
		warn("impossibly long path?");
		return -1;
	}
	if ((fd = open(path, mode)) == -1) {
#if 0
		/* verbose warning */
		warn("open config space for device %d", dev);
#endif
		return -1;
	}
	return fd;
}

int
rumpcomp_pci_confread(unsigned bus, unsigned dev, unsigned fun,
	int reg, unsigned int *rv)
{
	int fd;

	*rv = 0xffffffff;
	if (fun != 0 || bus != 0)
		return 1;

	if ((fd = openconf(dev, O_RDONLY)) == -1)
		return 1;
	if (pread(fd, rv, sizeof(*rv), reg) != 4)
		warn("pci dev %u config space read", dev);
	close(fd);

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
	int fd;

	assert(bus == 0 && fun == 0);

	if ((fd = openconf(dev, O_WRONLY)) == -1)
		return 1;
	if (pwrite(fd, &v, 4, reg) != 4)
		warn("pci dev %u config space write", dev);
	close(fd);

	return 0;
}

/* this is a multifunction data structure! */
struct irq {
	unsigned magic_cookie;
	unsigned device;

	int (*handler)(void *);
	void *data;
	int fd;

	LIST_ENTRY(irq) entries;
};
static LIST_HEAD(, irq) irqs = LIST_HEAD_INITIALIZER(&irqs);

static void *
intrthread(void *arg)
{
	struct irq *irq = arg;
	const unsigned device = irq->device;
	int val;

	rumpuser_component_kthread();
	for (;;) {
		rumpcomp_pci_confread(0, device, 0, 0x04, &val);
		if (val & 0x400) {
			//printf("interrupt disabled!\n");
			val &= ~0x400;
			rumpcomp_pci_confwrite(0, device, 0, 0x04, val);
		}
		if (read(irq->fd, &val, sizeof(val)) > 0) {
			//printf("INTERRUPT!\n");
			rumpuser_component_schedule(NULL);
			irq->handler(irq->data);
			rumpuser_component_unschedule();
		} else {
			printf("NOT AN INTERRUPT!\n");
		}
	}
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

	pthread_mutex_lock(&genericmtx);
	LIST_INSERT_HEAD(&irqs, irq, entries);
	pthread_mutex_unlock(&genericmtx);

	return 0;
}

void *
rumpcomp_pci_irq_establish(unsigned cookie, int (*handler)(void *), void *data)
{
	struct irq *irq;
	char path[32];
	pthread_t pt;
	int fd;

	pthread_mutex_lock(&genericmtx);
	LIST_FOREACH(irq, &irqs, entries) {
		if (irq->magic_cookie == cookie)
			break;
	}
	pthread_mutex_unlock(&genericmtx);
	if (!irq)
		return NULL;

	snprintf(path, sizeof(path), "/dev/uio%d", irq->device);
	fd = open(path, O_RDWR);
	if (fd == -1) {
		warn("open %s for intr", path);
		return NULL;
	}

	irq->handler = handler;
	irq->data = data;
	irq->fd = fd;

	if (pthread_create(&pt, NULL, intrthread, irq) != 0) {
		warn("interrupt thread create");
		free(irq);
		close(fd);
		return NULL;
	}

	return irq;
}

/*
 * Allocate physically contiguous memory.  We could be slightly more
 * efficient here and implement an allocator on top of the
 * hugepages to ensure that they get used more efficiently.  TODO4u
 */
int
rumpcomp_pci_dmalloc(size_t size, size_t align,
	unsigned long *pap, unsigned long *vap)
{
	const size_t pagesize = getpagesize();
	void *v;
	int mmapflags, sverr;

	mmapflags = MAP_ANON|MAP_PRIVATE;
	if (size > pagesize || align > pagesize) {
		mmapflags |= MAP_HUGETLB;
	}

	v = mmap(NULL, size, PROT_READ|PROT_WRITE, mmapflags, 0, 0);
	if (v == MAP_FAILED)
		return errno;
	if (mlockall(MCL_CURRENT|MCL_FUTURE) != 0) {
		sverr = errno;
		munmap(v, size);
		return sverr;
	}

	*vap = (uintptr_t)v;
	*pap = rumpcomp_pci_virt_to_mach(v);
	assert(*pap);

	return 0;
}

void
rumpcomp_pci_dmafree(unsigned long vap, size_t size)
{
	void *v = (void *) vap;
	munmap(v, size);
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
 * Finds the physical address for the given virtual address from
 * /proc/self/pagemap.
 */
unsigned long
rumpcomp_pci_virt_to_mach(void *virt)
{
	uint64_t voff, pte;
	unsigned long paddr = 0;
	int pagesize, offset;
	int fd;

	(void)*(volatile int *)virt;
	pagesize = getpagesize();
	assert((pagesize & (pagesize-1)) == 0);

	offset = (uintptr_t)virt & (pagesize-1);
	voff = sizeof(pte) * ((uint64_t)((uintptr_t)virt) / pagesize);

	fd = open("/proc/self/pagemap", O_RDONLY);
	if (fd == -1)
		return 0;
	if (pread(fd, &pte, sizeof(pte), voff) != sizeof(pte)) {
		warn("pread");
		return 0;
	}

	/* paddr is lowest 55 bits, plus offset */
	paddr = (pte & ((1ULL<<55)-1)) * pagesize + offset;
	close(fd);

	return paddr;
}
