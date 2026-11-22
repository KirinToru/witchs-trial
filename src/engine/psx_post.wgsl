//------------[PSX retro post-processing shader in WGSL]-------------------

// Configuration constants for tuning PSX aesthetics
const COLOR_STEPS: f32 = 31.0;
const DITHER_STRENGTH: f32 = 0.65;
const SCANLINE_STRENGTH: f32 = 0.07;
const VIGNETTE_STRENGTH: f32 = 0.18;

struct PsxUniforms {
    resolution: vec2<f32>,
    virtual_scale: f32,
    _pad: f32,
};

@group(0) @binding(0)
var<uniform> uniforms: PsxUniforms;

@group(0) @binding(1)
var t_scene: texture_2d<f32>;

@group(0) @binding(2)
var s_scene: sampler;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

//------------[Fullscreen triangle vertex shader]-------------------
@vertex
fn vs_post(@builtin(vertex_index) vertex_index: u32) -> VertexOutput {
    var pos = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>( 3.0, -1.0),
        vec2<f32>(-1.0,  3.0)
    );

    var out: VertexOutput;
    let p = pos[vertex_index];
    out.position = vec4<f32>(p, 0.0, 1.0);
    out.uv = vec2<f32>((p.x + 1.0) * 0.5, (1.0 - p.y) * 0.5);
    return out;
}

//------------[Bayer 4x4 matrix normalized to -0.5..+0.5]-------------------
fn get_bayer_4x4(x: u32, y: u32) -> f32 {
    let bayer = array<f32, 16>(
         0.0 / 16.0 - 0.5,  8.0 / 16.0 - 0.5,  2.0 / 16.0 - 0.5, 10.0 / 16.0 - 0.5,
        12.0 / 16.0 - 0.5,  4.0 / 16.0 - 0.5, 14.0 / 16.0 - 0.5,  6.0 / 16.0 - 0.5,
         3.0 / 16.0 - 0.5, 11.0 / 16.0 - 0.5,  1.0 / 16.0 - 0.5,  9.0 / 16.0 - 0.5,
        15.0 / 16.0 - 0.5,  7.0 / 16.0 - 0.5, 13.0 / 16.0 - 0.5,  5.0 / 16.0 - 0.5,
    );
    let idx = (y & 3u) * 4u + (x & 3u);
    return bayer[idx];
}

//------------[PSX fragment shader with 15-bit color quantization and Bayer dithering]-------------------
@fragment
fn fs_post(in: VertexOutput) -> @location(0) vec4<f32> {
    // 1:1 texture sampling for crisp text and platform edges
    let color = textureSample(t_scene, s_scene, in.uv);

    // Virtual pixel grid coordinates aligned to game resolution
    let vcoord = floor(in.position.xy / uniforms.virtual_scale);

    // 15-bit RGB555 color quantization (31 steps) with ordered Bayer dithering
    let bayer_val = get_bayer_4x4(u32(vcoord.x), u32(vcoord.y));
    let dither_offset = bayer_val * (DITHER_STRENGTH / COLOR_STEPS);
    let rgb_dithered = clamp(color.rgb + vec3<f32>(dither_offset), vec3<f32>(0.0), vec3<f32>(1.0));
    var quantized = floor(rgb_dithered * COLOR_STEPS + 0.5) / COLOR_STEPS;
    quantized = clamp(quantized, vec3<f32>(0.0), vec3<f32>(1.0));

    // Subtle soft scanlines on alternating screen rows
    let scanline = 1.0 - (f32(u32(in.position.y) & 1u) * SCANLINE_STRENGTH);
    quantized = quantized * scanline;

    // Gentle corner vignette
    let dist = length(in.uv - vec2<f32>(0.5, 0.5));
    let vignette = 1.0 - smoothstep(0.40, 0.75, dist) * VIGNETTE_STRENGTH;
    quantized = quantized * vignette;

    return vec4<f32>(quantized, color.a);
}
