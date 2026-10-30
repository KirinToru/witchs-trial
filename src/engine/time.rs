use std::time::Instant;

pub struct TimeAccumulator {
    fixed_dt: f32,
    accumulator: f32,
    last_time: Instant,
}

impl TimeAccumulator {
    //------------[Create time accumulator]-------------------
    pub fn new(fixed_hz: f32) -> Self {
        Self {
            fixed_dt: 1.0 / fixed_hz,
            accumulator: 0.0,
            last_time: Instant::now(),
        }
    }

    //------------[Accumulate frame delta]-------------------
    pub fn update(&mut self) {
        let now = Instant::now();
        let elapsed = now.duration_since(self.last_time).as_secs_f32();
        self.last_time = now;
        self.accumulator += elapsed.min(0.1);
    }

    //------------[Consume fixed physics tick]-------------------
    pub fn step(&mut self) -> bool {
        if self.accumulator >= self.fixed_dt {
            self.accumulator -= self.fixed_dt;
            true
        } else {
            false
        }
    }

    //------------[Get fixed delta time]-------------------
    pub fn dt(&self) -> f32 {
        self.fixed_dt
    }

    //------------[Set simulation frequency]-------------------
    pub fn set_hz(&mut self, fixed_hz: f32) {
        if fixed_hz > 0.0 {
            self.fixed_dt = 1.0 / fixed_hz;
        }
    }

    //------------[Interpolation alpha factor]-------------------
    #[allow(dead_code)]
    pub fn alpha(&self) -> f32 {
        (self.accumulator / self.fixed_dt).clamp(0.0, 1.0)
    }
}

pub struct FrameLimiter {
    target_fps: Option<u32>,
}

impl Default for FrameLimiter {
    //------------[Default frame limiter]-------------------
    fn default() -> Self {
        Self::new()
    }
}

impl FrameLimiter {
    //------------[Create frame limiter]-------------------
    pub fn new() -> Self {
        Self { target_fps: None }
    }

    //------------[Set target FPS]-------------------
    pub fn set_target_fps(&mut self, target_fps: Option<u32>) {
        self.target_fps = target_fps;
    }

    //------------[Wait for remaining frame duration]-------------------
    pub fn wait_remaining(&self, frame_start: Instant) {
        if let Some(fps) = self.target_fps {
            let target_duration = std::time::Duration::from_secs_f64(1.0 / fps as f64);
            let elapsed = frame_start.elapsed();
            if elapsed < target_duration {
                let sleep_time = target_duration - elapsed;
                if sleep_time > std::time::Duration::from_millis(3) {
                    std::thread::sleep(sleep_time - std::time::Duration::from_millis(2));
                }
                while frame_start.elapsed() < target_duration {
                    std::hint::spin_loop();
                }
            }
        }
    }
}
