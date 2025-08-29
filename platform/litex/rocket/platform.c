/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Dolu1990 <charles.papon.90@gmail.com>
 *
 */

#include <sbi/riscv_asm.h> 
#include <sbi/riscv_encoding.h> 
#include <sbi/riscv_io.h> 
#include <sbi/sbi_const.h> 
#include <sbi/sbi_hart.h> 
#include <sbi/sbi_platform.h> 
#include <sbi_utils/fdt/fdt_helper.h> 
#include <sbi_utils/fdt/fdt_fixup.h> 
#include <sbi_utils/ipi/aclint_mswi.h> 
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/litex-uart.h> 
#include <sbi_utils/timer/aclint_mtimer.h>

#define ROCKET_DEFAULT_HART_COUNT	1
#define ROCKET_DEFAULT_PLATFORM_FEATURES	SBI_PLATFORM_HAS_MFAULTS_DELEGATION
#define ROCKET_DEFAULT_UART_ADDR	0x12003000
#define ROCKET_DEFAULT_PLIC_ADDR	0xc000000
#define ROCKET_DEFAULT_PLIC_NUM_SOURCES	1
#define ROCKET_DEFAULT_CLINT_ADDR	0x2000000
#define ROCKET_DEFAULT_ACLINT_MTIMER_FREQ	100000000
#define ROCKET_DEFAULT_ACLINT_MSWI_ADDR	\
		(ROCKET_DEFAULT_CLINT_ADDR + CLINT_MSWI_OFFSET)
#define ROCKET_DEFAULT_ACLINT_MTIMER_ADDR	\
		(ROCKET_DEFAULT_CLINT_ADDR + CLINT_MTIMER_OFFSET)
#define ROCKET_DEFAULT_HART_STACK_SIZE	8192

/* clang-format on */

static struct plic_data plic = {
	.addr = ROCKET_DEFAULT_PLIC_ADDR,
	.num_src = ROCKET_DEFAULT_PLIC_NUM_SOURCES,
};

static struct aclint_mswi_data mswi = {
	.addr = ROCKET_DEFAULT_ACLINT_MSWI_ADDR,
	.size = ACLINT_MSWI_SIZE,
	.first_hartid = 0,
	.hart_count = ROCKET_DEFAULT_HART_COUNT,
};

static struct aclint_mtimer_data mtimer = {
	.mtime_freq = ROCKET_DEFAULT_ACLINT_MTIMER_FREQ,
	.mtime_addr = ROCKET_DEFAULT_ACLINT_MTIMER_ADDR +
			  ACLINT_DEFAULT_MTIME_OFFSET,
	.mtime_size = ACLINT_DEFAULT_MTIME_SIZE,
	.mtimecmp_addr = ROCKET_DEFAULT_ACLINT_MTIMER_ADDR +
			  ACLINT_DEFAULT_MTIMECMP_OFFSET,
	.mtimecmp_size = ACLINT_DEFAULT_MTIMECMP_SIZE,
	.first_hartid = 0,
	.hart_count = ROCKET_DEFAULT_HART_COUNT,
	.has_64bit_mmio = true,
};

/*
 * ROCKETRiscv platform early initialization.
 */
static int ROCKET_early_init(bool cold_boot)
{
	return 0;
}

/*
 * ROCKETRiscv platform final initialization.
 */
static int ROCKET_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = fdt_get_address();
	fdt_fixups(fdt);

	return 0;
}

/*
 * Initialize the ROCKETRiscv console.
 */
static int ROCKET_console_init(void)
{
	return litex_uart_init(ROCKET_DEFAULT_UART_ADDR);
}

/*
 * Initialize the ROCKETRiscv interrupt controller for current HART.
 */
static int ROCKET_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(&plic);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(&plic, hartid * 2, hartid * 2 + 1);

}

/*
 * Initialize IPI for current HART.
 */
static int ROCKET_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = aclint_mswi_cold_init(&mswi);
		if (rc)
			return rc;
	}

	return aclint_mswi_warm_init();
}

/*
 * Initialize ROCKETRiscv timer for current HART.
 */
static int ROCKET_timer_init(bool cold_boot)
{
	int rc;
	if (cold_boot) {
		rc = aclint_mtimer_cold_init(&mtimer, NULL); /* Timer has no reference */
		if (rc)
			return rc;
	}

	return aclint_mtimer_warm_init();
}

/*
 * Platform descriptor.
 */
const struct sbi_platform_operations platform_ops = {
	.early_init = ROCKET_early_init,
	.final_init = ROCKET_final_init,
	.console_init = ROCKET_console_init,
	.irqchip_init = ROCKET_irqchip_init,
	.ipi_init = ROCKET_ipi_init,
	.timer_init = ROCKET_timer_init
};

const struct sbi_platform platform = {
	.opensbi_version = OPENSBI_VERSION,
	.platform_version = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name = "LiteX / ROCKET",
	.features = ROCKET_DEFAULT_PLATFORM_FEATURES,
	.hart_count = ROCKET_DEFAULT_HART_COUNT,
	.hart_stack_size = ROCKET_DEFAULT_HART_STACK_SIZE,
	.heap_size =
		SBI_PLATFORM_DEFAULT_HEAP_SIZE(ROCKET_DEFAULT_HART_COUNT),
	.platform_ops_addr = (unsigned long)&platform_ops
};