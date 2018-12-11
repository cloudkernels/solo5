/* 
 * Copyright (c) 2015-2017 Contributors as noted in the AUTHORS file
 *
 * This file is part of ukvm, a unikernel monitor.
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted, provided
 * that the above copyright notice and this permission notice appear
 * in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * ukvm_hv_linux.c: x86_64 architecture-dependent part of Linux backend
 * implementation.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <poll.h>

#include "ukvm_cpu_x86_64.h"

#include "ukvm.h"
#include "ukvm_hv_linux.h"

void ukvm_hv_mem_size(size_t *mem_size) {
    ukvm_x86_mem_size(mem_size);
}

/* Yikes. Using rdtsc for timing is a bit suspect. */
uint64_t get_cpuinfo_freq(void)
{
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    int ghz=0, mhz=0;
    char buf[256];
    
    do {
        char *ptr;
        ptr = fgets(buf, 256, cpuinfo);
        if (!ptr)
            continue;
                
        ptr = strstr(buf, "GHz");
        if (!ptr)
            continue;

        sscanf(ptr - 4, "%d.%d", &ghz, &mhz);
        break;
    } while(!feof(cpuinfo));

    fclose(cpuinfo);

    return ((uint64_t)ghz * 1000000000 + (uint64_t)mhz * 10000000);
}
