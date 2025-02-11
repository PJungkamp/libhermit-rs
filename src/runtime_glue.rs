//! Minor functions that Rust really expects to be defined by the compiler,
//! but which we need to provide manually because we're on bare metal.

use crate::arch::kernel::processor::run_on_hypervisor;
use crate::{__sys_shutdown, arch};
use alloc::alloc::Layout;
use core::panic::PanicInfo;

// see https://users.rust-lang.org/t/psa-breaking-change-panic-fmt-language-item-removed-in-favor-of-panic-implementation/17875
#[cfg(any(target_os = "hermit", target_os = "none"))]
#[panic_handler]
fn panic(info: &PanicInfo<'_>) -> ! {
	print!("[{}][!!!PANIC!!!] ", arch::percore::core_id());

	if let Some(location) = info.location() {
		print!("{}:{}: ", location.file(), location.line());
	}

	if let Some(message) = info.message() {
		print!("{}", message);
	}

	println!();

	if run_on_hypervisor() {
		__sys_shutdown(1);
	}

	loop {
		arch::processor::halt();
	}
}

#[alloc_error_handler]
fn rust_oom(layout: Layout) -> ! {
	println!(
		"[{}][!!!OOM!!!] Memory allocation of {} bytes failed",
		arch::percore::core_id(),
		layout.size()
	);

	loop {
		arch::processor::halt();
	}
}

#[no_mangle]
pub unsafe extern "C" fn __rg_oom(size: usize, align: usize) -> ! {
	let layout = Layout::from_size_align_unchecked(size, align);
	rust_oom(layout)
}
