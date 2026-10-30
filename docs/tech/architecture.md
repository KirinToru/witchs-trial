# Witch's Trial Technical and API Documentation

## Automated API Documentation

Witch's Trial utilizes Rust's native automated documentation tool, `rustdoc` (the Rust ecosystem standard equivalent to Doxygen and Swagger/OpenAPI). All modules, structs, enums, functions, and internal methods are parsed directly from source code annotations and type signatures to ensure documentation is always in sync with implementation.

### Generating Interactive HTML Documentation

To build and open the interactive documentation website in your default web browser, run:

```bash
cargo doc --document-private-items --no-deps --open
```

The generated documentation is located at `target/doc/witchs_trial/index.html` and provides:

- Live client-side full-text search across all symbols, methods, and constants.
- Module hierarchy navigation (`engine`, `game`, `map`, `menu`, `physics`, `player`).
- Type signatures, trait implementations, visibility levels, and default values.
- Direct links to the underlying source code for every item.
