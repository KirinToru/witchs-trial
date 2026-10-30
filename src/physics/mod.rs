use glam::Vec2;

#[allow(clippy::upper_case_acronyms)]
#[derive(Debug, Clone, Copy)]
pub struct AABB {
    pub min: Vec2,
    pub max: Vec2,
}

impl AABB {
    //------------[Create AABB from min and max points]-------------------
    pub fn new(min: Vec2, max: Vec2) -> Self {
        Self { min, max }
    }

    //------------[Create AABB from position and size]-------------------
    pub fn from_pos_size(pos: Vec2, size: Vec2) -> Self {
        Self {
            min: pos,
            max: pos + size,
        }
    }

    //------------[Test AABB intersection]-------------------
    pub fn overlaps(&self, other: &AABB) -> bool {
        self.min.x < other.max.x
            && self.max.x > other.min.x
            && self.min.y < other.max.y
            && self.max.y > other.min.y
    }

    //------------[Test AABB proximity with tolerance]-------------------
    #[allow(dead_code)]
    pub fn touches(&self, other: &AABB, tolerance: f32) -> bool {
        self.min.x <= other.max.x + tolerance
            && self.max.x >= other.min.x - tolerance
            && self.min.y <= other.max.y + tolerance
            && self.max.y >= other.min.y - tolerance
    }
}

#[cfg(test)]
mod tests;
