use glam::Vec2;
use crate::engine::SfxEvent;
use crate::physics::{
    AABB, PushableBox, BOX_DASH_IMPULSE_BEAST, BOX_DASH_IMPULSE_WITCH, PUSH_SPEED_MULT_BEAST,
    PUSH_SPEED_MULT_WITCH,
};
use super::{Player, PlayerForm, BEAST_SPEED_MULT};

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
                self.pending_sfx.push(SfxEvent::BeastHurt);

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
                if !self.dead {
                    self.pending_sfx.push(SfxEvent::Death);
                }
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
        crates: &mut [PushableBox],
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
                if !self.dead {
                    self.pending_sfx.push(SfxEvent::Death);
                }
                self.dead = true;
                break;
            }

            // Check spike collision before sub-step move
            self.handle_spike_collision(spike_tiles);
            if self.dead {
                break;
            }

            // Move and resolve along horizontal axis against walls and active crates
            self.resolve_horizontal_movement(sub_dt, solid_tiles);
            self.resolve_horizontal_crates(sub_dt, crates, solid_tiles);

            // Move and resolve along vertical axis against walls and active crates
            self.resolve_vertical_movement(sub_dt, solid_tiles, crates);

            // Resolve one-way platforms using start position of this sub-step
            self.resolve_one_way_platforms(step_prev_pos, dropping_down, one_way_platforms);

            // Check void after sub-step move (instant death)
            if self.check_hazard_collision(void_tiles) {
                if !self.dead {
                    self.pending_sfx.push(SfxEvent::Death);
                }
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

    //------------[Resolve horizontal collision and pushing with active crates]-------------------
    pub fn resolve_horizontal_crates(
        &mut self,
        sub_dt: f32,
        crates: &mut [PushableBox],
        solid_tiles: &[AABB],
    ) {
        if crates.is_empty() {
            return;
        }

        let player_box = self.aabb();

        // Find any active crate that player overlaps horizontally (with vertical overlap)
        let mut hit_idx = None;
        for (i, krate) in crates.iter().enumerate() {
            if !krate.active {
                continue;
            }
            let c_box = krate.aabb();
            let vertical_overlap =
                self.pos.y < c_box.max.y && self.pos.y + self.size.y > c_box.min.y + 2.0;
            if vertical_overlap && player_box.overlaps(&c_box) {
                hit_idx = Some(i);
                break;
            }
        }

        let hit_idx = match hit_idx {
            Some(i) => i,
            None => return,
        };

        // 1. Dash impact: apply immediate impulse to crate, stop player before penetrating
        if self.dash_timer > 0.0 {
            let dir = self.dash_dir.signum();
            let impulse = if self.form == PlayerForm::Beast {
                BOX_DASH_IMPULSE_BEAST
            } else {
                BOX_DASH_IMPULSE_WITCH
            };
            crates[hit_idx].vel.x = dir * impulse;
            // Emit crate dash sound only once per dash even if crate is pressed against a wall
            if !self.dash_crate_sfx_played {
                self.pending_sfx.push(SfxEvent::CrateDash);
                self.dash_crate_sfx_played = true;
            }

            if dir > 0.0 {
                self.pos.x = crates[hit_idx].pos.x - self.size.x;
            } else {
                self.pos.x = crates[hit_idx].pos.x + crates[hit_idx].size.x;
            }
            self.vel.x = 0.0;
            // Ensure zero residual crate or wall overlap after dash impact
            self.separate_horizontal_crates(crates, solid_tiles);
            return;
        }

        // 2. Normal pushing (chain pushing - Variant B)
        let crate_center_x = crates[hit_idx].pos.x + crates[hit_idx].size.x * 0.5;
        let player_center_x = self.pos.x + self.size.x * 0.5;
        let pushing_into_crate = (self.vel.x > 0.0 && player_center_x < crate_center_x)
            || (self.vel.x < 0.0 && player_center_x > crate_center_x);

        if pushing_into_crate {
            let dir = self.vel.x.signum();

            // Calculate actual penetration overlap to avoid double movement
            let overlap = if dir > 0.0 {
                (self.pos.x + self.size.x) - crates[hit_idx].pos.x
            } else {
                (crates[hit_idx].pos.x + crates[hit_idx].size.x) - self.pos.x
            };

            if overlap > 0.0 {
                // Build chain of adjacent crates in push direction
                let mut chain = vec![hit_idx];
                let mut current = hit_idx;
                loop {
                    let mut next_crate = None;
                    let curr_pos = crates[current].pos;
                    let curr_size = crates[current].size;

                    for (j, other) in crates.iter().enumerate() {
                        if !other.active || chain.contains(&j) {
                            continue;
                        }
                        let y_match = (other.pos.y - curr_pos.y).abs() < 24.0;
                        if !y_match {
                            continue;
                        }
                        if dir > 0.0 {
                            let x_gap = other.pos.x - (curr_pos.x + curr_size.x);
                            if (-1.0..=2.0).contains(&x_gap) {
                                next_crate = Some(j);
                                break;
                            }
                        } else {
                            let x_gap = curr_pos.x - (other.pos.x + other.size.x);
                            if (-1.0..=2.0).contains(&x_gap) {
                                next_crate = Some(j);
                                break;
                            }
                        }
                    }

                    if let Some(nxt) = next_crate {
                        chain.push(nxt);
                        current = nxt;
                    } else {
                        break;
                    }
                }

                let lead_idx = *chain.last().unwrap();
                let push_mult = if self.form == PlayerForm::Beast {
                    PUSH_SPEED_MULT_BEAST
                } else {
                    PUSH_SPEED_MULT_WITCH
                };
                let speed_mult = if self.form == PlayerForm::Beast {
                    BEAST_SPEED_MULT
                } else {
                    1.0
                };
                let max_push_speed = 250.0 * speed_mult * push_mult;
                let max_step_disp = max_push_speed * sub_dt;
                let push_disp = dir * overlap.min(max_step_disp);

                // Check walls blocking the lead crate
                let lead_box = crates[lead_idx].aabb();
                let mut min_wall_dist = f32::INFINITY;

                for wall in solid_tiles {
                    let y_match = lead_box.min.y < wall.max.y && lead_box.max.y > wall.min.y;
                    if !y_match {
                        continue;
                    }
                    if dir > 0.0 && wall.min.x >= lead_box.max.x - 0.5 {
                        let dist = (wall.min.x - lead_box.max.x).max(0.0);
                        min_wall_dist = min_wall_dist.min(dist);
                    } else if dir < 0.0 && wall.max.x <= lead_box.min.x + 0.5 {
                        let dist = (lead_box.min.x - wall.max.x).max(0.0);
                        min_wall_dist = min_wall_dist.min(dist);
                    }
                }

                if min_wall_dist <= 1e-4 {
                    // Blocked by solid wall: zero velocity and clamp position
                    for &c_idx in &chain {
                        crates[c_idx].vel.x = 0.0;
                    }
                    if dir > 0.0 {
                        self.pos.x = crates[chain[0]].pos.x - self.size.x;
                    } else {
                        self.pos.x = crates[chain[0]].pos.x + crates[chain[0]].size.x;
                    }
                    self.vel.x = 0.0;
                } else {
                    // Free to move: apply allowed push displacement
                    let allowed_disp = if dir > 0.0 {
                        push_disp.min(min_wall_dist)
                    } else {
                        push_disp.max(-min_wall_dist)
                    };

                    for &c_idx in &chain {
                        crates[c_idx].pos.x += allowed_disp;
                        crates[c_idx].vel.x = dir * max_push_speed;
                    }
                    if dir > 0.0 {
                        self.pos.x = crates[chain[0]].pos.x - self.size.x;
                    } else {
                        self.pos.x = crates[chain[0]].pos.x + crates[chain[0]].size.x;
                    }
                    self.vel.x = dir * max_push_speed;
                }
            }
        } else if self.vel.x == 0.0 {
            // Stationary horizontal contact: clamp player at crate edge without stopping sliding crate
            if player_center_x < crate_center_x {
                self.pos.x = crates[hit_idx].pos.x - self.size.x;
            } else {
                self.pos.x = crates[hit_idx].pos.x + crates[hit_idx].size.x;
            }
            self.vel.x = 0.0;
        }

        // Guarantee zero residual horizontal overlap with active crates and solid walls
        self.separate_horizontal_crates(crates, solid_tiles);
    }

    //------------[Separate horizontal crate and wall overlaps]-------------------
    fn separate_horizontal_crates(&mut self, crates: &[PushableBox], solid_tiles: &[AABB]) {
        let mut player_box = self.aabb();

        // Separate from every active crate overlapping horizontally (ignoring top landing edge)
        for krate in crates {
            if !krate.active {
                continue;
            }
            let c_box = krate.aabb();
            let vertical_overlap =
                self.pos.y < c_box.max.y && self.pos.y + self.size.y > c_box.min.y + 2.0;
            if vertical_overlap && player_box.overlaps(&c_box) {
                self.push_out_horizontal_span(c_box.min.x, c_box.max.x, true);
                player_box = self.aabb();
            }
        }

        // Re-clamp against solid tiles so crate separation never embeds player into a wall
        for wall in solid_tiles {
            if player_box.overlaps(wall) {
                self.push_out_horizontal_span(wall.min.x, wall.max.x, false);
                player_box = self.aabb();
            }
        }
    }

    //------------[Push player out of horizontal span without ULP overlap]-------------------
    fn push_out_horizontal_span(&mut self, min_x: f32, max_x: f32, only_zero_if_deep: bool) {
        let player_center_x = self.pos.x + self.size.x * 0.5;
        let span_center_x = (min_x + max_x) * 0.5;

        if player_center_x < span_center_x {
            let penetration = (self.pos.x + self.size.x) - min_x;
            self.pos.x = min_x - self.size.x;
            // Step down by at most a few ULPs so pos.x + size.x never exceeds min_x
            for _ in 0..4 {
                if self.pos.x + self.size.x <= min_x {
                    break;
                }
                self.pos.x = self.pos.x.next_down();
            }
            // Keep push velocity on tiny f32 rounding nudges, stop deeper horizontal overlap
            if (!only_zero_if_deep || penetration > 1e-3) && self.vel.x > 0.0 {
                self.vel.x = 0.0;
            }
        } else {
            let penetration = max_x - self.pos.x;
            self.pos.x = max_x;
            // Step up by at most a few ULPs so pos.x is never strictly below max_x
            for _ in 0..4 {
                if self.pos.x >= max_x {
                    break;
                }
                self.pos.x = self.pos.x.next_up();
            }
            if (!only_zero_if_deep || penetration > 1e-3) && self.vel.x < 0.0 {
                self.vel.x = 0.0;
            }
        }
    }

    //------------[Resolve vertical collision]-------------------
    pub fn resolve_vertical_movement(
        &mut self,
        dt: f32,
        solid_tiles: &[AABB],
        crates: &[PushableBox],
    ) {
        let was_grounded = self.grounded;
        // Record vertical bounds before moving to distinguish top/bottom contact from side contact
        let prev_y = self.pos.y;
        let prev_bottom = self.pos.y + self.size.y;
        self.pos.y += self.vel.y * dt;
        self.grounded = false;
        let mut player_box = self.aabb();

        // 1. Solid tiles
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

        // 2. Active crates (only land from above or bump head from below)
        for krate in crates {
            if !krate.active {
                continue;
            }
            let crate_box = krate.aabb();
            if player_box.overlaps(&crate_box) {
                if self.vel.y > 0.0 && prev_bottom <= crate_box.min.y + 4.0 {
                    self.pos.y = crate_box.min.y - self.size.y;
                    self.vel.y = 0.0;
                    self.grounded = true;
                } else if self.vel.y < 0.0 && prev_y >= crate_box.max.y - 1.0 {
                    self.pos.y = crate_box.max.y;
                    self.vel.y = 0.0;
                }
                player_box = self.aabb();
            }
        }

        // Re-clamp against solid tiles so crate resolution never pushes player into floor or ceiling
        for wall in solid_tiles {
            if player_box.overlaps(wall) {
                let player_center_y = self.pos.y + self.size.y * 0.5;
                let wall_center_y = (wall.min.y + wall.max.y) * 0.5;
                if player_center_y < wall_center_y {
                    self.pos.y = wall.min.y - self.size.y;
                    if self.vel.y > 0.0 {
                        self.vel.y = 0.0;
                    }
                    self.grounded = true;
                } else {
                    self.pos.y = wall.max.y;
                    if self.vel.y < 0.0 {
                        self.vel.y = 0.0;
                    }
                }
                player_box = self.aabb();
            }
        }

        // 3. Keep player grounded when standing flush on floor or crate at very small dt
        if !self.grounded && was_grounded && self.vel.y >= 0.0 {
            // Probe 0.5 px below feet because tiny gravity steps can be smaller than f32 precision
            let ground_probe = AABB::from_pos_size(
                Vec2::new(self.pos.x, self.pos.y + self.size.y),
                Vec2::new(self.size.x, 0.5),
            );
            for wall in solid_tiles {
                if ground_probe.overlaps(wall) {
                    self.pos.y = wall.min.y - self.size.y;
                    self.vel.y = 0.0;
                    self.grounded = true;
                    return;
                }
            }
            for krate in crates {
                if !krate.active {
                    continue;
                }
                let crate_box = krate.aabb();
                if ground_probe.overlaps(&crate_box) {
                    self.pos.y = crate_box.min.y - self.size.y;
                    self.vel.y = 0.0;
                    self.grounded = true;
                    return;
                }
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
            // Extend foot probe 0.5 px downward so standing on platforms stays stable at high FPS
            let probe_box = AABB::from_pos_size(self.pos, self.size + Vec2::new(0.0, 0.5));

            for plat in one_way_platforms {
                if prev_bottom <= plat.min.y + 4.0 && current_bottom >= plat.min.y - 1e-4 {
                    let plat_horizontal = AABB::from_pos_size(
                        Vec2::new(plat.min.x, plat.min.y),
                        Vec2::new(plat.max.x - plat.min.x, 8.0),
                    );
                    if probe_box.overlaps(&plat_horizontal) {
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

