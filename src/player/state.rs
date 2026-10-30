#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PlayerState {
    Idle,
    Walk,
    Jump,
    Fall,
    WallSlide,
    DashFreeze,
    Dash,
}

impl PlayerState {
    //------------[Get animation sprite parameters]-------------------
    pub fn anim_params(self) -> (usize, usize, f32) {
        match self {
            Self::Idle => (0, 2, 0.30),
            Self::Walk => (2, 4, 0.075),
            Self::Jump => (4, 2, 0.11),
            Self::Fall => (5, 2, 0.11),
            Self::WallSlide => (5, 2, 0.15),
            Self::DashFreeze => (0, 1, 0.22),
            Self::Dash => (2, 4, 0.045),
        }
    }
}

