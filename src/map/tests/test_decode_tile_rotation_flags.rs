use crate::map::decode_tile_gid;
use glam::Vec2;

//------------[Verify Tiled Rotation and Flip Flag Decoding]-------------------
#[test]
fn test_decode_tile_rotation_flags() {
    // Plain tile 3
    let (gid, rot, scale) = decode_tile_gid(3);
    assert_eq!(gid, 3);
    assert_eq!(rot, 0.0);
    assert_eq!(scale, Vec2::new(1.0, 1.0));

    // Vertical flip (0x40000003)
    let (gid_v, rot_v, scale_v) = decode_tile_gid(0x4000_0003);
    assert_eq!(gid_v, 3);
    assert_eq!(rot_v, 0.0);
    assert_eq!(scale_v, Vec2::new(1.0, -1.0));

    // 180 degree rotation (Horizontal + Vertical flip: 0xC0000002)
    let (gid_180, rot_180, scale_180) = decode_tile_gid(0xC000_0002);
    assert_eq!(gid_180, 2);
    assert_eq!(rot_180, 180.0);
    assert_eq!(scale_180, Vec2::new(1.0, 1.0));

    // 90 degree clockwise rotation (Diagonal + Horizontal flip: 0xA0000005)
    let (gid_90, rot_90, scale_90) = decode_tile_gid(0xA000_0005);
    assert_eq!(gid_90, 5);
    assert_eq!(rot_90, 90.0);
    assert_eq!(scale_90, Vec2::new(1.0, 1.0));
}

