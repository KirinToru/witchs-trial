use crate::map::merge_horizontal_colliders;

//------------[Verify Tiles with Gaps or in Different Rows Do Not Merge]-------------------
#[test]
fn test_merge_horizontal_colliders_gaps_and_rows() {
    let tile_size = 32.0;
    // Row 2: cols 0 and 1 contiguous, gap at 2, col 3 alone
    // Row 4: col 1 alone
    let tiles = vec![(0, 2), (1, 2), (3, 2), (1, 4)];

    let colliders = merge_horizontal_colliders(&tiles, tile_size);

    // Expect 3 separate colliders:
    // 1) row 2, cols 0..2 (width 64)
    // 2) row 2, col 3 (width 32)
    // 3) row 4, col 1 (width 32)
    assert_eq!(colliders.len(), 3);

    // Collider 1
    assert_eq!(colliders[0].min.x, 0.0);
    assert_eq!(colliders[0].min.y, 2.0 * tile_size);
    assert_eq!(colliders[0].max.x, 2.0 * tile_size);
    assert_eq!(colliders[0].max.y, 3.0 * tile_size);

    // Collider 2
    assert_eq!(colliders[1].min.x, 3.0 * tile_size);
    assert_eq!(colliders[1].min.y, 2.0 * tile_size);
    assert_eq!(colliders[1].max.x, 4.0 * tile_size);
    assert_eq!(colliders[1].max.y, 3.0 * tile_size);

    // Collider 3
    assert_eq!(colliders[2].min.x, 1.0 * tile_size);
    assert_eq!(colliders[2].min.y, 4.0 * tile_size);
    assert_eq!(colliders[2].max.x, 2.0 * tile_size);
    assert_eq!(colliders[2].max.y, 5.0 * tile_size);
}
