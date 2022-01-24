use crate::arch::irq;

#[cfg(any(target_os = "hermit", target_os = "none"))]
#[no_mangle]
pub extern "C" fn sys_irq_enable() {
    irq::enable();
}

#[cfg(any(target_os = "hermit", target_os = "none"))]
#[no_mangle]
pub extern "C" fn sys_irq_disable() -> bool {
    irq::nested_disable()
}
