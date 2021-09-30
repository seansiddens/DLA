#version 430
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std140, binding = 4) buffer Walker {
    ivec3 Walkers[ ];
};

uniform uint u_frame;
uniform uint u_window_width;
uniform uint u_window_height;
uniform uint u_num_particles;

#define EPSILON 0.01

float colormap_red(float x) {
    if (x < 0.7) {
        return 4.0 * x - 1.5;
    } else {
        return -4.0 * x + 4.5;
    }
}

float colormap_green(float x) {
    if (x < 0.5) {
        return 4.0 * x - 0.5;
    } else {
        return -4.0 * x + 3.5;
    }
}

float colormap_blue(float x) {
    if (x < 0.3) {
       return 4.0 * x + 0.5;
    } else {
       return -4.0 * x + 2.5;
    }
}

vec4 colormap_jet(float x) {
    float r = clamp(colormap_red(x), 0.0, 1.0);
    float g = clamp(colormap_green(x), 0.0, 1.0);
    float b = clamp(colormap_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}

// cosine based palette, 4 vec3 params
// From: https://iquilezles.org/www/articles/palettes/palettes.htm
vec3 palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d ) {
    return a + b*cos( 6.28318*(c*t+d) );
}


uvec2 pcg2d(uvec2 v) {
    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v>>16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v>>16u);

    return v;
}

uint pcg_hash(uint i) {
    uint state = i * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Returns a random float in [0.0, 1.0]
float random_float(uint i) {
    return pcg_hash(i) * (1.0 / float(0xffffffffu));
}

void main() {
    // Index into global workspace
    uint gid = gl_GlobalInvocationID.x;
    if (Walkers[gid].z == 1) {
        // Get random offset
        uvec2 hash = pcg2d(uvec2(gid, u_frame));
        vec2 rand = vec2(hash) * (1.0 / 0xffffffffu);
        rand *= 3.0;
        ivec2 offset = ivec2(floor(rand));
        offset -= 1;

        // Update position by offset
        Walkers[gid].xy += offset;
        Walkers[gid].xy = ivec2(mod(Walkers[gid].xy, ivec2(u_window_width, u_window_height))); // Walkers live on torus

        // Cluster checking
        vec3 write_col = vec3(1.0);
        ivec2 pixel_coords = Walkers[gid].xy;
        int count = 0;
        vec3 read_left = imageLoad(img_output, pixel_coords + ivec2(-1, 0)).rgb;
        if (dot(read_left, read_left) > 0) {
            count += 1;
        }
        vec3 read_right = imageLoad(img_output, pixel_coords + ivec2(1, 0)).rgb;
        if (dot(read_right, read_right) > 0) {
            count += 1;
        }
        vec3 read_up = imageLoad(img_output, pixel_coords + ivec2(0, 1)).rgb;
        if (dot(read_up, read_up) > 0) {
            count += 1;
        }
        vec3 read_down = imageLoad(img_output, pixel_coords + ivec2(0, -1)).rgb;
        if (dot(read_down, read_down) > 0) {
            count += 1;
        }

        vec3 sum = read_left + read_right + read_up + read_down;

        int x = int(dot(sum, sum) <= 0.0);
        // Zero if any of the pixels we read were part of the cluster, one otherwise
        if (x == 0) {
//            float t = mod(u_frame / 1000.0, 1.0);
//            vec3 final_col = colormap_jet(t).rgb;
            imageStore(img_output, pixel_coords, vec4(sum / count, 1.0));
            Walkers[gid].z = x;
        }
    }
}


