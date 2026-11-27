use glam::Vec2;
use crate::map::{extract_note_objects, extract_tileset_mappings, TilesetKind};

//------------[Verify TMX Note Object Parsing for Rectangles and Tile Objects]-------------------
#[test]
fn test_parse_tmx_note_objects() {
    let sample_tmx = r#"<map version="1.10" width="20" height="15" tilewidth="32" tileheight="32">
        <tileset firstgid="1" source="../tilesets/ts_main.tsx"/>
        <tileset firstgid="65" source="../tilesets/ts_bricks.tsx"/>
        <objectgroup id="4" name="Notes">
            <object id="1" name="intro_note" class="note" x="128" y="256" width="32" height="32">
                <properties>
                    <property name="title" value="Controls"/>
                    <property name="text">Line 1&#10;Line 2 with &quot;quotes&quot; &amp; tips</property>
                </properties>
            </object>
            <object id="2" gid="65" x="320" y="192" width="32" height="32">
                <properties>
                    <property name="text" value="Watch out!\nSpikes ahead."/>
                </properties>
            </object>
            <object id="3" name="player_spawn" x="64" y="64" width="32" height="32"/>
        </objectgroup>
    </map>"#;

    let mappings = extract_tileset_mappings(sample_tmx);
    let notes = extract_note_objects(sample_tmx, 32.0, &mappings);

    assert_eq!(notes.len(), 2);

    // 1. Standard rectangle object preserves top-left coordinates and decodes multiline XML text
    assert_eq!(notes[0].pos, Vec2::new(128.0, 256.0));
    assert_eq!(notes[0].size, Vec2::new(32.0, 32.0));
    assert_eq!(notes[0].title, "Controls");
    assert_eq!(notes[0].text, "Line 1\nLine 2 with \"quotes\" & tips");
    assert!(notes[0].visual.is_none());

    // 2. Tile Object adjusts bottom-left Y by height and preserves its resolved TileVisual
    assert_eq!(notes[1].pos, Vec2::new(320.0, 160.0));
    assert_eq!(notes[1].size, Vec2::new(32.0, 32.0));
    assert_eq!(notes[1].title, "");
    assert_eq!(notes[1].text, "Watch out!\nSpikes ahead.");
    let visual = notes[1].visual.as_ref().expect("Tile Object should produce a visual");
    assert_eq!(visual.pos, Vec2::new(320.0, 160.0));
    assert_eq!(visual.tileset, TilesetKind::Bricks);
}
