/*
 * syscall reporting example for seccomp
 *
 * Copyright (c) 2012 The Chromium OS Authors <chromium-os-dev@chromium.org>
 * Authors:
 *  Will Drewry <wad@chromium.org>
 *  Kees Cook <keescook@chromium.org>
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "syscall-reporter.h"
#include "syscall-names.h"
#include "seccomp-bpf.h"
#include <err.h>

const char * const msg_needed = "Looks like you also need syscall: ";

/* Since "sprintf" is technically not signal-safe, reimplement %d here. */
static void write_uint(char *buf, unsigned int val)
{
	int width = 0;
	unsigned int tens;

	if (val == 0) {
		strcpy(buf, "0");
		return;
	}
	for (tens = val; tens; tens /= 10)
		++ width;
	buf[width] = '\0';
	for (tens = val; tens; tens /= 10)
		buf[--width] = '0' + (tens % 10);
}

static void reporter(int nr, siginfo_t *info, void *void_context)
{
	char buf[128];
	ucontext_t *ctx = (ucontext_t *)(void_context);
	unsigned int syscall;
	if (info->si_code != SYS_SECCOMP)
		return;
	if (!ctx)
		return;
	syscall = ctx->uc_mcontext.M_SYSCALL;
	strcpy(buf, msg_needed);
	if (syscall < sizeof(syscall_names)) {
		strcat(buf, syscall_names[syscall]);
		strcat(buf, "(");
	}
	write_uint(buf + strlen(buf), syscall);
	if (syscall < sizeof(syscall_names))
		strcat(buf, ")");
	strcat(buf, "\n");
	syscall = write(STDOUT_FILENO, buf, strlen(buf));
	_exit(1);
}

static void sig_handler(int signo)
{
	exit(1);
}

int install_syscall_reporter(void)
{
	struct sigaction act;
	sigset_t mask;
	memset(&act, 0, sizeof(act));
	sigemptyset(&mask);
	sigaddset(&mask, SIGSYS);

	act.sa_sigaction = &reporter;
	act.sa_flags = SA_SIGINFO;
	if (sigaction(SIGSYS, &act, NULL) < 0) {
		perror("sigaction");
		return -1;
	}
	if (sigprocmask(SIG_UNBLOCK, &mask, NULL)) {
		perror("sigprocmask");
		return -1;
	}

	/*
	 * Our current sig handler in ukvm_main.c is doing some bad system
	 * call; let's replace it with this simpler sig_handler that only
	 * exits(1).
	 */
	struct sigaction sa;
	memset (&sa, 0, sizeof (struct sigaction));
	sa.sa_handler = sig_handler;
	sigfillset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1)
		err(1, "Could not install signal handler");
	if (sigaction(SIGTERM, &sa, NULL) == -1)
		err(1, "Could not install signal handler");

	return 0;
}
