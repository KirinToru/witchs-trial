use glam::Vec2;
use crate::physics::AABB;
use super::Player;

impl Player {
    //------------[Check hazard overlap]-------------------
    pub fn check_hazard_collision(&self, hazard_tiles: &[AABB]) -> bool {
        let box_hurt = self.hurtbox();
        hazard_tiles.iter().any(|h| box_hurt.overlaps(h))
    }

    //------------[Handle spike hazard collision and damage]-------------------
    pub fn handle_spike_collision(&mut self, spike_tiles: &[AABB]) {
        if !self.check_hazard_collision(spike_tiles) {
            return;
        }
        // Spikes are ignored while invulnerable
        if self.invuln_timer > 0.0 {
            return;
        }

        match self.form {
            super::PlayerForm::Beast => {
                // Revert to Witch form and gain 2.0s invulnerability
                self.form = super::PlayerForm::Witch;
                self.invuln_timer = 2.0;
                self.knockback_lock_timer = 0.15;
                self.dash_timer = 0.0;
                self.dash_freeze_timer = 0.0;

                // Push away from velocity or facing direction
                let knockback_dir = if self.vel.x.abs() > 10.0 {
                    -self.vel.x.signum()
                } else {
                    -self.facing.signum()
                };
                self.vel.x = knockback_dir * super::BEAST_HIT_KNOCKBACK_X;
                self.vel.y = super::BEAST_HIT_KNOCKBACK_Y;
                self.grounded = false;
            }
            super::PlayerForm::Witch => {
                self.dead = true;
            }
        }
    }

    //------------[Execute sub-stepped movement and collision]-------------------
    // Splits movement into safe sub-steps if velocity is high to prevent tunneling
    pub fn move_and_collide(
        &mut self,
        dt: f32,
        dropping_down: bool,
        solid_tiles: &[AABB],
        one_way_platforms: &[AABB],
        void_tiles: &[AABB],
        spike_tiles: &[AABB],
    ) {
        // Calculate maximum axis displacement
        let max_displacement = (self.vel.x.abs() * dt).max(self.vel.y.abs() * dt);
        // Safe threshold is 8.0 pixels per sub-step
        let steps = ((max_displacement / 8.0).ceil() as usize).max(1);
        let sub_dt = dt / steps as f32;

        for _ in 0..steps {
            let step_prev_pos = self.pos;

            // Check void before sub-step move (instant death)
            if self.check_hazard_collision(void_tiles) {
                self.dead = true;
                break;
            }

            // Check spike collision before sub-step move
            self.handle_spike_collision(spike_tiles);
            if self.dead {
                break;
            }

            // Move and resolve along horizontal axis
            self.resolve_horizontal_movement(sub_dt, solid_tiles);

            // Move and resolve along vertical axis
            self.resolve_vertical_movement(sub_dt, solid_tiles);

            // Resolve one-way platforms using start position of this sub-step
            self.resolve_one_way_platforms(step_prev_pos, dropping_down, one_way_platforms);

            // Check void after sub-step move (instant death)
            if self.check_hazard_collision(void_tiles) {
                self.dead = true;
                break;
            }

            // Check spike collision after sub-step move
            self.handle_spike_collision(spike_tiles);
            if self.dead {
                break;
            }
        }
    }

    //------------[Resolve horizontal collision]-------------------
    pub fn resolve_horizontal_movement(&mut self, dt: f32, solid_tiles: &[AABB]) {
        if self.vel.x == 0.0 {
            return;
        }
        self.pos.x += self.vel.x * dt;
        let mut player_box = self.aabb();
        for wall in solid_tiles {
            if player_box.overlaps(wall) {
                if self.vel.x > 0.0 {
                    self.pos.x = wall.min.x - self.size.x;
                } else if self.vel.x < 0.0 {
                    self.pos.x = wall.max.x;
                }
                self.vel.x = 0.0;
                player_box = self.aabb();
            }
        }
    }

    //------------[Resolve vertical collision]-------------------
    pub fn resolve_vertical_movement(&mut self, dt: f32, solid_tiles: &[AABB]) {
        let was_grounded = self.grounded;
        self.pos.y += self.vel.y * dt;
        self.grounded = false;
        let mut player_box = self.aabb();

        for wall in solid_tiles {
            if player_box.overlaps(wall) {
                if self.vel.y > 0.0 {
                    self.pos.y = wall.min.y - self.size.y;
                    self.vel.y = 0.0;
                    self.grounded = true;
                } else if self.vel.y < 0.0 {
                    self.pos.y = wall.max.y;
                    self.vel.y = 0.0;
                }
                player_box = self.aabb();
            }
        }

        // If player was grounded and did not move vertically, verify solid ground contact
        if !self.grounded && was_grounded && self.vel.y == 0.0 {
            let ground_probe = AABB::from_pos_size(
                Vec2::new(self.pos.x + 2.0, self.pos.y + self.size.y),
                Vec2::new(self.size.x - 4.0, 1.0),
            );
            if solid_tiles.iter().any(|wall| ground_probe.overlaps(wall)) {
                self.grounded = true;
            }
        }
    }

    //------------[Resolve one-way platform collision]-------------------
    pub fn resolve_one_way_platforms(
        &mut self,
        prev_pos: Vec2,
        dropping_down: bool,
        one_way_platforms: &[AABB],
    ) {
        if !dropping_down && self.vel.y >= 0.0 {
            let prev_bottom = prev_pos.y + self.size.y;
            let current_bottom = self.pos.y + self.size.y;

            for plat in one_way_platforms {
                if prev_bottom <= plat.min.y + 4.0 && current_bottom >= plat.min.y {
                    let plat_horizontal = AABB::from_pos_size(
                        Vec2::new(plat.min.x, plat.min.y),
                        Vec2::new(plat.max.x - plat.min.x, 8.0),
                    );
                    if self.aabb().overlaps(&plat_horizontal) {
                        self.pos.y = plat.min.y - self.size.y;
                        self.vel.y = 0.0;
                        self.grounded = true;
                        break;
                    }
                }
            }
        }
    }

    //------------[Update wall proximity probes]-------------------
    pub fn update_wall_probes(&mut self, solid_tiles: &[AABB]) {
        let probe_margin = 2.0;
        let left_probe = AABB::from_pos_size(
            self.pos + Vec2::new(-probe_margin, 4.0),
            Vec2::new(probe_margin, self.size.y - 8.0),
        );
        let right_probe = AABB::from_pos_size(
            self.pos + Vec2::new(self.size.x, 4.0),
            Vec2::new(probe_margin, self.size.y - 8.0),
        );

        let mut left = false;
        let mut right = false;
        for wall in solid_tiles {
            if !left && left_probe.overlaps(wall) {
                left = true;
            }
            if !right && right_probe.overlaps(wall) {
                right = true;
            }
            if left && right {
                break;
            }
        }
        self.walled_left = left;
        self.walled_right = right;
    }
}

