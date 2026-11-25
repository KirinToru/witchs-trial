use std::thread;
use std::time::{Duration, Instant};
use crate::engine::TimeAccumulator;

//------------[Verify Reset Accumulator Clears Backlog Alpha and Step Queue]-------------------
#[test]
fn test_time_accumulator_reset() {
    // 120 Hz gives an 8.33 ms fixed physics step
    let mut time = TimeAccumulator::new(120.0);

    // Wait long enough to queue multiple physics steps
    let start = Instant::now();
    while start.elapsed() < Duration::from_millis(30) {
        thread::sleep(Duration::from_millis(5));
    }
    time.update();

    // Confirm time accumulated and at least one step is queued with leftover time
    assert!(time.alpha() > 0.0);
    assert!(time.step());
    assert!(time.alpha() > 0.0);

    // Resetting accumulator must clear queued time while preserving step duration
    time.reset_accumulator();
    assert_eq!(time.dt(), 1.0 / 120.0);
    assert_eq!(time.alpha(), 0.0);
    assert!(!time.step());
}
