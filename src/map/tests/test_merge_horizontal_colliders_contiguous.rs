use crate::map::merge_horizontal_colliders;

//------------[Verify Contiguous Horizontal Tiles Merge into Single Wide AABB]-------------------
#[test]
fn test_merge_horizontal_colliders_contiguous() {
    let tile_size = 32.0;
    // Four contiguous tiles in row 5: cols 2, 3, 4, 5
    let tiles = vec![(2, 5), (3, 5), (4, 5), (5, 5)];

    let colliders = merge_horizontal_colliders(&tiles, tile_size);

    // Must produce exactly one merged AABB
    assert_eq!(colliders.len(), 1);
    let aabb = &colliders[0];
    assert_eq!(aabb.min.x, 2.0 * tile_size);
    assert_eq!(aabb.min.y, 5.0 * tile_size);
    assert_eq!(aabb.max.x, 6.0 * tile_size); // 2 + 4 tiles = 6
    assert_eq!(aabb.max.y, 6.0 * tile_size);
}
