use crate::map::extract_layer_data;

//------------[Verify Layer Extraction with Fallback Layer Names]-------------------
#[test]
fn test_extract_layer_data_fallback() {
    let sample_xml = r#"<map>
        <layer id="3" name="Tile Layer 3"><data>1,2,3</data></layer>
        <layer id="2" name="Tile Layer 2"><data>51,51</data></layer>
    </map>"#;

    let visuals = extract_layer_data(sample_xml, "Visuals Layer")
        .or_else(|| extract_layer_data(sample_xml, "Tile Layer 3"));
    assert_eq!(visuals, Some("1,2,3".to_string()));

    let func1 = extract_layer_data(sample_xml, "Func Layer 1")
        .or_else(|| extract_layer_data(sample_xml, "Tile Layer 2"));
    assert_eq!(func1, Some("51,51".to_string()));
}

