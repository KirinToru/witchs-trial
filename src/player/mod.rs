//! Player entity kinematics, state transitions, collision resolution, and animation.

use crate::engine::{InputState, Key, SfxEvent, Texture, WgpuRenderer};
use crate::map::{InteractiveKind, InteractiveTile};
use crate::physics::{AABB, PushableBox};
use glam::Vec2;

pub mod collision;
pub mod kinematics;
pub mod render;
pub mod state;

pub use kinematics::move_towards;
pub use state::PlayerState;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlayerForm {
    Witch,
    Beast,
}

// Multipliers and parameters for The Beast transformation
pub const BEAST_SPEED_MULT: f32 = 1.40;
pub const BEAST_JUMP_MULT: f32 = 1.25;
pub const BEAST_DASH_SPEED_MULT: f32 = 1.35;
pub const BEAST_DASH_DURATION_MULT: f32 = 1.50;
pub const BEAST_HIT_KNOCKBACK_X: f32 = 320.0;
pub const BEAST_HIT_KNOCKBACK_Y: f32 = -380.0;

// Multipliers and physics constants for Ice surfaces
pub const ICE_SPEED_MULT: f32 = 1.35;
pub const ICE_ACCEL: f32 = 450.0;
pub const ICE_FRICTION: f32 = 250.0;

pub struct Player {
    pub pos: Vec2,
    pub prev_pos: Vec2,
    pub vel: Vec2,
    pub size: Vec2,
    pub facing: f32,
    pub grounded: bool,
    pub on_ice: bool,
    pub ice_jump_active: bool,
    pub state: PlayerState,
    pub dead: bool,
    pub form: PlayerForm,
    pub invuln_timer: f32,
    pub knockback_lock_timer: f32,
    pub checkpoint_pos: Vec2,
    pub walled_left: bool,
    pub walled_right: bool,
    pub last_wall_dir: Option<i8>,
    pub wall_slide_timer: f32,
    pub wall_jump_lock_timer: f32,
    pub dash_freeze_timer: f32,
    pub dash_timer: f32,
    pub dash_dir: f32,
    pub dash_cooldown: f32,
    pub can_dash: bool,
    pub coyote_timer: f32,
    pub jump_buffer_timer: f32,
    pub wall_jump_buffer_timer: f32,
    pub dash_buffer_timer: f32,
    pub(crate) dash_crate_sfx_played: bool,
    pub jumps_count: u32,
    pub dashes_count: u32,
    pub bhop_velocity: f32,
    pub bhop_timer: f32,
    pub is_bhopping: bool,
    pub bhop_chain: u32,
    pub auto_bhop: bool,
    pub status_message: Option<String>,
    pub status_timer: f32,
    pub pending_sfx: Vec<SfxEvent>,
    pub(crate) anim_timer: f32,
    pub(crate) anim_frame: usize,
    pub spritesheet: Option<Texture>,
}

impl Player {
    //------------[Create headless player entity without GPU resources]-------------------
    pub fn new_headless(spawn_pos: Vec2) -> Self {
        Self {
            pos: spawn_pos,
            prev_pos: spawn_pos,
            vel: Vec2::ZERO,
            size: Vec2::new(18.0, 26.0),
            facing: 1.0,
            grounded: false,
            on_ice: false,
            ice_jump_active: false,
            state: PlayerState::Idle,
            dead: false,
            form: PlayerForm::Witch,
            invuln_timer: 0.0,
            knockback_lock_timer: 0.0,
            checkpoint_pos: spawn_pos,
            walled_left: false,
            walled_right: false,
            last_wall_dir: None,
            wall_slide_timer: 0.0,
            wall_jump_lock_timer: 0.0,
            dash_freeze_timer: 0.0,
            dash_timer: 0.0,
            dash_dir: 1.0,
            dash_cooldown: 0.0,
            can_dash: true,
            coyote_timer: 0.0,
            jump_buffer_timer: 0.0,
            wall_jump_buffer_timer: 0.0,
            dash_buffer_timer: 0.0,
            dash_crate_sfx_played: false,
            jumps_count: 0,
            dashes_count: 0,
            bhop_velocity: 0.0,
            bhop_timer: 0.0,
            is_bhopping: false,
            bhop_chain: 0,
            auto_bhop: true,
            status_message: None,
            status_timer: 0.0,
            pending_sfx: Vec::new(),
            anim_timer: 0.0,
            anim_frame: 0,
            spritesheet: None,
        }
    }

    //------------[Create player entity with GPU textures]-------------------
    pub fn new(spawn_pos: Vec2, renderer: &WgpuRenderer) -> Result<Self, String> {
        let mut player = Self::new_headless(spawn_pos);
        let spritesheet = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/player/spritesheet.png",
            &renderer.texture_bind_group_layout,
        )?;
        player.spritesheet = Some(spritesheet);
        Ok(player)
    }

    //------------[Reset player state and position]-------------------
    pub fn reset(&mut self, reset_pos: Vec2) {
        self.pos = reset_pos;
        self.prev_pos = reset_pos;
        self.vel = Vec2::ZERO;
        self.grounded = false;
        self.on_ice = false;
        self.ice_jump_active = false;
        self.state = PlayerState::Idle;
        self.dead = false;
        self.form = PlayerForm::Witch;
        self.invuln_timer = 0.0;
        self.knockback_lock_timer = 0.0;
        self.walled_left = false;
        self.walled_right = false;
        self.last_wall_dir = None;
        self.wall_slide_timer = 0.0;
        self.wall_jump_lock_timer = 0.0;
        self.dash_freeze_timer = 0.0;
        self.dash_timer = 0.0;
        self.dash_cooldown = 0.0;
        self.can_dash = true;
        self.coyote_timer = 0.0;
        self.jump_buffer_timer = 0.0;
        self.wall_jump_buffer_timer = 0.0;
        self.bhop_velocity = 0.0;
        self.bhop_timer = 0.0;
        self.is_bhopping = false;
        self.dash_crate_sfx_played = false;
        self.pending_sfx.clear();
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

    //------------[Check if player is standing on ice]-------------------
    pub fn check_on_ice(&self, ice_tiles: &[AABB]) -> bool {
        if !self.grounded {
            return false;
        }
        let probe_height = 2.0;
        let probe_inset = 2.0;
        let probe_width = self.size.x - probe_inset * 2.0;
        let ice_probe = AABB::from_pos_size(
            Vec2::new(self.pos.x + probe_inset, self.pos.y + self.size.y),
            Vec2::new(probe_width, probe_height),
        );
        ice_tiles.iter().any(|ice| ice_probe.overlaps(ice))
    }

    //------------[Update player physics and state]-------------------
    pub fn update(
        &mut self,
        dt: f32,
        input: &InputState,
        solid_tiles: &[AABB],
        one_way_platforms: &[AABB],
        void_tiles: &[AABB],
        spike_tiles: &[AABB],
        ice_tiles: &[AABB],
        crates: &mut [PushableBox],
        interactives: &mut [InteractiveTile],
    ) {
        if self.dead {
            return;
        }

        // Store previous position before physics tick for sub-frame interpolation
        self.prev_pos = self.pos;

        let was_grounded = self.grounded;
        let was_on_ice = self.on_ice;
        let prev_state = self.state;

        // Detect ice surface under feet before movement logic
        self.on_ice = self.check_on_ice(ice_tiles);

        // Timers and buffers update
        if self.status_timer > 0.0 {
            self.status_timer -= dt;
            if self.status_timer <= 0.0 {
                self.status_message = None;
            }
        }
        if self.invuln_timer > 0.0 {
            self.invuln_timer = (self.invuln_timer - dt).max(0.0);
        }
        if self.knockback_lock_timer > 0.0 {
            self.knockback_lock_timer = (self.knockback_lock_timer - dt).max(0.0);
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
            // Ground contact resets wall jump limit
            self.last_wall_dir = None;
        } else if self.coyote_timer > 0.0 {
            self.coyote_timer -= dt;
        }

        // Jump and wall-jump buffers (loaded exclusively on fresh press)
        if input.is_just_pressed(Key::Jump) {
            self.jump_buffer_timer = 0.16;
            self.wall_jump_buffer_timer = 0.08;
        } else {
            if self.jump_buffer_timer > 0.0 {
                self.jump_buffer_timer -= dt;
            }
            if self.wall_jump_buffer_timer > 0.0 {
                self.wall_jump_buffer_timer -= dt;
            }
        }

        // Dash buffer
        if input.is_just_pressed(Key::Dash) {
            self.dash_buffer_timer = 0.16;
        } else if self.dash_buffer_timer > 0.0 {
            self.dash_buffer_timer -= dt;
        }

        // Moon collectible overlap check (activates The Beast transformation)
        let player_box = self.aabb();
        for item in interactives.iter_mut() {
            if item.kind == InteractiveKind::Moon && !item.activated {
                let item_box = AABB::from_pos_size(item.pos, item.size);
                if player_box.overlaps(&item_box) {
                    item.activated = true;
                    self.form = PlayerForm::Beast;
                    self.pending_sfx.push(SfxEvent::BeastTransform);
                    self.status_message = Some("The Beast Unleashed!".to_string());
                    self.status_timer = 2.0;
                }
            }
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
                            if !item.activated || self.checkpoint_pos != item.pos {
                                self.pending_sfx.push(SfxEvent::Checkpoint);
                            }
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
                        InteractiveKind::Light | InteractiveKind::Moon => {}
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
            // Allow CrateDash SFX once for this new dash
            self.dash_crate_sfx_played = false;
            self.pending_sfx.push(SfxEvent::Dash);
        }

        // Dash freeze phase
        if self.dash_freeze_timer > 0.0 {
            self.dash_freeze_timer -= dt;
            self.vel = Vec2::ZERO;
            self.state = PlayerState::DashFreeze;
            if self.state != prev_state {
                // Reset animation timing when entering dash freeze
                self.anim_timer = 0.0;
                self.anim_frame = 0;
            }

            if self.dash_freeze_timer <= 0.0 {
                let dash_dur = 0.20
                    * if self.form == PlayerForm::Beast {
                        BEAST_DASH_DURATION_MULT
                    } else {
                        1.0
                    };
                let dash_spd = 600.0
                    * if self.form == PlayerForm::Beast {
                        BEAST_DASH_SPEED_MULT
                    } else {
                        1.0
                    };
                self.dash_timer = dash_dur;
                self.vel.x = self.dash_dir * dash_spd;
                self.vel.y = 0.0;
                self.state = PlayerState::Dash;
            } else {
                return;
            }
        }

        // Dash impulse phase (smooth speed taper from max to exit speed)
        if self.dash_timer > 0.0 {
            self.dash_timer -= dt;
            self.vel.y = 0.0;
            self.state = PlayerState::Dash;

            let dash_dur = 0.20
                * if self.form == PlayerForm::Beast {
                    BEAST_DASH_DURATION_MULT
                } else {
                    1.0
                };
            let max_dash_spd = 600.0
                * if self.form == PlayerForm::Beast {
                    BEAST_DASH_SPEED_MULT
                } else {
                    1.0
                };
            let exit_dash_spd = 300.0
                * if self.form == PlayerForm::Beast {
                    BEAST_DASH_SPEED_MULT
                } else {
                    1.0
                };

            let t = (self.dash_timer / dash_dur).clamp(0.0, 1.0);
            let current_dash_speed = exit_dash_spd + (max_dash_spd - exit_dash_spd) * t;
            self.vel.x = self.dash_dir * current_dash_speed;

            if self.dash_timer <= 0.0 {
                self.vel.x = self.dash_dir * exit_dash_spd;
                self.bhop_velocity = self.dash_dir * exit_dash_spd;
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

        if self.dash_timer <= 0.0
            && self.wall_jump_lock_timer <= 0.0
            && self.knockback_lock_timer <= 0.0
        {
            let speed_mult = if self.form == PlayerForm::Beast {
                BEAST_SPEED_MULT
            } else {
                1.0
            };
            let max_ground_speed =
                250.0 * speed_mult * if self.on_ice { ICE_SPEED_MULT } else { 1.0 };

            if self.grounded {
                let accel = if self.on_ice { ICE_ACCEL } else { 1800.0 };
                let friction = if self.on_ice { ICE_FRICTION } else { 2000.0 };

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
                // Cancel ice jump bonus on air counter-steer
                if self.ice_jump_active
                    && move_x != 0.0
                    && move_x.signum() != self.vel.x.signum()
                    && self.vel.x.abs() > 20.0
                {
                    self.ice_jump_active = false;
                }

                let effective_air_cap = 300.0
                    * speed_mult
                    * if self.ice_jump_active {
                        ICE_SPEED_MULT
                    } else {
                        1.0
                    };

                // In air: speed cap scales with form and ice jump
                if move_x != 0.0 {
                    self.facing = move_x;
                    if self.is_bhopping && move_x.signum() == self.vel.x.signum() {
                        self.vel.x =
                            move_towards(self.vel.x, move_x * effective_air_cap, 420.0 * dt);
                    } else {
                        let target_speed = if self.is_bhopping {
                            self.vel.x.abs().min(effective_air_cap)
                        } else if self.ice_jump_active {
                            (250.0 * speed_mult * ICE_SPEED_MULT).min(effective_air_cap)
                        } else {
                            250.0 * speed_mult
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
                    let air_base_speed = 250.0
                        * speed_mult
                        * if self.ice_jump_active {
                            ICE_SPEED_MULT
                        } else {
                            1.0
                        };
                    if self.vel.x.abs() > air_base_speed {
                        self.vel.x = move_towards(
                            self.vel.x,
                            self.vel.x.signum() * air_base_speed,
                            950.0 * dt,
                        );
                    }
                }

                self.vel.x = self.vel.x.clamp(-effective_air_cap, effective_air_cap);
            }
        }

        // Wall interaction check (Witch vs Beast)
        let is_touching_wall_direction = (!self.grounded)
            && ((self.walled_left && move_x < 0.0) || (self.walled_right && move_x > 0.0));

        let mut is_wall_sliding = false;

        if is_touching_wall_direction && self.dash_timer <= 0.0 && self.vel.y >= -180.0 {
            if self.form == PlayerForm::Beast {
                if input.is_pressed(Key::Down) {
                    // Beast wall slide when holding Down
                    is_wall_sliding = true;
                    self.wall_slide_timer += dt;
                    let initial_slide_speed = 35.0;
                    let slide_acceleration = 480.0;
                    let max_slide_speed = 440.0;
                    self.vel.y = (initial_slide_speed + self.wall_slide_timer * slide_acceleration)
                        .min(max_slide_speed);
                } else {
                    // Beast stick to wall in place without falling
                    is_wall_sliding = true;
                    self.wall_slide_timer = 0.0;
                    self.vel.y = 0.0;
                }
            } else {
                // Witch standard wall slide
                is_wall_sliding = true;
                self.wall_slide_timer += dt;
                let initial_slide_speed = 35.0;
                let slide_acceleration = 480.0;
                let max_slide_speed = 440.0;
                self.vel.y = (initial_slide_speed + self.wall_slide_timer * slide_acceleration)
                    .min(max_slide_speed);
            }
        } else {
            self.wall_slide_timer = 0.0;
        }

        // Wall jump execution (limit 1 jump per wall, alternating allowed)
        let can_jump_left = self.walled_left && self.last_wall_dir != Some(-1);
        let can_jump_right = self.walled_right && self.last_wall_dir != Some(1);

        // When touching both walls, prioritize the wall opposite to the last used one
        let wall_jump_dir: Option<i8> = if can_jump_left && can_jump_right {
            if self.last_wall_dir == Some(-1) {
                Some(1)
            } else {
                Some(-1)
            }
        } else if can_jump_left {
            Some(-1)
        } else if can_jump_right {
            Some(1)
        } else {
            None
        };

        let eligible_wall_jump = if self.wall_jump_buffer_timer > 0.0
            && !self.grounded
            && self.dash_timer <= 0.0
        {
            wall_jump_dir
        } else {
            None
        };

        if let Some(wall_dir) = eligible_wall_jump {
            let push_dir = if wall_dir == -1 { 1.0 } else { -1.0 };
            let jump_mult = if self.form == PlayerForm::Beast {
                BEAST_JUMP_MULT
            } else {
                1.0
            };
            let speed_mult = if self.form == PlayerForm::Beast {
                BEAST_SPEED_MULT
            } else {
                1.0
            };
            self.vel.x = push_dir * 250.0 * speed_mult;
            self.vel.y = -500.0 * jump_mult;
            self.facing = push_dir;
            self.wall_jump_lock_timer = 0.08;
            self.wall_slide_timer = 0.0;
            self.jump_buffer_timer = 0.0;
            self.wall_jump_buffer_timer = 0.0;
            self.coyote_timer = 0.0;
            self.jumps_count += 1;
            self.is_bhopping = false;
            // Record last used wall direction (-1 for left, 1 for right)
            self.last_wall_dir = Some(wall_dir);
            // Wall jump cancels ice jump momentum bonus
            self.ice_jump_active = false;
            self.pending_sfx.push(SfxEvent::WallJump);
        } else if ((self.jump_buffer_timer > 0.0 && (self.grounded || self.coyote_timer > 0.0))
            || (self.grounded && self.auto_bhop && input.is_pressed(Key::Jump)))
            && self.dash_timer <= 0.0
        {
            // Activate ice jump momentum preservation when launching from ice
            if self.on_ice || (self.coyote_timer > 0.0 && was_on_ice) {
                self.ice_jump_active = true;
            }
            let speed_mult = if self.form == PlayerForm::Beast {
                BEAST_SPEED_MULT
            } else {
                1.0
            };
            let jump_mult = if self.form == PlayerForm::Beast {
                BEAST_JUMP_MULT
            } else {
                1.0
            };
            let max_air_speed = 300.0
                * speed_mult
                * if self.ice_jump_active {
                    ICE_SPEED_MULT
                } else {
                    1.0
                };
            // Bunnyhop chain timing
            let is_bhop_timing = (self.bhop_timer > 0.0 && self.is_bhopping)
                || (self.auto_bhop && input.is_pressed(Key::Jump) && self.vel.x.abs() > 80.0);

            if is_bhop_timing {
                let current_spd = self
                    .vel
                    .x
                    .abs()
                    .max(self.bhop_velocity.abs())
                    .max(250.0 * speed_mult);
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
            self.vel.y = -520.0 * jump_mult;
            self.grounded = false;
            self.coyote_timer = 0.0;
            self.jump_buffer_timer = 0.0;
            self.wall_jump_buffer_timer = 0.0;
            self.bhop_timer = 0.0;
            self.jumps_count += 1;
            self.pending_sfx.push(SfxEvent::Jump);
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

        // Capture vertical speed before collision zeros it to detect real landings
        let pre_move_vel_y = self.vel.y;

        // Movement and collision with adaptive sub-stepping
        let dropping_down = input.is_pressed(Key::Down);
        self.move_and_collide(
            dt,
            dropping_down,
            solid_tiles,
            one_way_platforms,
            void_tiles,
            spike_tiles,
            crates,
        );

        if self.dead {
            return;
        }

        // Landing event
        let just_landed = !was_grounded && self.grounded;
        if just_landed {
            // Emit landing sound only after noticeable downward velocity
            if pre_move_vel_y > 30.0 {
                self.pending_sfx.push(SfxEvent::Land);
            }
            if self.is_bhopping && self.bhop_velocity.abs() > 200.0 {
                self.bhop_timer = 0.15;
            } else {
                self.bhop_timer = 0.0;
                self.is_bhopping = false;
                self.bhop_velocity = 0.0;
            }
        }

        // Reset ice jump bonus when on ground or hitting a horizontal obstacle
        if self.grounded || self.vel.x == 0.0 {
            self.ice_jump_active = false;
        }

        // Reset wall jump limit when on ground
        if self.grounded {
            self.last_wall_dir = None;
        }

        // Re-check ice under feet after position updates
        self.on_ice = self.check_on_ice(ice_tiles);

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

        // Reset animation timer and frame on state transition to prevent timing carryover
        if self.state != prev_state {
            self.anim_timer = 0.0;
            self.anim_frame = 0;
        }

        // Sprite animation frame update in real time
        let (_, frame_count, frame_duration) = self.state.anim_params();

        self.anim_timer += dt;
        while self.anim_timer >= frame_duration {
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

