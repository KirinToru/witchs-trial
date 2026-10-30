use crate::player::move_towards;

//------------[Verify Move Towards Numerical Stepping Accuracy]-------------------
#[test]
fn test_move_towards() {
    assert_eq!(move_towards(0.0, 100.0, 25.0), 25.0);
    assert_eq!(move_towards(90.0, 100.0, 25.0), 100.0);
    assert_eq!(move_towards(50.0, 0.0, 20.0), 30.0);
    assert_eq!(move_towards(10.0, 0.0, 20.0), 0.0);
}

