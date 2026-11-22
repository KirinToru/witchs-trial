//------------[Voxel post-processing shader in WGSL]-------------------

struct VoxelUniforms {
    resolution: vec2<f32>,
    voxel_size: f32,
    _pad: f32,
};

@group(0) @binding(0)
var<uniform> uniforms: VoxelUniforms;

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

//------------[Voxel pixel fragment shader]-------------------
@fragment
fn fs_post(in: VertexOutput) -> @location(0) vec4<f32> {
    let screen_coord = in.uv * uniforms.resolution;
    let cell_size = uniforms.voxel_size;

    // Grid coordinates
    let cell_coord = floor(screen_coord / cell_size);
    let cell_center_px = (cell_coord + vec2<f32>(0.5, 0.5)) * cell_size;
    let cell_center_uv = cell_center_px / uniforms.resolution;

    // Sample color from voxel center
    var color = textureSample(t_scene, s_scene, cell_center_uv);

    // Color quantization (32 levels per channel for dark fantasy aesthetic)
    let quant_levels = 32.0;
    color = vec4<f32>(floor(color.rgb * quant_levels + 0.5) / quant_levels, color.a);

    // Local coordinates within voxel cell (0.0 .. 1.0)
    let local_pos = fract(screen_coord / cell_size);
    let bevel_thick = clamp(1.0 / cell_size, 0.20, 0.35);

    // 2.5D facet lighting: top/left brightened (+0.07), bottom/right darkened (-0.09)
    let is_top_or_left = local_pos.x < bevel_thick || local_pos.y < bevel_thick;
    let is_bottom_or_right = local_pos.x > (1.0 - bevel_thick) || local_pos.y > (1.0 - bevel_thick);
    var bevel = 0.0;
    if (is_top_or_left && !is_bottom_or_right) {
        bevel = 0.07;
    } else if (is_bottom_or_right && !is_top_or_left) {
        bevel = -0.09;
    }

    // Edge contrast enhancement against neighboring voxels
    let left_uv = ((cell_coord + vec2<f32>(-0.5, 0.5)) * cell_size) / uniforms.resolution;
    let top_uv = ((cell_coord + vec2<f32>(0.5, -0.5)) * cell_size) / uniforms.resolution;
    let c_left = textureSample(t_scene, s_scene, left_uv).rgb;
    let c_top = textureSample(t_scene, s_scene, top_uv).rgb;

    let lum_curr = dot(color.rgb, vec3<f32>(0.299, 0.587, 0.114));
    let lum_left = dot(c_left, vec3<f32>(0.299, 0.587, 0.114));
    let lum_top = dot(c_top, vec3<f32>(0.299, 0.587, 0.114));
    let edge_diff = clamp(abs(lum_curr - lum_left) + abs(lum_curr - lum_top), 0.0, 1.0);
    let edge_boost = edge_diff * 0.04;

    color = vec4<f32>(clamp(color.rgb + vec3<f32>(bevel + edge_boost), vec3<f32>(0.0), vec3<f32>(1.0)), color.a);

    // Subtle vignette around screen borders
    let dist = distance(in.uv, vec2<f32>(0.5, 0.5));
    let vignette = smoothstep(0.95, 0.35, dist);
    color = vec4<f32>(color.rgb * mix(1.0, vignette, 0.22), color.a);

    return color;
}
