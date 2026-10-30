use crate::map::extract_all_layers;

//------------[Verify Extraction of Arbitrary Multiple Tiled Layers in Order]-------------------
#[test]
fn test_extract_all_layers() {
    let sample_xml = r#"<map width="30" height="20">
        <layer id="2" name="Tile Layer 2"><data encoding="csv">65,65,0</data></layer>
        <layer id="1" name="Tile Layer 1"><data encoding="csv">9,0,1</data></layer>
    </map>"#;

    let layers = extract_all_layers(sample_xml);
    assert_eq!(layers.len(), 2);
    assert_eq!(layers[0].0, "Tile Layer 2");
    assert_eq!(layers[0].1, "65,65,0");
    assert_eq!(layers[1].0, "Tile Layer 1");
    assert_eq!(layers[1].1, "9,0,1");
}

