//------------[Trigger rebuild when embedded assets directory changes]-------------------
fn main() {
    // Re-run build script and re-embed files whenever anything inside assets/ changes
    println!("cargo:rerun-if-changed=assets");
}
