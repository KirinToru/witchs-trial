use crate::engine::{InputState, Key, Texture, WgpuRenderer};
use crate::map::{InteractiveKind, InteractiveTile};
use crate::physics::AABB;
use glam::Vec2;

pub mod collision;
pub mod kinematics;
pub mod render;
pub mod state;

pub use kinematics::move_towards;
pub use state::PlayerState;

pub struct Player {
    pub pos: Vec2,
    pub vel: Vec2,
    pub size: Vec2,
    pub facing: f32,
    pub grounded: bool,
    pub state: PlayerState,
    pub dead: bool,
    pub checkpoint_pos: Vec2,
    pub walled_left: bool,
    pub walled_right: bool,
    pub wall_slide_timer: f32,
    pub wall_jump_lock_timer: f32,
    pub dash_freeze_timer: f32,
    pub dash_timer: f32,
    pub dash_dir: f32,
    pub dash_cooldown: f32,
    pub can_dash: bool,
    pub coyote_timer: f32,
    pub jump_buffer_timer: f32,
    pub dash_buffer_timer: f32,
    pub jumps_count: u32,
    pub dashes_count: u32,
    pub bhop_velocity: f32,
    pub bhop_timer: f32,
    pub is_bhopping: bool,
    pub bhop_chain: u32,
    pub auto_bhop: bool,
    pub status_message: Option<String>,
    pub status_timer: f32,
    pub(crate) anim_timer: f32,
    pub(crate) anim_frame: usize,
    pub spritesheet: Texture,
}

impl Player {
    //------------[Create player entity]-------------------
    pub fn new(spawn_pos: Vec2, renderer: &WgpuRenderer) -> Result<Self, String> {
        let spritesheet = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/player/spritesheet.png",
            &renderer.texture_bind_group_layout,
        )?;

        Ok(Self {
            pos: spawn_pos,
            vel: Vec2::ZERO,
            size: Vec2::new(18.0, 26.0),
            facing: 1.0,
            grounded: false,
            state: PlayerState::Idle,
            dead: false,
            checkpoint_pos: spawn_pos,
            walled_left: false,
            walled_right: false,
            wall_slide_timer: 0.0,
            wall_jump_lock_timer: 0.0,
            dash_freeze_timer: 0.0,
            dash_timer: 0.0,
            dash_dir: 1.0,
            dash_cooldown: 0.0,
            can_dash: true,
            coyote_timer: 0.0,
            jump_buffer_timer: 0.0,
            dash_buffer_timer: 0.0,
            jumps_count: 0,
            dashes_count: 0,
            bhop_velocity: 0.0,
            bhop_timer: 0.0,
            is_bhopping: false,
            bhop_chain: 0,
            auto_bhop: true,
            status_message: None,
            status_timer: 0.0,
            anim_timer: 0.0,
            anim_frame: 0,
            spritesheet,
        })
    }

    //------------[Reset player state and position]-------------------
    pub fn reset(&mut self, reset_pos: Vec2) {
        self.pos = reset_pos;
        self.vel = Vec2::ZERO;
        self.grounded = false;
        self.state = PlayerState::Idle;
        self.dead = false;
        self.walled_left = false;
        self.walled_right = false;
        self.wall_slide_timer = 0.0;
        self.wall_jump_lock_timer = 0.0;
        self.dash_freeze_timer = 0.0;
        self.dash_timer = 0.0;
        self.dash_cooldown = 0.0;
        self.can_dash = true;
        self.coyote_timer = 0.0;
        self.jump_buffer_timer = 0.0;
        self.bhop_velocity = 0.0;
        self.bhop_timer = 0.0;
        self.is_bhopping = false;
        self.anim_timer = 0.0;
        self.anim_frame = 0;
    }

    //------------[Get player collision AABB]-------------------
    pub fn aabb(&self) -> AABB {
        AABB::from_pos_size(self.pos, self.size)
    }

    //------------[Get player hazard hurtbox]-------------------
    pub fn hurtbox(&self) -> AABB {
        let inset = Vec2::new(2.0, 2.0);
        AABB::new(self.pos + inset, self.pos + self.size - inset)
    }

    //------------[Update player physics and state]-------------------
    pub fn update(
        &mut self,
        dt: f32,
        input: &InputState,
        solid_tiles: &[AABB],
        one_way_platforms: &[AABB],
        hazard_tiles: &[AABB],
        interactives: &mut [InteractiveTile],
    ) {
        if self.dead {
            return;
        }

        let was_grounded = self.grounded;

        // Timers and buffers update
        if self.status_timer > 0.0 {
            self.status_timer -= dt;
            if self.status_timer <= 0.0 {
                self.status_message = None;
            }
        }
        if self.wall_jump_lock_timer > 0.0 {
            self.wall_jump_lock_timer -= dt;
        }
        if self.dash_cooldown > 0.0 {
            self.dash_cooldown -= dt;
            if self.dash_cooldown <= 0.0 && !self.can_dash {
                self.can_dash = true;
            }
        }
        if self.bhop_timer > 0.0 {
            self.bhop_timer -= dt;
            if self.bhop_timer <= 0.0 && self.grounded {
                self.bhop_velocity = 0.0;
                self.is_bhopping = false;
            }
        }
        if self.grounded {
            self.coyote_timer = 0.12;
            self.can_dash = true;
        } else if self.coyote_timer > 0.0 {
            self.coyote_timer -= dt;
        }

        // Jump buffer (renew while holding Space if auto-bhop is on)
        if input.is_just_pressed(Key::Jump) || (self.auto_bhop && input.is_pressed(Key::Jump)) {
            self.jump_buffer_timer = 0.16;
        } else if self.jump_buffer_timer > 0.0 {
            self.jump_buffer_timer -= dt;
        }

        // Dash buffer
        if input.is_just_pressed(Key::Dash) {
            self.dash_buffer_timer = 0.16;
        } else if self.dash_buffer_timer > 0.0 {
            self.dash_buffer_timer -= dt;
        }

        // Checkpoint / exit interaction (W key)
        if input.is_just_pressed(Key::Interact) {
            let player_center = self.pos + self.size * 0.5;
            for item in interactives.iter_mut() {
                let item_center = item.pos + item.size * 0.5;
                if player_center.distance(item_center) < 36.0 {
                    match item.kind {
                        InteractiveKind::SpawnPoint => {
                            self.checkpoint_pos = item.pos;
                            self.status_message = Some("Spawn Point Registered".to_string());
                            self.status_timer = 2.0;
                        }
                        InteractiveKind::Checkpoint => {
                            self.checkpoint_pos = item.pos;
                            item.activated = true;
                            self.status_message = Some("Checkpoint Activated!".to_string());
                            self.status_timer = 2.5;
                        }
                        InteractiveKind::LevelExit => {
                            item.activated = true;
                            self.status_message =
                                Some("Level Completed! Press R to Restart".to_string());
                            self.status_timer = 3.0;
                        }
                        InteractiveKind::Light => {}
                    }
                }
            }
        }

        // Dash trigger
        if self.dash_buffer_timer > 0.0
            && self.can_dash
            && self.dash_freeze_timer <= 0.0
            && self.dash_timer <= 0.0
            && self.dash_cooldown <= 0.0
        {
            self.dash_buffer_timer = 0.0;
            let requested_dir = if input.is_pressed(Key::Right) {
                1.0
            } else if input.is_pressed(Key::Left) {
                -1.0
            } else {
                self.facing
            };

            self.dash_dir = requested_dir.signum();
            self.facing = self.dash_dir;
            self.dash_freeze_timer = 0.06; // 60 ms freeze impact
            self.vel = Vec2::ZERO;
            self.can_dash = false;
            self.dash_cooldown = 0.45;
            self.dashes_count += 1;
        }

        // Dash freeze phase
        if self.dash_freeze_timer > 0.0 {
            self.dash_freeze_timer -= dt;
            self.vel = Vec2::ZERO;
            self.state = PlayerState::DashFreeze;

            if self.dash_freeze_timer <= 0.0 {
                // Launch horizontal impulse at 600 px/s
                self.dash_timer = 0.20;
                self.vel.x = self.dash_dir * 600.0;
                self.vel.y = 0.0;
                self.state = PlayerState::Dash;
            } else {
                return;
            }
        }

        // Dash impulse phase (smooth speed taper 600 -> 300)
        if self.dash_timer > 0.0 {
            self.dash_timer -= dt;
            self.vel.y = 0.0;
            self.state = PlayerState::Dash;

            let t = (self.dash_timer / 0.20).clamp(0.0, 1.0);
            let current_dash_speed = 300.0 + (600.0 - 300.0) * t;
            self.vel.x = self.dash_dir * current_dash_speed;

            if self.dash_timer <= 0.0 {
                let exit_speed = 300.0;
                self.vel.x = self.dash_dir * exit_speed;
                self.bhop_velocity = self.dash_dir * exit_speed;
                self.bhop_timer = 0.25;
                self.is_bhopping = true;
                self.bhop_chain = 1;
            }
        }

        // Horizontal movement input
        let mut move_x: f32 = 0.0;
        if input.is_pressed(Key::Right) {
            move_x += 1.0;
        }
        if input.is_pressed(Key::Left) {
            move_x -= 1.0;
        }

        // Cancel wall jump lock on counter-steer
        if move_x != 0.0
            && (move_x.signum() != self.vel.x.signum() || self.wall_jump_lock_timer <= 0.04)
        {
            self.wall_jump_lock_timer = 0.0;
        }

        if self.dash_timer <= 0.0 && self.wall_jump_lock_timer <= 0.0 {
            let max_ground_speed = 250.0;
            let max_air_speed = 300.0;

            if self.grounded {
                let accel = 1800.0;
                let friction = 2000.0;

                if self.bhop_timer > 0.0 && self.is_bhopping {
                    // Skip ground friction during active bhop window
                    self.bhop_timer -= dt;
                    if move_x != 0.0 {
                        self.facing = move_x;
                    }
                } else {
                    self.is_bhopping = false;
                    self.bhop_chain = 0;

                    if move_x != 0.0 {
                        self.facing = move_x;
                        let target_vel = move_x * max_ground_speed;
                        self.vel.x = move_towards(self.vel.x, target_vel, accel * dt);
                    } else {
                        self.vel.x = move_towards(self.vel.x, 0.0, friction * dt);
                    }
                }
            } else {
                // In air: speed cap is 300 px/s
                if move_x != 0.0 {
                    self.facing = move_x;
                    if self.is_bhopping && move_x.signum() == self.vel.x.signum() {
                        self.vel.x = move_towards(self.vel.x, move_x * max_air_speed, 420.0 * dt);
                    } else {
                        let target_speed = if self.is_bhopping {
                            self.vel.x.abs().min(max_air_speed)
                        } else {
                            max_ground_speed
                        };
                        let air_accel =
                            if move_x.signum() != self.vel.x.signum() && self.vel.x.abs() > 20.0 {
                                3200.0 // Snappy turnaround
                            } else {
                                1500.0 // Normal air acceleration
                            };
                        self.vel.x =
                            move_towards(self.vel.x, move_x * target_speed, air_accel * dt);
                        if move_x.signum() != self.vel.x.signum() {
                            self.is_bhopping = false;
                        }
                    }
                } else if !self.is_bhopping {
                    if self.vel.x.abs() > max_ground_speed {
                        self.vel.x = move_towards(
                            self.vel.x,
                            self.vel.x.signum() * max_ground_speed,
                            950.0 * dt,
                        );
                    }
                }

                self.vel.x = self.vel.x.clamp(-max_air_speed, max_air_speed);
            }
        }

        // Wall slide check
        let is_touching_wall_direction = (!self.grounded)
            && ((self.walled_left && move_x < 0.0) || (self.walled_right && move_x > 0.0));

        let mut is_wall_sliding = false;

        if is_touching_wall_direction && self.dash_timer <= 0.0 && self.vel.y >= -180.0 {
            is_wall_sliding = true;
            self.wall_slide_timer += dt;
            let initial_slide_speed = 35.0;
            let slide_acceleration = 480.0;
            let max_slide_speed = 440.0;
            self.vel.y = (initial_slide_speed + self.wall_slide_timer * slide_acceleration)
                .min(max_slide_speed);
        } else {
            self.wall_slide_timer = 0.0;
        }

        // Wall jump execution
        if self.jump_buffer_timer > 0.0
            && !self.grounded
            && (self.walled_left || self.walled_right)
            && self.dash_timer <= 0.0
        {
            let push_dir = if self.walled_left { 1.0 } else { -1.0 };
            self.vel.x = push_dir * 250.0;
            self.vel.y = -500.0;
            self.facing = push_dir;
            self.wall_jump_lock_timer = 0.08;
            self.wall_slide_timer = 0.0;
            self.jump_buffer_timer = 0.0;
            self.coyote_timer = 0.0;
            self.jumps_count += 1;
            self.is_bhopping = false;
        } else if self.jump_buffer_timer > 0.0
            && (self.grounded || self.coyote_timer > 0.0)
            && self.dash_timer <= 0.0
        {
            let max_air_speed = 300.0;
            // Bunnyhop chain timing
            let is_bhop_timing = (self.bhop_timer > 0.0 && self.is_bhopping)
                || (self.auto_bhop && input.is_pressed(Key::Jump) && self.vel.x.abs() > 80.0);

            if is_bhop_timing {
                let current_spd = self.vel.x.abs().max(self.bhop_velocity.abs()).max(250.0);
                let boosted_spd = (current_spd + 25.0).min(max_air_speed);
                let dir = if move_x != 0.0 {
                    move_x.signum()
                } else if self.vel.x != 0.0 {
                    self.vel.x.signum()
                } else {
                    self.facing
                };
                self.vel.x = dir * boosted_spd;
                self.facing = dir;
                self.bhop_velocity = self.vel.x;
                self.is_bhopping = true;
                self.bhop_chain += 1;
            } else {
                self.is_bhopping = false;
                self.bhop_chain = 0;
            }
            self.vel.y = -520.0;
            self.grounded = false;
            self.coyote_timer = 0.0;
            self.jump_buffer_timer = 0.0;
            self.bhop_timer = 0.0;
            self.jumps_count += 1;
        }

        // Gravity curve and variable jump height
        if self.dash_timer <= 0.0 && !is_wall_sliding {
            let gravity = if self.vel.y < 0.0 {
                if !input.is_pressed(Key::Jump) {
                    2400.0 // Cut jump height on key release
                } else {
                    1200.0 // Ascending gravity
                }
            } else if self.vel.y < 200.0 {
                1350.0 // Apex gravity
            } else {
                1750.0 // Falling gravity
            };

            self.vel.y = (self.vel.y + gravity * dt).min(720.0);
        }

        // Previous position for one-way platform check
        let prev_pos = self.pos;

        // Pre-move hazard overlap check
        if self.check_hazard_collision(hazard_tiles) {
            self.dead = true;
            return;
        }

        // Horizontal movement & collision
        self.resolve_horizontal_movement(dt, solid_tiles);

        // Vertical movement & collision
        self.resolve_vertical_movement(dt, solid_tiles);

        // One-way platforms (pass through from below, drop with S)
        let dropping_down = input.is_pressed(Key::Down);
        self.resolve_one_way_platforms(prev_pos, dropping_down, one_way_platforms);

        // Landing event
        let just_landed = !was_grounded && self.grounded;
        if just_landed {
            if self.is_bhopping && self.bhop_velocity.abs() > 200.0 {
                self.bhop_timer = 0.15;
            } else {
                self.bhop_timer = 0.0;
                self.is_bhopping = false;
                self.bhop_velocity = 0.0;
            }
        }

        // Post-move hazard check
        if self.check_hazard_collision(hazard_tiles) {
            self.dead = true;
        }

        // Wall proximity probes
        self.update_wall_probes(solid_tiles);

        // State machine update
        if self.dash_freeze_timer > 0.0 {
            self.state = PlayerState::DashFreeze;
        } else if self.dash_timer > 0.0 {
            self.state = PlayerState::Dash;
        } else if is_wall_sliding {
            self.state = PlayerState::WallSlide;
        } else if !self.grounded {
            if self.vel.y < 0.0 {
                self.state = PlayerState::Jump;
            } else {
                self.state = PlayerState::Fall;
            }
        } else if self.vel.x.abs() > 8.0 {
            self.state = PlayerState::Walk;
        } else {
            self.state = PlayerState::Idle;
        }

        // Sprite animation frame update
        let (_, frame_count, frame_duration) = self.state.anim_params();

        self.anim_timer += dt;
        if self.anim_timer >= frame_duration {
            self.anim_timer -= frame_duration;
            self.anim_frame = (self.anim_frame + 1) % frame_count;
        }
        if self.anim_frame >= frame_count {
            self.anim_frame = 0;
        }
    }
}

#[cfg(test)]
mod tests;

