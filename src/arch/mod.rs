// Platform-specific implementations
#[cfg(target_arch = "aarch64")]
pub mod aarch64;

#[cfg(target_arch = "x86_64")]
pub mod x86_64;

// Export our platform-specific modules.
#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::*;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::stubs::{set_oneshot_timer, wakeup_core};

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::{
	application_processor_init, boot_application_processors, boot_processor_init,
	get_processor_count, message_output_init, output_message_buf, output_message_byte,
};

#[cfg(target_arch = "aarch64")]
use crate::arch::aarch64::kernel::percore::core_scheduler;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::percore;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::scheduler;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::processor;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::irq;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::systemtime::get_boot_time;

#[cfg(target_arch = "aarch64")]
pub use crate::arch::aarch64::kernel::switch;

#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::*;

#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::apic::{set_oneshot_timer, wakeup_core};
#[cfg(all(target_arch = "x86_64", any(target_os = "hermit", target_os = "none"), feature = "smp"))]
pub use crate::arch::x86_64::kernel::application_processor_init;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::gdt::set_current_kernel_stack;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::irq;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::percore;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::processor;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::scheduler;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::switch;
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::systemtime::get_boot_time;
#[cfg(all(target_arch = "x86_64", any(target_os = "hermit", target_os = "none")))]
pub use crate::arch::x86_64::kernel::{boot_application_processors, boot_processor_init};
#[cfg(target_arch = "x86_64")]
pub use crate::arch::x86_64::kernel::{
	get_processor_count, message_output_init, output_message_buf, output_message_byte,
};
