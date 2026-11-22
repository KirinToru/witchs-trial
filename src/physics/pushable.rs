//! Pushable crate entity with 2D physics, gravity, friction, and sub-stepping.

use glam::Vec2;
use crate::physics::AABB;

// Physics constants for pushable crates
pub const BOX_SIZE: f32 = 32.0;
pub const BOX_GRAVITY: f32 = 1200.0;
pub const BOX_MAX_FALL_SPEED: f32 = 720.0;
pub const BOX_GROUND_FRICTION: f32 = 500.0;
pub const BOX_ICE_FRICTION: f32 = 110.0;
pub const BOX_MAX_SPEED_X: f32 = 650.0;
pub const BOX_DASH_IMPULSE_WITCH: f32 = 420.0;
pub const BOX_DASH_IMPULSE_BEAST: f32 = 620.0;
pub const PUSH_SPEED_MULT_WITCH: f32 = 0.55;
pub const PUSH_SPEED_MULT_BEAST: f32 = 0.85;

//------------[Move scalar value towards target at linear rate]-------------------
fn move_towards(current: f32, target: f32, max_delta: f32) -> f32 {
    if (target - current).abs() <= max_delta {
        target
    } else {
        current + (target - current).signum() * max_delta
    }
}

#[derive(Debug, Clone)]
pub struct PushableBox {
    pub pos: Vec2,
    pub prev_pos: Vec2,
    pub spawn_pos: Vec2,
    pub vel: Vec2,
    pub size: Vec2,
    pub grounded: bool,
    pub on_ice: bool,
    pub active: bool,
}

impl PushableBox {
    //------------[Create new pushable box at spawn position]-------------------
    pub fn new(spawn_pos: Vec2) -> Self {
        Self {
            pos: spawn_pos,
            prev_pos: spawn_pos,
            spawn_pos,
            vel: Vec2::ZERO,
            size: Vec2::splat(BOX_SIZE),
            grounded: false,
            on_ice: false,
            active: true,
        }
    }

    //------------[Reset box to initial spawn state]-------------------
    pub fn reset(&mut self) {
        self.pos = self.spawn_pos;
        self.prev_pos = self.spawn_pos;
        self.vel = Vec2::ZERO;
        self.grounded = false;
        self.on_ice = false;
        self.active = true;
    }

    //------------[Get bounding box AABB]-------------------
    pub fn aabb(&self) -> AABB {
        AABB::from_pos_size(self.pos, self.size)
    }

    //------------[Check if box is standing on ice surface]-------------------
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

    //------------[Update single box physics step]-------------------
    pub fn update_step(
        &mut self,
        dt: f32,
        solid_tiles: &[AABB],
        one_way_platforms: &[AABB],
        ice_tiles: &[AABB],
        void_tiles: &[AABB],
        spike_tiles: &[AABB],
        other_boxes: &[AABB],
        map_bottom: f32,
    ) {
        if !self.active {
            return;
        }

        // Store previous position for sub-frame interpolation
        self.prev_pos = self.pos;

        // Check if already in void or below map
        if self.pos.y > map_bottom || void_tiles.iter().any(|v| self.aabb().overlaps(v)) {
            self.active = false;
            return;
        }

        // Detect ice surface under feet before movement logic
        self.on_ice = self.check_on_ice(ice_tiles);

        // Apply gravity
        self.vel.y = (self.vel.y + BOX_GRAVITY * dt).min(BOX_MAX_FALL_SPEED);

        // Ground friction
        if self.grounded {
            let friction = if self.on_ice {
                BOX_ICE_FRICTION
            } else {
                BOX_GROUND_FRICTION
            };
            self.vel.x = move_towards(self.vel.x, 0.0, friction * dt);
        }

        // Clamp horizontal velocity
        self.vel.x = self.vel.x.clamp(-BOX_MAX_SPEED_X, BOX_MAX_SPEED_X);

        // Adaptive sub-stepping (8.0 px safe threshold)
        let max_displacement = (self.vel.x.abs() * dt).max(self.vel.y.abs() * dt);
        let steps = ((max_displacement / 8.0).ceil() as usize).max(1);
        let sub_dt = dt / steps as f32;

        for _ in 0..steps {
            // Check void before sub-step move
            if self.pos.y > map_bottom || void_tiles.iter().any(|v| self.aabb().overlaps(v)) {
                self.active = false;
                return;
            }

            // Resolve horizontal movement
            if self.vel.x != 0.0 {
                self.pos.x += self.vel.x * sub_dt;
                let mut box_aabb = self.aabb();

                // Solid walls
                for wall in solid_tiles {
                    if box_aabb.overlaps(wall) {
                        if self.vel.x > 0.0 {
                            self.pos.x = wall.min.x - self.size.x;
                        } else if self.vel.x < 0.0 {
                            self.pos.x = wall.max.x;
                        }
                        self.vel.x = 0.0;
                        box_aabb = self.aabb();
                    }
                }

                // Other active crates
                for other in other_boxes {
                    if box_aabb.overlaps(other) {
                        if self.vel.x > 0.0 {
                            self.pos.x = other.min.x - self.size.x;
                        } else if self.vel.x < 0.0 {
                            self.pos.x = other.max.x;
                        }
                        self.vel.x = 0.0;
                        box_aabb = self.aabb();
                    }
                }
            }

            // Resolve vertical movement
            let was_grounded = self.grounded;
            self.pos.y += self.vel.y * sub_dt;
            self.grounded = false;
            let mut box_aabb = self.aabb();

            // Solid floor/ceiling
            for wall in solid_tiles {
                if box_aabb.overlaps(wall) {
                    if self.vel.y > 0.0 {
                        self.pos.y = wall.min.y - self.size.y;
                        self.vel.y = 0.0;
                        self.grounded = true;
                    } else if self.vel.y < 0.0 {
                        self.pos.y = wall.max.y;
                        self.vel.y = 0.0;
                    }
                    box_aabb = self.aabb();
                }
            }

            // Spike colliders: box is immune and safely lands on top of spikes
            for spike in spike_tiles {
                if box_aabb.overlaps(spike) {
                    if self.vel.y > 0.0 {
                        self.pos.y = spike.min.y - self.size.y;
                        self.vel.y = 0.0;
                        self.grounded = true;
                    } else if self.vel.y < 0.0 {
                        self.pos.y = spike.max.y;
                        self.vel.y = 0.0;
                    }
                    box_aabb = self.aabb();
                }
            }

            // One-way platforms: land from above
            if self.vel.y >= 0.0 {
                let prev_bottom = self.pos.y + self.size.y - self.vel.y * sub_dt;
                let current_bottom = self.pos.y + self.size.y;
                for plat in one_way_platforms {
                    if prev_bottom <= plat.min.y + 4.0 && current_bottom >= plat.min.y {
                        let plat_horizontal = AABB::from_pos_size(
                            Vec2::new(plat.min.x, plat.min.y),
                            Vec2::new(plat.max.x - plat.min.x, 8.0),
                        );
                        if box_aabb.overlaps(&plat_horizontal) {
                            self.pos.y = plat.min.y - self.size.y;
                            self.vel.y = 0.0;
                            self.grounded = true;
                            box_aabb = self.aabb();
                            break;
                        }
                    }
                }
            }

            // Other active crates: can stack and land on top
            for other in other_boxes {
                if box_aabb.overlaps(other) {
                    if self.vel.y > 0.0 {
                        self.pos.y = other.min.y - self.size.y;
                        self.vel.y = 0.0;
                        self.grounded = true;
                    } else if self.vel.y < 0.0 {
                        self.pos.y = other.max.y;
                        self.vel.y = 0.0;
                    }
                    box_aabb = self.aabb();
                }
            }

            // Verify continuous ground support when stationary
            if !self.grounded && was_grounded && self.vel.y == 0.0 {
                let ground_probe = AABB::from_pos_size(
                    Vec2::new(self.pos.x + 2.0, self.pos.y + self.size.y),
                    Vec2::new(self.size.x - 4.0, 1.0),
                );
                let on_solid = solid_tiles.iter().any(|w| ground_probe.overlaps(w));
                let on_spike = spike_tiles.iter().any(|s| ground_probe.overlaps(s));
                let on_plat = one_way_platforms.iter().any(|p| ground_probe.overlaps(p));
                let on_box = other_boxes.iter().any(|o| ground_probe.overlaps(o));
                if on_solid || on_spike || on_plat || on_box {
                    self.grounded = true;
                }
            }

            // Check void after sub-step move
            if self.pos.y > map_bottom || void_tiles.iter().any(|v| self.aabb().overlaps(v)) {
                self.active = false;
                return;
            }
        }

        // Re-check ice under feet after position updates
        self.on_ice = self.check_on_ice(ice_tiles);
    }

    //------------[Update all active pushable crates]-------------------
    pub fn update_all(
        crates: &mut [PushableBox],
        dt: f32,
        solid_tiles: &[AABB],
        one_way_platforms: &[AABB],
        ice_tiles: &[AABB],
        void_tiles: &[AABB],
        spike_tiles: &[AABB],
        map_bottom: f32,
    ) {
        let count = crates.len();
        for i in 0..count {
            if !crates[i].active {
                continue;
            }

            // Collect other active crates bounding boxes
            let other_boxes: Vec<AABB> = crates
                .iter()
                .enumerate()
                .filter(|&(j, c)| j != i && c.active)
                .map(|(_, c)| c.aabb())
                .collect();

            crates[i].update_step(
                dt,
                solid_tiles,
                one_way_platforms,
                ice_tiles,
                void_tiles,
                spike_tiles,
                &other_boxes,
                map_bottom,
            );
        }
    }
}
