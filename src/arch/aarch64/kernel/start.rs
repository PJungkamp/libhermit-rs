use crate::arch::aarch64::kernel::serial::SerialPort;
use crate::arch::aarch64::kernel::{scheduler::TaskStacks, BootInfo};
use crate::KERNEL_STACK_SIZE;

static mut BOOT_STACK: [u8; KERNEL_STACK_SIZE] = [0; KERNEL_STACK_SIZE];

/// Entrypoint - Initalize Stack pointer and Exception Table
#[inline(never)]
#[no_mangle]
#[naked]
pub unsafe extern "C" fn _start() -> ! {
	asm!("ldr x1, {0}",
		 "add x1, x1, {1}",
		 "sub x1, x1, {2}",   /* Previous version subtracted 0x10 from End, so I'm doing this too. Not sure why though. COMMENT from SL: This is a habit of mine. I always start 0x10 bytes before the end of the stack. */
		 "mov sp, x1",
		 /* Set exception table */
		 "adr x8, vector_table",
		 "msr vbar_el1, x8",
		 "b pre_init",
		sym BOOT_STACK,
		const KERNEL_STACK_SIZE,
		const TaskStacks::MARKER_SIZE,
		options(noreturn),
	)
}

#[inline(never)]
#[no_mangle]
unsafe fn pre_init(boot_info: &'static mut BootInfo) -> ! {
	println!("Welcome to hermit kernel.");
	if boot_info.cpu_online == 0 {
		crate::boot_processor_main()
	} else {
		#[cfg(not(feature = "smp"))]
		{
			error!("SMP support deactivated");
			loop {
				//processor::halt();
			}
		}
		#[cfg(feature = "smp")]
		crate::application_processor_main();
	}
}
