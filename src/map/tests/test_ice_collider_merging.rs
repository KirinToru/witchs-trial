use crate::map::merge_horizontal_colliders;

//------------[Verify Ice Tiles Merge Horizontally into Continuous AABB Colliders]-------------------
#[test]
fn test_ice_collider_merging() {
    let tile_size = 32.0;

    // Simulate mixed row: stone tiles at cols 0..2, ice tiles at cols 2..5
    let ice_tiles = vec![(2, 10), (3, 10), (4, 10)];
    let stone_tiles = vec![(0, 10), (1, 10)];

    // In map loading:
    // solid_tile_coords gets both stone and ice (to prevent snagging seams)
    let mut solid_tile_coords = stone_tiles.clone();
    solid_tile_coords.extend_from_slice(&ice_tiles);

    let solid_colliders = merge_horizontal_colliders(&solid_tile_coords, tile_size);
    let ice_colliders = merge_horizontal_colliders(&ice_tiles, tile_size);

    // Solid colliders merge across the entire 5 tiles (0 to 5)
    assert_eq!(solid_colliders.len(), 1);
    assert_eq!(solid_colliders[0].min.x, 0.0);
    assert_eq!(solid_colliders[0].max.x, 5.0 * tile_size);

    // Ice colliders merge specifically for cols 2 to 5
    assert_eq!(ice_colliders.len(), 1);
    assert_eq!(ice_colliders[0].min.x, 2.0 * tile_size);
    assert_eq!(ice_colliders[0].max.x, 5.0 * tile_size);
    assert_eq!(ice_colliders[0].min.y, 10.0 * tile_size);
    assert_eq!(ice_colliders[0].max.y, 11.0 * tile_size);
}
