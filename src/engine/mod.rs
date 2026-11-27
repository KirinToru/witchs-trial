//! Engine subsystem providing hardware rendering, input handling, and timing primitives.

pub mod audio;
pub mod debug_font;
pub mod input;
pub mod renderer;
pub mod texture;
pub mod time;

pub use audio::{AudioSystem, MusicTrack, SfxEvent};
pub use input::{InputState, Key};
pub use renderer::{GraphicsBackendOption, WgpuRenderer};
pub use texture::Texture;
pub use time::{FrameLimiter, TimeAccumulator};

#[cfg(test)]
mod tests;

