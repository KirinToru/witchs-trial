use glam::Vec2;
use crate::engine::WgpuRenderer;
use super::{Player, PlayerState};

impl Player {
    //------------[Render player sprite with sub-frame interpolation]-------------------
    pub fn render(&self, renderer: &mut WgpuRenderer, alpha: f32) {
        // Invulnerability flicker: skip rendering sprite on alternating intervals
        if self.invuln_timer > 0.0 {
            let flicker_phase = (self.invuln_timer / 0.09) as u32;
            if flicker_phase % 2 == 1 {
                return;
            }
        }

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

        // Interpolate player position between previous and current physics tick
        let interp_pos = self.prev_pos.lerp(self.pos, alpha);

        // Center 36x36 sprite over 18x26 collider
        let render_pos = interp_pos + Vec2::new(-9.0, -10.0);
        let render_size = Vec2::new(36.0, 36.0);

        // Form and state tint
        let base_tint = match self.form {
            super::PlayerForm::Beast => [1.0, 0.25, 0.25, 1.0],
            super::PlayerForm::Witch => [1.0, 1.0, 1.0, 1.0],
        };

        let tint = match self.state {
            PlayerState::DashFreeze => [1.9, 1.9, 1.9, 1.0], // Impact flash
            PlayerState::Dash => {
                if self.form == super::PlayerForm::Beast {
                    [1.9, 0.5, 0.5, 0.95]
                } else {
                    [0.7, 1.3, 1.9, 0.95] // Motion tint
                }
            }
            PlayerState::WallSlide => {
                if self.form == super::PlayerForm::Beast {
                    [1.0, 0.35, 0.35, 1.0]
                } else {
                    [1.0, 0.95, 0.9, 1.0]
                }
            }
            _ => base_tint,
        };

        if let Some(spritesheet) = &self.spritesheet {
            renderer.draw_texture_rect(
                render_pos,
                render_size,
                Vec2::new(u0, v0),
                Vec2::new(u1, v1),
                spritesheet.id,
                &spritesheet.bind_group,
                tint,
            );
        }
    }
}

