use crate::engine::assets;

//------------[Verify Embedded Assets Load Bytes and Map Listing Includes Builtin Maps]-------------------
#[test]
fn test_embedded_assets_available() {
    // 1. Verify embedded PNG sprite sheet loads non-empty bytes with valid PNG header
    let sprite_bytes = assets::load_bytes("assets/player/spritesheet.png")
        .expect("spritesheet.png must load from disk or embedded bundle");
    assert!(!sprite_bytes.is_empty());
    assert_eq!(&sprite_bytes[0..4], &[0x89, b'P', b'N', b'G']);

    // 2. Verify map list includes known bundled TMX files
    let maps = assets::list_maps();
    assert!(maps.contains(&"test_func.tmx".to_string()));
}
