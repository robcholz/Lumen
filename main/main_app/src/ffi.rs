use crate::main;
use alloc::boxed::Box;
use alloc::ffi::CString;
use core::alloc::{GlobalAlloc, Layout};
use core::ffi::{c_char, c_int, c_uint, c_void};
use core::fmt;
use core::fmt::Write;
use core::panic::PanicInfo;
use core::ptr::{addr_of_mut, null_mut};
use critical_section::{Impl, RawRestoreState};

#[repr(C)]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum VisionUiAction {
    UiActionNone = 0,
    UiActionGoPrev = 1,
    UiActionGoNext = 2,
    UiActionEnter = 3,
    UiActionExit = 4,
}

unsafe extern "C" {
    fn main_app_log(level: i32, text: *const c_char);
    fn acquire_main_app_log_buffer_lock();
    fn release_main_app_log_buffer_lock();
    fn malloc(size: usize) -> *mut c_void;
    fn free(ptr: *mut c_void);
    fn vPortEnterCritical();
    fn vPortExitCritical();
    fn xQueueReceive(queue: *mut c_void, item: *mut c_void, ticks: u32) -> c_int;
    fn xTaskCreatePinnedToCore(
        task: *mut c_void,
        name: *const u8,
        stack_depth: u32,
        parameter: *mut c_void,
        priority: c_uint,
        handle: *mut c_void,
        core_id: c_int,
    ) -> c_int;
    fn vTaskDelete(handle: *mut c_void);
    fn main_app_abort(details: *const c_char) -> !;
    fn current_sensor_init();
    fn current_sensor_read_debug();
    fn control_init();
    fn control_turn_on();
    fn control_turn_off();
    fn efuse_init();
    fn buzzer_init();
    fn buzzer_tone(freq_hz: u32, duration_ms: u16);
    fn encoder_init(long_press_duration: u32) -> *mut c_void;
    fn display_init(action: extern "C" fn() -> VisionUiAction);
    fn display_measure_fps();
    fn motion_init();
    fn motion_read_debug();
    fn delay(ms: u32);
}

const PANIC_BUF_LEN: usize = 512;
static mut PANIC_BUF: [u8; PANIC_BUF_LEN] = [0; PANIC_BUF_LEN];

struct StaticPanicBufWriter;

impl Write for StaticPanicBufWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        unsafe {
            let buf_ptr = addr_of_mut!(PANIC_BUF) as *mut u8;

            let mut pos = 0;
            while pos < PANIC_BUF_LEN && *buf_ptr.add(pos) != 0 {
                pos += 1;
            }

            let bytes = s.as_bytes();
            let max_write = PANIC_BUF_LEN.saturating_sub(1 + pos);
            let write_len = bytes.len().min(max_write);

            for i in 0..write_len {
                *buf_ptr.add(pos + i) = bytes[i];
            }

            *buf_ptr.add(pos + write_len) = 0;
        }
        Ok(())
    }
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    let buf_ptr;
    unsafe {
        buf_ptr = addr_of_mut!(PANIC_BUF) as *mut u8;
        *buf_ptr.add(0) = 0;
    }

    let mut w = StaticPanicBufWriter;
    let _ = write!(&mut w, "Panic: {}", info);

    unsafe { main_app_abort(buf_ptr as *const c_char) }
}

struct FreeRtosCriticalSection;

critical_section::set_impl!(FreeRtosCriticalSection);

unsafe impl Impl for FreeRtosCriticalSection {
    unsafe fn acquire() -> RawRestoreState {
        unsafe {
            vPortEnterCritical();
        }
        ()
    }

    unsafe fn release(_state: RawRestoreState) {
        unsafe {
            vPortExitCritical();
        }
    }
}

struct MyAlloc;

unsafe impl GlobalAlloc for MyAlloc {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let p;
        unsafe {
            p = malloc(layout.size());
        }
        p as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe {
            free(ptr as *mut c_void);
        }
    }
}

#[global_allocator]
static GLOBAL: MyAlloc = MyAlloc;

#[unsafe(no_mangle)]
pub extern "C" fn main_app_run() {
    main();
}

struct Stdout;

const LOG_BUF_LEN: usize = 1024;
static mut LOG_BUF: [u8; LOG_BUF_LEN] = [0; LOG_BUF_LEN];
static mut LOG_LEVEL: i32 = 1;

impl Write for Stdout {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        unsafe {
            let ptr = addr_of_mut!(LOG_BUF) as *mut u8;

            *ptr = 0;

            let bytes = s.as_bytes();
            let to_copy = bytes.len().min(LOG_BUF_LEN - 1);

            core::ptr::copy_nonoverlapping(bytes.as_ptr(), ptr, to_copy);

            *ptr.add(to_copy) = 0;

            main_app_log(LOG_LEVEL, ptr as *const c_char);
        }

        Ok(())
    }
}

#[allow(unused)]
pub fn log_impl(level: i32, args: fmt::Arguments<'_>) {
    unsafe {
        acquire_main_app_log_buffer_lock();
        LOG_LEVEL = level;
    }
    Stdout.write_fmt(args).unwrap();
    unsafe {
        release_main_app_log_buffer_lock();
    }
}

#[macro_export]
macro_rules! info {
    ($($arg:tt)*) => {{
        $crate::ffi::log_impl(1, core::format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! debug {
    ($($arg:tt)*) => {{
        $crate::ffi::log_impl(0, core::format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! error {
    ($($arg:tt)*) => {{
        $crate::ffi::log_impl(2, core::format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! warn {
    ($($arg:tt)*) => {{
        $crate::ffi::log_impl(3, core::format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! trace {
    ($($arg:tt)*) => {{
        $crate::ffi::log_impl(4, core::format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! print {
    ($fmt: literal $(, $($arg: tt)+)?) => {
        $crate::ffi::log_impl(1,format_args!($fmt $(, $($arg)+)?))
    }
}

#[macro_export]
macro_rules! println {
    ($fmt: literal $(, $($arg: tt)+)?) => {
        $crate::ffi::log_impl(1,format_args!(concat!($fmt, "\n") $(, $($arg)+)?))
    }
}

/// Current sensor–related safe API
#[allow(unused)]
pub mod current_sensor {
    use super::*;

    /// Initialize the current sensor.
    pub fn init() {
        unsafe { current_sensor_init() }
    }

    /// Print debug info about the current sensor
    pub fn debug_dump() {
        unsafe { current_sensor_read_debug() }
    }
}

/// USB Control subsystem.
#[allow(unused)]
pub mod usb {
    use super::*;

    /// Initialize the control subsystem.
    pub fn init() {
        unsafe { control_init() }
    }

    /// Turn the USB on.
    pub fn turn_on() {
        unsafe { control_turn_on() }
    }

    /// Turn the USB off.
    pub fn turn_off() {
        unsafe { control_turn_off() }
    }
}

/// EFuse Control subsystem.
#[allow(unused)]
pub mod efuse {
    use super::*;

    /// Initialize the efuse subsystem.
    pub fn init() {
        unsafe { efuse_init() }
    }
}

/// Buzzer control.
#[allow(unused)]
pub mod buzzer {
    use super::*;

    /// Initialize the buzzer hardware.
    pub fn init() {
        unsafe { buzzer_init() }
    }

    /// Play a tone at `freq_hz` for `duration_ms` milliseconds.
    pub fn tone(freq_hz: u32, duration_ms: u16) {
        unsafe { buzzer_tone(freq_hz, duration_ms) }
    }
}

#[allow(unused)]
pub struct EncoderQueue {
    handle: *mut c_void,
}

unsafe impl Send for EncoderQueue {}
unsafe impl Sync for EncoderQueue {}

impl From<*mut c_void> for EncoderQueue {
    fn from(value: *mut c_void) -> Self {
        Self { handle: value }
    }
}

#[derive(Debug)]
pub enum EncoderEvent {
    CW,
    CCW,
    Click,
    Press,
}

impl EncoderQueue {
    pub(crate) fn receive(&mut self) -> Option<EncoderEvent> {
        unsafe {
            let mut event: u8 = 0;
            if xQueueReceive(self.handle, &mut event as *mut u8 as *mut c_void, 0) == 0 {
                return None;
            }
            match event {
                0 => Some(EncoderEvent::CW),
                1 => Some(EncoderEvent::CCW),
                2 => Some(EncoderEvent::Click),
                3 => Some(EncoderEvent::Press),
                _ => None,
            }
        }
    }
}

type BoxedTask = Box<dyn FnOnce() + Send + 'static>;

extern "C" fn trampoline(p: *mut c_void) {
    unsafe {
        let boxed: Box<BoxedTask> = Box::from_raw(p as *mut BoxedTask);
        (*boxed)();
        vTaskDelete(null_mut());
    }
}

pub struct Task {
    handle: *mut c_void,
}

impl Task {
    pub(crate) fn spawn<S, F>(name: S, priority: u8, stack_depth: u32, task: F) -> Task
    where
        S: AsRef<str>,
        F: FnOnce() + Send + 'static,
    {
        let cname = CString::new(name.as_ref()).unwrap();

        let boxed: BoxedTask = Box::new(task);
        let param = Box::into_raw(Box::new(boxed)) as *mut c_void;

        let mut handle: *mut c_void = null_mut();

        let rc = unsafe {
            xTaskCreatePinnedToCore(
                trampoline as usize as *mut c_void,
                cname.as_ptr() as *const u8,
                stack_depth,
                param,
                priority as c_uint,
                (&mut handle as *mut *mut c_void) as *mut c_void,
                0x7FFFFFFF,
            )
        };

        if rc != 1 {
            unsafe { drop(Box::from_raw(param as *mut BoxedTask)) };
            handle = null_mut();
        }

        Task { handle }
    }
}

impl Drop for Task {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            unsafe { vTaskDelete(self.handle) };
            self.handle = null_mut();
        }
    }
}

/// Encoder (rotary encoder / input device).
#[allow(unused)]
pub mod encoder {
    use super::*;
    use core::time::Duration;

    #[must_use]
    pub fn init(long_press_activation: Duration) -> EncoderQueue {
        EncoderQueue::from(unsafe { encoder_init(long_press_activation.as_micros() as u32) })
    }
}

/// Display subsystem.
#[allow(unused)]
pub mod display {
    use super::*;
    use core::ptr::null_mut;

    type ActionObj = dyn FnMut() -> VisionUiAction + 'static;

    static mut ACTION_CB: *mut Box<ActionObj> = null_mut();

    extern "C" fn trampoline() -> VisionUiAction {
        unsafe {
            if ACTION_CB.is_null() {
                return VisionUiAction::UiActionNone;
            }
            let cb: &mut Box<ActionObj> = &mut *ACTION_CB;
            cb()
        }
    }

    pub fn init<F>(action: F)
    where
        F: FnMut() -> VisionUiAction + 'static,
    {
        unsafe {
            if !ACTION_CB.is_null() {
                drop(Box::from_raw(ACTION_CB));
                ACTION_CB = null_mut();
            }

            let boxed: Box<ActionObj> = Box::new(action);
            ACTION_CB = Box::into_raw(Box::new(boxed));

            display_init(trampoline);
        }
    }

    pub fn frame_render() {
        unsafe { display_measure_fps() }
    }
}

/// Motion sensor.
#[allow(unused)]

pub mod motion {
    use super::*;

    pub fn init() {
        unsafe { motion_init() }
    }

    pub fn debug_dump() {
        unsafe { motion_read_debug() }
    }
}

/// System-level helpers (delay, etc).
#[allow(unused)]
pub mod system {
    /// Convenience wrapper using `core::time::Duration`.
    pub fn delay(duration: core::time::Duration) {
        // truncate to u32 milliseconds
        let ms = duration.as_millis().min(u32::MAX as u128) as u32;
        unsafe { crate::ffi::delay(ms) }
    }
}
