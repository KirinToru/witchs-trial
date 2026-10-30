use winit::keyboard::{KeyCode, PhysicalKey};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Key {
    Left,
    Right,
    Up,
    Down,
    Jump,
    Dash,
    Interact,
    Reset,
    Escape,
    ToggleDebug,
    ToggleFullscreen,
    Confirm,
}

pub struct InputState {
    pressed_mask: u16,
    just_pressed_mask: u16,
    pub mouse_pos: glam::Vec2,
    pub mouse_pressed: bool,
    pub mouse_just_pressed: bool,
}

impl InputState {
    //------------[Create empty input state]-------------------
    pub fn new() -> Self {
        Self {
            pressed_mask: 0,
            just_pressed_mask: 0,
            mouse_pos: glam::Vec2::ZERO,
            mouse_pressed: false,
            mouse_just_pressed: false,
        }
    }

    //------------[Clear single-frame edge triggers]-------------------
    pub fn clear_frame_edges(&mut self) {
        self.just_pressed_mask = 0;
        self.mouse_just_pressed = false;
    }

    //------------[Update cursor position]-------------------
    pub fn on_cursor_moved(&mut self, pos: glam::Vec2) {
        self.mouse_pos = pos;
    }

    //------------[Handle mouse button input]-------------------
    pub fn on_mouse_input(&mut self, button: winit::event::MouseButton, state: winit::event::ElementState) {
        if button == winit::event::MouseButton::Left {
            match state {
                winit::event::ElementState::Pressed => {
                    if !self.mouse_pressed {
                        self.mouse_just_pressed = true;
                        self.mouse_pressed = true;
                    }
                }
                winit::event::ElementState::Released => {
                    self.mouse_pressed = false;
                }
            }
        }
    }

    //------------[Process physical key input]-------------------
    pub fn on_key_event(&mut self, key_code: PhysicalKey, state: winit::event::ElementState) {
        let mapped_keys: &[Key] = match key_code {
            PhysicalKey::Code(KeyCode::KeyA) | PhysicalKey::Code(KeyCode::ArrowLeft) => &[Key::Left],
            PhysicalKey::Code(KeyCode::KeyD) | PhysicalKey::Code(KeyCode::ArrowRight) => &[Key::Right],
            PhysicalKey::Code(KeyCode::KeyW) => &[Key::Up, Key::Interact],
            PhysicalKey::Code(KeyCode::ArrowUp) => &[Key::Up],
            PhysicalKey::Code(KeyCode::KeyS) | PhysicalKey::Code(KeyCode::ArrowDown) => &[Key::Down],
            PhysicalKey::Code(KeyCode::Space) => &[Key::Jump, Key::Confirm],
            PhysicalKey::Code(KeyCode::Enter) | PhysicalKey::Code(KeyCode::NumpadEnter) => &[Key::Confirm],
            PhysicalKey::Code(KeyCode::ShiftLeft) | PhysicalKey::Code(KeyCode::ShiftRight) => &[Key::Dash],
            PhysicalKey::Code(KeyCode::KeyR) => &[Key::Reset],
            PhysicalKey::Code(KeyCode::Escape) => &[Key::Escape],
            PhysicalKey::Code(KeyCode::F3) => &[Key::ToggleDebug],
            PhysicalKey::Code(KeyCode::F4) => &[Key::ToggleFullscreen],
            _ => &[],
        };

        for &key in mapped_keys {
            let mask = 1u16 << (key as u16);
            match state {
                winit::event::ElementState::Pressed => {
                    if (self.pressed_mask & mask) == 0 {
                        self.just_pressed_mask |= mask;
                        self.pressed_mask |= mask;
                    }
                }
                winit::event::ElementState::Released => {
                    self.pressed_mask &= !mask;
                }
            }
        }
    }

    //------------[Check if key is held]-------------------
    pub fn is_pressed(&self, key: Key) -> bool {
        (self.pressed_mask & (1u16 << (key as u16))) != 0
    }

    //------------[Check if key was just pressed]-------------------
    pub fn is_just_pressed(&self, key: Key) -> bool {
        (self.just_pressed_mask & (1u16 << (key as u16))) != 0
    }
}
