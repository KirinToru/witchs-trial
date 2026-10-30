# Contributing to Witch's Trial

Thank you for contributing to Witch's Trial. This project is a 2D platformer built with Rust, wgpu, and winit.

## Licensing

Witch's Trial is licensed under the GNU General Public License v3.0 (GNU GPL-3.0). By submitting contributions, you agree that your work will be distributed under these terms.

## Code Standards

1. Function Separators:
   Functions can be preceded by a standardized visual separator:
   `//------------[Function Description]-------------------`

2. Modularity:
   Keep files compact and focused. Place tests in dedicated test directories with one file per unit test.

3. Zero Warnings Policy:
   All code must compile cleanly using `cargo check`, `cargo test`, and `cargo clippy --all-targets -- -D warnings`.

4. Emoji and Dash Policy:
   No emojis are permitted in code, commits, or documentation. Use standard short hyphens (-) instead of dashes.

## Pull Request Process

1. Create a feature branch from main.
2. Verify code passes all checks:
   - `cargo test`
   - `cargo clippy --all-targets -- -D warnings`
3. Submit a concise pull request describing the changes.
