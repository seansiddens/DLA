#version 430
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

void main() {
    // Index into global workspace
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    vec3 prev_col = imageLoad(img_output, pixel_coords).rgb;

    // Write col to new position
    imageStore(img_output, pixel_coords, vec4(prev_col * vec3(.95, .95, .99), 1.0));
}
