use glam::Vec2;
use std::sync::Arc;
use winit::window::Window;
use crate::engine::Texture;

#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Vertex {
    pub position: [f32; 2],
    pub uv: [f32; 2],
    pub color: [f32; 4],
}

impl Vertex {
    //------------[Vertex buffer layout]-------------------
    pub fn desc() -> wgpu::VertexBufferLayout<'static> {
        wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<Vertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &[
                wgpu::VertexAttribute {
                    offset: 0,
                    shader_location: 0,
                    format: wgpu::VertexFormat::Float32x2,
                },
                wgpu::VertexAttribute {
                    offset: std::mem::size_of::<[f32; 2]>() as wgpu::BufferAddress,
                    shader_location: 1,
                    format: wgpu::VertexFormat::Float32x2,
                },
                wgpu::VertexAttribute {
                    offset: (std::mem::size_of::<[f32; 2]>() + std::mem::size_of::<[f32; 2]>()) as wgpu::BufferAddress,
                    shader_location: 2,
                    format: wgpu::VertexFormat::Float32x4,
                },
            ],
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone, Debug, bytemuck::Pod, bytemuck::Zeroable)]
pub struct CameraUniform {
    pub view_proj: [f32; 16],
}

struct DrawBatch {
    texture_id: usize,
    start_vertex: u32,
    count: u32,
    bind_group: wgpu::BindGroup,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GraphicsBackendOption {
    Auto,
    Vulkan,
    DirectX12,
    OpenGL,
    Metal,
}

impl GraphicsBackendOption {
    //------------[Convert to wgpu backends]-------------------
    pub fn to_wgpu(self) -> wgpu::Backends {
        match self {
            Self::Auto => wgpu::Backends::all(),
            Self::Vulkan => wgpu::Backends::VULKAN,
            Self::DirectX12 => wgpu::Backends::DX12,
            Self::OpenGL => wgpu::Backends::GL,
            Self::Metal => wgpu::Backends::METAL,
        }
    }

    //------------[Backend display label]-------------------
    pub fn label(self) -> &'static str {
        match self {
            Self::Auto => "Auto",
            Self::Vulkan => "Vulkan",
            Self::DirectX12 => "DirectX 12",
            Self::OpenGL => "OpenGL",
            Self::Metal => "Metal",
        }
    }

    //------------[Platform supported backends]-------------------
    pub fn available_backends() -> &'static [GraphicsBackendOption] {
        #[cfg(target_os = "windows")]
        {
            &[
                GraphicsBackendOption::Auto,
                GraphicsBackendOption::DirectX12,
                GraphicsBackendOption::Vulkan,
                GraphicsBackendOption::OpenGL,
            ]
        }
        #[cfg(target_os = "macos")]
        {
            &[
                GraphicsBackendOption::Auto,
                GraphicsBackendOption::Metal,
            ]
        }
        #[cfg(not(any(target_os = "windows", target_os = "macos")))]
        {
            &[
                GraphicsBackendOption::Auto,
                GraphicsBackendOption::Vulkan,
                GraphicsBackendOption::OpenGL,
            ]
        }
    }

    //------------[Cycle next backend]-------------------
    pub fn next(self) -> Self {
        let available = Self::available_backends();
        if let Some(pos) = available.iter().position(|&b| b == self) {
            available[(pos + 1) % available.len()]
        } else {
            available[0]
        }
    }

    //------------[Cycle previous backend]-------------------
    pub fn prev(self) -> Self {
        let available = Self::available_backends();
        if let Some(pos) = available.iter().position(|&b| b == self) {
            if pos == 0 {
                available[available.len() - 1]
            } else {
                available[pos - 1]
            }
        } else {
            available[0]
        }
    }
}

#[allow(dead_code)]
pub struct WgpuRenderer {
    pub window: Arc<Window>,
    pub device: wgpu::Device,
    pub queue: wgpu::Queue,
    pub surface: wgpu::Surface<'static>,
    pub surface_config: wgpu::SurfaceConfiguration,
    pub texture_bind_group_layout: wgpu::BindGroupLayout,
    pub active_backend_name: String,
    pub supported_present_modes: Vec<wgpu::PresentMode>,
    camera_buffer: wgpu::Buffer,
    camera_bind_group: wgpu::BindGroup,
    render_pipeline: wgpu::RenderPipeline,
    vertex_buffer: wgpu::Buffer,
    vertex_buffer_capacity: usize,
    vertices: Vec<Vertex>,
    batches: Vec<DrawBatch>,
    pub default_white: Texture,
    pub debug_font_texture: Texture,
    active_camera_pos: Vec2,
    active_viewport_size: Vec2,
}

impl WgpuRenderer {
    //------------[Create renderer with auto backend]-------------------
    #[allow(dead_code)]
    pub fn new(window: Arc<Window>) -> Self {
        Self::new_with_backend(window, GraphicsBackendOption::Auto)
    }

    //------------[Create renderer with chosen backend]-------------------
    pub fn new_with_backend(window: Arc<Window>, backend_opt: GraphicsBackendOption) -> Self {
        let size = window.inner_size();
        let width = size.width.max(1);
        let height = size.height.max(1);

        let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
            backends: backend_opt.to_wgpu(),
            flags: wgpu::InstanceFlags::default(),
            backend_options: wgpu::BackendOptions::default(),
            display: None,
            memory_budget_thresholds: Default::default(),
        });
        let surface = instance
            .create_surface(window.clone())
            .expect("Failed to create wgpu surface");

        let adapter = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::HighPerformance,
            compatible_surface: Some(&surface),
            force_fallback_adapter: false,
            apply_limit_buckets: false,
        }))
        .expect("Failed to find supported graphics adapter");

        let adapter_info = adapter.get_info();
        let active_backend_name = format!("{:?}", adapter_info.backend);

        let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor {
            label: Some("engine_device"),
            required_features: wgpu::Features::empty(),
            required_limits: wgpu::Limits::default(),
            memory_hints: wgpu::MemoryHints::default(),
            trace: wgpu::Trace::Off,
            experimental_features: Default::default(),
        }))
        .expect("Failed to create wgpu device and command queue");

        let surface_caps = surface.get_capabilities(&adapter);
        let surface_format = surface_caps
            .formats
            .iter()
            .copied()
            .find(|f| f.is_srgb())
            .unwrap_or(surface_caps.formats[0]);

        let surface_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: surface_format,
            color_space: Default::default(),
            width,
            height,
            present_mode: wgpu::PresentMode::AutoVsync,
            alpha_mode: surface_caps.alpha_modes[0],
            view_formats: vec![],
            desired_maximum_frame_latency: 2,
        };
        surface.configure(&device, &surface_config);

        // Camera Uniform & Bind Group (Group 0)
        let camera_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("camera_uniform_buffer"),
            size: std::mem::size_of::<CameraUniform>() as wgpu::BufferAddress,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let camera_bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("camera_bind_group_layout"),
            entries: &[wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::VERTEX,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Uniform,
                    has_dynamic_offset: false,
                    min_binding_size: None,
                },
                count: None,
            }],
        });

        let camera_bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("camera_bind_group"),
            layout: &camera_bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: camera_buffer.as_entire_binding(),
            }],
        });

        // Texture Bind Group Layout (Group 1)
        let texture_bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("texture_bind_group_layout"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
            ],
        });

        let default_white = Texture::white_1x1(&device, &queue, &texture_bind_group_layout);
        let font_rgba = crate::engine::debug_font::create_font_bitmap_rgba();
        let debug_font_texture = Texture::from_rgba(
            &device,
            &queue,
            128,
            64,
            &font_rgba,
            &texture_bind_group_layout,
        );

        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("engine_2d_shader"),
            source: wgpu::ShaderSource::Wgsl(std::borrow::Cow::Borrowed(
                r#"
                struct CameraUniform {
                    view_proj: mat4x4<f32>,
                };

                @group(0) @binding(0)
                var<uniform> camera: CameraUniform;

                @group(1) @binding(0)
                var t_diffuse: texture_2d<f32>;
                @group(1) @binding(1)
                var s_diffuse: sampler;

                struct VertexInput {
                    @location(0) position: vec2<f32>,
                    @location(1) uv: vec2<f32>,
                    @location(2) color: vec4<f32>,
                };

                struct VertexOutput {
                    @builtin(position) clip_position: vec4<f32>,
                    @location(0) uv: vec2<f32>,
                    @location(1) color: vec4<f32>,
                };

                //------------[WGSL vertex shader]-------------------
                @vertex
                fn vs_main(in: VertexInput) -> VertexOutput {
                    var out: VertexOutput;
                    out.clip_position = camera.view_proj * vec4<f32>(in.position, 0.0, 1.0);
                    out.uv = in.uv;
                    out.color = in.color;
                    return out;
                }

                //------------[WGSL fragment shader]-------------------
                @fragment
                fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
                    let tex_color = textureSample(t_diffuse, s_diffuse, in.uv);
                    return tex_color * in.color;
                }
                "#,
            )),
        });

        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("engine_pipeline_layout"),
            bind_group_layouts: &[Some(&camera_bind_group_layout), Some(&texture_bind_group_layout)],
            immediate_size: 0,
        });

        let render_pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("engine_render_pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: Some("vs_main"),
                buffers: &[Some(Vertex::desc())],
                compilation_options: Default::default(),
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: Some("fs_main"),
                targets: &[Some(wgpu::ColorTargetState {
                    format: surface_config.format,
                    blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                compilation_options: Default::default(),
            }),
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: None,
                polygon_mode: wgpu::PolygonMode::Fill,
                unclipped_depth: false,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState::default(),
            multiview_mask: None,
            cache: None,
        });

        let initial_capacity = 4096;
        let vertex_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("engine_vertex_buffer"),
            size: (initial_capacity * std::mem::size_of::<Vertex>()) as wgpu::BufferAddress,
            usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        Self {
            window,
            device,
            queue,
            surface,
            surface_config,
            texture_bind_group_layout,
            active_backend_name,
            camera_buffer,
            camera_bind_group,
            render_pipeline,
            vertex_buffer,
            vertex_buffer_capacity: initial_capacity,
            vertices: Vec::with_capacity(initial_capacity),
            batches: Vec::new(),
            default_white,
            debug_font_texture,
            active_camera_pos: Vec2::ZERO,
            active_viewport_size: Vec2::new(width as f32, height as f32),
            supported_present_modes: surface_caps.present_modes,
        }
    }

    //------------[Resize surface swapchain]-------------------
    pub fn resize(&mut self, new_size: winit::dpi::PhysicalSize<u32>) {
        if new_size.width > 0 && new_size.height > 0 {
            self.surface_config.width = new_size.width;
            self.surface_config.height = new_size.height;
            self.surface.configure(&self.device, &self.surface_config);
        }
    }

    //------------[Get window physical size]-------------------
    pub fn window_size(&self) -> Vec2 {
        Vec2::new(self.surface_config.width as f32, self.surface_config.height as f32)
    }

    //------------[Update VSync present mode]-------------------
    pub fn set_vsync(&mut self, enabled: bool) {
        let mode = if enabled {
            if self.supported_present_modes.contains(&wgpu::PresentMode::Fifo) {
                wgpu::PresentMode::Fifo
            } else {
                wgpu::PresentMode::AutoVsync
            }
        } else {
            if self.supported_present_modes.contains(&wgpu::PresentMode::Immediate) {
                wgpu::PresentMode::Immediate
            } else if self.supported_present_modes.contains(&wgpu::PresentMode::Mailbox) {
                wgpu::PresentMode::Mailbox
            } else {
                wgpu::PresentMode::AutoNoVsync
            }
        };
        self.surface_config.present_mode = mode;
        self.surface.configure(&self.device, &self.surface_config);
    }

    //------------[Update camera matrix]-------------------
    pub fn set_camera(&mut self, camera_pos: Vec2, viewport_size: Vec2) {
        self.active_camera_pos = camera_pos;
        self.active_viewport_size = viewport_size;

        let right = viewport_size.x;
        let bottom = viewport_size.y;

        let view_proj = [
            2.0 / right, 0.0, 0.0, 0.0,
            0.0, -2.0 / bottom, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            -1.0, 1.0, 0.0, 1.0,
        ];

        self.queue.write_buffer(
            &self.camera_buffer,
            0,
            bytemuck::cast_slice(&[CameraUniform { view_proj }]),
        );
    }

    //------------[Push quad vertices and extend batch]-------------------
    fn push_quad(
        &mut self,
        v0: Vertex,
        v1: Vertex,
        v2: Vertex,
        v3: Vertex,
        texture_id: usize,
        texture_bind_group: &wgpu::BindGroup,
    ) {
        let start_vertex = self.vertices.len() as u32;
        self.vertices.extend_from_slice(&[v0, v1, v2, v0, v2, v3]);

        if let Some(last) = self.batches.last_mut().filter(|b| b.texture_id == texture_id) {
            last.count += 6;
        } else {
            self.batches.push(DrawBatch {
                texture_id,
                start_vertex,
                count: 6,
                bind_group: texture_bind_group.clone(),
            });
        }
    }

    //------------[Draw textured quad]-------------------
    pub fn draw_texture_rect(
        &mut self,
        pos: Vec2,
        size: Vec2,
        uv_min: Vec2,
        uv_max: Vec2,
        texture_id: usize,
        texture_bind_group: &wgpu::BindGroup,
        tint: [f32; 4],
    ) {
        let screen_pos = pos - self.active_camera_pos;
        let x0 = screen_pos.x;
        let y0 = screen_pos.y;
        let x1 = screen_pos.x + size.x;
        let y1 = screen_pos.y + size.y;

        let v0 = Vertex { position: [x0, y0], uv: [uv_min.x, uv_min.y], color: tint };
        let v1 = Vertex { position: [x1, y0], uv: [uv_max.x, uv_min.y], color: tint };
        let v2 = Vertex { position: [x1, y1], uv: [uv_max.x, uv_max.y], color: tint };
        let v3 = Vertex { position: [x0, y1], uv: [uv_min.x, uv_max.y], color: tint };

        self.push_quad(v0, v1, v2, v3, texture_id, texture_bind_group);
    }

    //------------[Draw rotated textured quad]-------------------
    pub fn draw_texture_rect_rotated(
        &mut self,
        pos: Vec2,
        size: Vec2,
        rot_deg: f32,
        scale: Vec2,
        uv_min: Vec2,
        uv_max: Vec2,
        texture_id: usize,
        texture_bind_group: &wgpu::BindGroup,
        tint: [f32; 4],
    ) {
        let screen_pos = pos - self.active_camera_pos;
        let (p0, p1, p2, p3) = if rot_deg == 0.0 && scale.x == 1.0 && scale.y == 1.0 {
            (
                [screen_pos.x, screen_pos.y],
                [screen_pos.x + size.x, screen_pos.y],
                [screen_pos.x + size.x, screen_pos.y + size.y],
                [screen_pos.x, screen_pos.y + size.y],
            )
        } else {
            let center = screen_pos + size * 0.5;
            let hw = size.x * 0.5 * scale.x;
            let hh = size.y * 0.5 * scale.y;

            let rad = rot_deg.to_radians();
            let cos_a = rad.cos();
            let sin_a = rad.sin();

            let rotate = |lx: f32, ly: f32| -> [f32; 2] {
                [
                    center.x + lx * cos_a - ly * sin_a,
                    center.y + lx * sin_a + ly * cos_a,
                ]
            };

            (
                rotate(-hw, -hh),
                rotate(hw, -hh),
                rotate(hw, hh),
                rotate(-hw, hh),
            )
        };

        let v0 = Vertex { position: p0, uv: [uv_min.x, uv_min.y], color: tint };
        let v1 = Vertex { position: p1, uv: [uv_max.x, uv_min.y], color: tint };
        let v2 = Vertex { position: p2, uv: [uv_max.x, uv_max.y], color: tint };
        let v3 = Vertex { position: p3, uv: [uv_min.x, uv_max.y], color: tint };

        self.push_quad(v0, v1, v2, v3, texture_id, texture_bind_group);
    }

    //------------[Draw solid color quad]-------------------
    #[allow(dead_code)]
    pub fn draw_quad(&mut self, pos: Vec2, size: Vec2, color: [f32; 4]) {
        let screen_pos = pos - self.active_camera_pos;
        let x0 = screen_pos.x;
        let y0 = screen_pos.y;
        let x1 = screen_pos.x + size.x;
        let y1 = screen_pos.y + size.y;

        let v0 = Vertex { position: [x0, y0], uv: [0.0, 0.0], color };
        let v1 = Vertex { position: [x1, y0], uv: [1.0, 0.0], color };
        let v2 = Vertex { position: [x1, y1], uv: [1.0, 1.0], color };
        let v3 = Vertex { position: [x0, y1], uv: [0.0, 1.0], color };

        let start_vertex = self.vertices.len() as u32;
        self.vertices.extend_from_slice(&[v0, v1, v2, v0, v2, v3]);

        let tex_id = self.default_white.id;
        if let Some(last) = self.batches.last_mut().filter(|b| b.texture_id == tex_id) {
            last.count += 6;
        } else {
            self.batches.push(DrawBatch {
                texture_id: tex_id,
                start_vertex,
                count: 6,
                bind_group: self.default_white.bind_group.clone(),
            });
        }
    }

    //------------[Draw textured sprite]-------------------
    #[allow(dead_code)]
    pub fn draw_sprite(&mut self, pos: Vec2, size: Vec2, texture: &Texture) {
        self.draw_texture_rect(
            pos,
            size,
            Vec2::ZERO,
            Vec2::ONE,
            texture.id,
            &texture.bind_group,
            [1.0, 1.0, 1.0, 1.0],
        );
    }

    //------------[Draw rectangle outline]-------------------
    pub fn draw_rect_outline(&mut self, pos: Vec2, size: Vec2, thickness: f32, color: [f32; 4]) {
        self.draw_quad(pos, Vec2::new(size.x, thickness), color);
        self.draw_quad(Vec2::new(pos.x, pos.y + size.y - thickness), Vec2::new(size.x, thickness), color);
        self.draw_quad(pos, Vec2::new(thickness, size.y), color);
        self.draw_quad(Vec2::new(pos.x + size.x - thickness, pos.y), Vec2::new(thickness, size.y), color);
    }

    //------------[Draw debug box with outline]-------------------
    pub fn draw_debug_box(&mut self, pos: Vec2, size: Vec2, fill_color: [f32; 4], outline_color: [f32; 4]) {
        self.draw_quad(pos, size, fill_color);
        self.draw_rect_outline(pos, size, 1.0, outline_color);
    }

    //------------[Draw debug text string]-------------------
    pub fn draw_debug_text(&mut self, text: &str, start_pos: Vec2, color: [f32; 4], scale: f32) {
        let glyph_w = 8.0 * scale;
        let glyph_h = 8.0 * scale;
        let tex_id = self.debug_font_texture.id;
        let bind_group = self.debug_font_texture.bind_group.clone();

        let mut cursor_x = start_pos.x;
        let mut cursor_y = start_pos.y;

        for ch in text.chars() {
            if ch == '\n' {
                cursor_x = start_pos.x;
                cursor_y += glyph_h + 2.0;
                continue;
            }

            let ascii_code = ch as u32;
            if ascii_code >= 32 && ascii_code < 128 {
                let col = (ascii_code % 16) as f32;
                let row = (ascii_code / 16) as f32;

                let u0 = col / 16.0;
                let u1 = (col + 1.0) / 16.0;
                let v0 = row / 8.0;
                let v1 = (row + 1.0) / 8.0;

                self.draw_texture_rect(
                    Vec2::new(cursor_x, cursor_y),
                    Vec2::new(glyph_w, glyph_h),
                    Vec2::new(u0, v0),
                    Vec2::new(u1, v1),
                    tex_id,
                    &bind_group,
                    color,
                );
            }

            cursor_x += glyph_w;
        }
    }

    //------------[Begin frame batch]-------------------
    pub fn begin_frame(&mut self) {
        self.vertices.clear();
        self.batches.clear();
    }

    //------------[End frame and submit to GPU]-------------------
    pub fn end_frame(&mut self) {
        let output = match self.surface.get_current_texture() {
            wgpu::CurrentSurfaceTexture::Success(tex)
            | wgpu::CurrentSurfaceTexture::Suboptimal(tex) => tex,
            _ => return,
        };
        let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

        if !self.vertices.is_empty() {
            if self.vertices.len() > self.vertex_buffer_capacity {
                self.vertex_buffer_capacity = self.vertices.len() * 2;
                self.vertex_buffer = self.device.create_buffer(&wgpu::BufferDescriptor {
                    label: Some("engine_vertex_buffer_expanded"),
                    size: (self.vertex_buffer_capacity * std::mem::size_of::<Vertex>()) as wgpu::BufferAddress,
                    usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
                    mapped_at_creation: false,
                });
            }

            self.queue.write_buffer(
                &self.vertex_buffer,
                0,
                bytemuck::cast_slice(&self.vertices),
            );
        }

        let mut encoder = self.device.create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("engine_render_encoder"),
        });

        {
            let mut render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("engine_render_pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    depth_slice: None,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.0,
                            g: 0.0,
                            b: 0.0,
                            a: 1.0,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
                multiview_mask: None,
            });

            render_pass.set_pipeline(&self.render_pipeline);
            render_pass.set_bind_group(0, &self.camera_bind_group, &[]);
            render_pass.set_vertex_buffer(0, self.vertex_buffer.slice(..));

            for batch in &self.batches {
                render_pass.set_bind_group(1, &batch.bind_group, &[]);
                render_pass.draw(batch.start_vertex..(batch.start_vertex + batch.count), 0..1);
            }
        }

        self.queue.submit(std::iter::once(encoder.finish()));
        self.queue.present(output);
    }
}
