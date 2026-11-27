//! Hybrid asset loader preferring local disk overrides with embedded binary fallback.

use std::collections::BTreeSet;
use std::path::Path;
use include_dir::{include_dir, Dir};

static EMBEDDED_ASSETS: Dir<'_> = include_dir!("$CARGO_MANIFEST_DIR/assets");

//------------[Normalize asset path relative to embedded assets root]-------------------
fn normalize_embedded_path(path: &str) -> String {
    let unified = path.replace('\\', "/");
    let trimmed = unified.trim_start_matches("./").trim_start_matches('/');
    if let Some(stripped) = trimmed.strip_prefix("assets/") {
        stripped.to_string()
    } else {
        trimmed.to_string()
    }
}

//------------[Collect TMX filenames from an optional disk directory]-------------------
fn collect_disk_maps(dir: &Path, out: &mut BTreeSet<String>) {
    if let Ok(entries) = std::fs::read_dir(dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.is_file() {
                if let Some(ext) = path.extension() {
                    if ext.to_string_lossy().eq_ignore_ascii_case("tmx") {
                        if let Some(filename) = path.file_name() {
                            out.insert(filename.to_string_lossy().to_string());
                        }
                    }
                }
            }
        }
    }
}

//------------[Load asset bytes from disk override or embedded binary]-------------------
pub fn load_bytes(path: &str) -> Result<Vec<u8>, String> {
    // 1. Check disk relative to current working directory
    let cwd_candidate = Path::new(path);
    if cwd_candidate.is_file() {
        return std::fs::read(cwd_candidate)
            .map_err(|e| format!("Failed to read asset {}: {}", cwd_candidate.display(), e));
    }

    // 2. Check disk relative to executable directory for external overrides
    if let Ok(exe_path) = std::env::current_exe() {
        if let Some(exe_dir) = exe_path.parent() {
            let exe_candidate = exe_dir.join(path);
            if exe_candidate.is_file() {
                return std::fs::read(&exe_candidate).map_err(|e| {
                    format!("Failed to read asset {}: {}", exe_candidate.display(), e)
                });
            }
        }
    }

    // 3. Fall back to embedded binary asset bundle
    let rel_path = normalize_embedded_path(path);
    if let Some(file) = EMBEDDED_ASSETS.get_file(&rel_path) {
        return Ok(file.contents().to_vec());
    }

    Err(format!(
        "Asset not found on disk or in embedded bundle: {}",
        path
    ))
}

//------------[Load UTF-8 asset text from disk override or embedded binary]-------------------
pub fn load_string(path: &str) -> Result<String, String> {
    let bytes = load_bytes(path)?;
    String::from_utf8(bytes).map_err(|e| format!("Invalid UTF-8 in asset {}: {}", path, e))
}

//------------[List sorted TMX map filenames merged from disk and embedded bundle]-------------------
pub fn list_maps() -> Vec<String> {
    let mut maps = BTreeSet::new();

    // 1. Disk maps relative to working directory and executable directory
    collect_disk_maps(Path::new("assets/maps"), &mut maps);
    if let Ok(exe_path) = std::env::current_exe() {
        if let Some(exe_dir) = exe_path.parent() {
            collect_disk_maps(&exe_dir.join("assets/maps"), &mut maps);
        }
    }

    // 2. Embedded TMX maps compiled into the executable
    if let Some(dir) = EMBEDDED_ASSETS.get_dir("maps") {
        for file in dir.files() {
            if let Some(ext) = file.path().extension() {
                if ext.to_string_lossy().eq_ignore_ascii_case("tmx") {
                    if let Some(filename) = file.path().file_name() {
                        maps.insert(filename.to_string_lossy().to_string());
                    }
                }
            }
        }
    }

    maps.into_iter().collect()
}
