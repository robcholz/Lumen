#![no_std]
extern crate alloc;

use crate::ffi::{
    EncoderEvent, Task, VisionUiAction, buzzer, current_sensor, display, efuse, encoder, motion,
    system, usb,
};
use core::time::Duration;

mod ffi;

fn main() {
    usb::init();
    buzzer::init();
    current_sensor::init();
    efuse::init();
    motion::init();
    let mut encoder_queue = encoder::init(Duration::from_secs(1));
    display::init(move || match encoder_queue.receive() {
        Some(EncoderEvent::CW) => VisionUiAction::UiActionGoNext,
        Some(EncoderEvent::CCW) => VisionUiAction::UiActionGoPrev,
        Some(EncoderEvent::Click) => VisionUiAction::UiActionEnter,
        Some(EncoderEvent::Press) => VisionUiAction::UiActionExit,
        None => VisionUiAction::UiActionNone,
    });
    let _ui_task = Task::spawn("ui_task", 9, 8192, move || {
        loop {
            display::frame_render();
            system::delay(Duration::from_millis(10));
        }
    });

    loop {
        system::delay(Duration::from_millis(100));
        //motion::debug_dump();
        //current_sensor::debug_dump();
        //system::delay(Duration::from_millis(100));
    }
}
