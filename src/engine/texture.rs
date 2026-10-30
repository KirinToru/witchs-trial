use std::path::Path;
use std::sync::atomic::{AtomicUsize, Ordering};

static NEXT_TEXTURE_ID: AtomicUsize = AtomicUsize::new(1);

#[allow(dead_code)]
pub struct Texture {
    pub id: usize,
    pub texture: wgpu::Texture,
    pub view: wgpu::TextureView,
    pub sampler: wgpu::Sampler,
    pub bind_group: wgpu::BindGroup,
    pub width: u32,
    pub height: u32,
}

impl Texture {
    //------------[Load texture from file]-------------------
    pub fn from_file<P: AsRef<Path>>(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        path: P,
        layout: &wgpu::BindGroupLayout,
    ) -> Result<Self, String> {
        let bytes = std::fs::read(&path)
            .map_err(|e| format!("Failed to read file {}: {}", path.as_ref().display(), e))?;
        Self::from_bytes(device, queue, &bytes, layout)
    }

    //------------[Load texture from memory bytes]-------------------
    pub fn from_bytes(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        bytes: &[u8],
        layout: &wgpu::BindGroupLayout,
    ) -> Result<Self, String> {
        let img = image::load_from_memory(bytes)
            .map_err(|e| format!("Failed to decode image bytes: {}", e))?;
        let rgba = img.to_rgba8();
        let (width, height) = (img.width(), img.height());
        Ok(Self::from_rgba(device, queue, width, height, &rgba, layout))
    }

    //------------[Create GPU texture from RGBA buffer]-------------------
    pub fn from_rgba(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        width: u32,
        height: u32,
        rgba: &[u8],
        layout: &wgpu::BindGroupLayout,
    ) -> Self {
        let id = NEXT_TEXTURE_ID.fetch_add(1, Ordering::Relaxed);
        let size = wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        };

        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some("engine_texture"),
            size,
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8UnormSrgb,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });

        queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &texture,
                mip_level: 0,
                origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            rgba,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(4 * width),
                rows_per_image: Some(height),
            },
            size,
        );

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());

        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            address_mode_u: wgpu::AddressMode::Repeat,
            address_mode_v: wgpu::AddressMode::Repeat,
            address_mode_w: wgpu::AddressMode::Repeat,
            mag_filter: wgpu::FilterMode::Nearest,
            min_filter: wgpu::FilterMode::Nearest,
            mipmap_filter: wgpu::MipmapFilterMode::Nearest,
            ..Default::default()
        });

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("engine_texture_bind_group"),
            layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(&view),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
            ],
        });

        Self {
            id,
            texture,
            view,
            sampler,
            bind_group,
            width,
            height,
        }
    }

    //------------[Create 1x1 white fallback texture]-------------------
    pub fn white_1x1(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        layout: &wgpu::BindGroupLayout,
    ) -> Self {
        Self::from_rgba(device, queue, 1, 1, &[255, 255, 255, 255], layout)
    }
}
