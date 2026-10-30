use glam::Vec2;
use crate::physics::AABB;
use super::Player;

impl Player {
    //------------[Check hazard overlap]-------------------
    pub fn check_hazard_collision(&self, hazard_tiles: &[AABB]) -> bool {
        let box_hurt = self.hurtbox();
        hazard_tiles.iter().any(|h| box_hurt.overlaps(h))
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

