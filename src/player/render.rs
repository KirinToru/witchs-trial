use glam::Vec2;
use crate::engine::{Texture, WgpuRenderer};
use super::{Player, PlayerState};

impl Player {
    //------------[Reload player textures]-------------------
    #[allow(dead_code)]
    pub fn reload_textures(&mut self, renderer: &WgpuRenderer) -> Result<(), String> {
        self.spritesheet = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/player/spritesheet.png",
            &renderer.texture_bind_group_layout,
        )?;
        Ok(())
    }

    //------------[Render player sprite]-------------------
    pub fn render(&self, renderer: &mut WgpuRenderer) {
        let (row, frame_count, _) = self.state.anim_params();

        let current_col = (self.anim_frame % frame_count) as f32;
        let total_cols = 4.0;
        let total_rows = 6.0;

        let mut u0 = current_col / total_cols;
        let mut u1 = (current_col + 1.0) / total_cols;
        let v0 = row as f32 / total_rows;
        let v1 = (row as f32 + 1.0) / total_rows;

        // Facing direction flip
        let render_facing = if self.state == PlayerState::WallSlide {
            if self.walled_left {
                -1.0
            } else {
                1.0
            }
        } else {
            self.facing
        };

        if render_facing < 0.0 {
            std::mem::swap(&mut u0, &mut u1);
        }

        // Center 36x36 sprite over 18x26 collider
        let render_pos = self.pos + Vec2::new(-9.0, -10.0);
        let render_size = Vec2::new(36.0, 36.0);

        // State tint
        let tint = match self.state {
            PlayerState::DashFreeze => [1.9, 1.9, 1.9, 1.0], // Impact flash
            PlayerState::Dash => [0.7, 1.3, 1.9, 0.95],      // Motion tint
            PlayerState::WallSlide => [1.0, 0.95, 0.9, 1.0],
            _ => [1.0, 1.0, 1.0, 1.0],
        };

        renderer.draw_texture_rect(
            render_pos,
            render_size,
            Vec2::new(u0, v0),
            Vec2::new(u1, v1),
            self.spritesheet.id,
            &self.spritesheet.bind_group,
            tint,
        );
    }
}

