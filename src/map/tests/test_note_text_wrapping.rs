use crate::map::wrap_note_text;

//------------[Verify Note Text Wrapping Preserves Newlines and Line Limits]-------------------
#[test]
fn test_note_text_wrapping() {
    let raw = "Hold SPACE longer to jump higher.\n\nPress SHIFT to dash across wide gaps safely.";
    let max_chars = 18;
    let lines = wrap_note_text(raw, max_chars);

    // Blank paragraph line from "\n\n" must stay intact
    assert!(lines.contains(&String::new()));
    assert!(lines.len() >= 5);

    for line in &lines {
        assert!(
            line.chars().count() <= max_chars,
            "Line '{}' exceeded max width {}",
            line,
            max_chars
        );
    }

    // Rejoining non-empty lines restores all original words in exact order
    let normalized_input: String = raw.split_whitespace().collect::<Vec<_>>().join(" ");
    let normalized_output: String = lines
        .iter()
        .filter(|l| !l.is_empty())
        .flat_map(|l| l.split_whitespace())
        .collect::<Vec<_>>()
        .join(" ");
    assert_eq!(normalized_output, normalized_input);

    // Extra-long token without spaces is split safely within limit
    let long_token_lines = wrap_note_text("Supercalifragilistic", 8);
    assert_eq!(long_token_lines, vec!["Supercal", "ifragili", "stic"]);
}
