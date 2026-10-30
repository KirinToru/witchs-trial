use crate::map::{extract_tileset_mappings, resolve_tile, TilesetKind};

//------------[Verify Dynamic Tileset Mapping and Local GID Resolution]-------------------
#[test]
fn test_dynamic_tileset_resolution() {
    let xml = r#"<map>
        <tileset firstgid="1" source="../tilesets/ts_main.tsx"/>
        <tileset firstgid="65" source="../tilesets/ts_bricks.tsx"/>
        <tileset firstgid="71" source="../tilesets/ts_prototype.tsx"/>
    </map>"#;

    let mappings = extract_tileset_mappings(xml);
    assert_eq!(mappings.len(), 3);

    // GID 1 -> Main, local 0
    let (m1, l1) = resolve_tile(1, &mappings).expect("Should resolve GID 1");
    assert_eq!(m1.kind, TilesetKind::Main);
    assert_eq!(l1, 0);

    // GID 9 -> Main, local 8 (SolidWall)
    let (m9, l9) = resolve_tile(9, &mappings).expect("Should resolve GID 9");
    assert_eq!(m9.kind, TilesetKind::Main);
    assert_eq!(l9, 8);

    // GID 65 -> Bricks, local 0
    let (m65, l65) = resolve_tile(65, &mappings).expect("Should resolve GID 65");
    assert_eq!(m65.kind, TilesetKind::Bricks);
    assert_eq!(l65, 0);

    // GID 94 -> Prototype, local 23 (94 - 71 = 23)
    let (m94, l94) = resolve_tile(94, &mappings).expect("Should resolve GID 94");
    assert_eq!(m94.kind, TilesetKind::Prototype);
    assert_eq!(l94, 23);
}

