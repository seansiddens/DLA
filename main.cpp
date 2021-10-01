#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "Shader.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600

#define WORK_GRP_SIZE 16
#define NUM_PARTICLES WORK_GRP_SIZE * 32000

//TODO: Rendering a higher/lower resolution texture to the window
//TODO: GUI stuff - cluster drawing color, clearing of the screen, reset simulation, particle count, etc.


// Struct definition for a random walker
struct Walker {
    int x, y, state;
};

struct color {
    float r, g, b;
};


void put_seed(float *image, int x, int y, float r, float g, float b) {
    int i = y * WINDOW_WIDTH * 4 + x * 4;
    image[i] = r;
    image[i+1] = g;
    image[i+2] = b;
    image[i+3] = 1.0;
}


//
// Returns a random real in [0, 1)
//
float random_uniform() {
    return (float) random() / (float) ((long int) RAND_MAX + 1);
}


//
// Simple GUI overlay for displaying program metrics and parameters.
// bool *p_open: Flag whether to display or not.
// double frameTime: Time (in ms) between frames.
//
static void debugOverlay(bool* p_open, double frameTime, unsigned int frame)
{
    static int corner = 0;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration;
    if (corner != -1)
    {
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
        window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
        window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
        window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        window_flags |= ImGuiWindowFlags_NoMove;
    }
    ImGui::SetNextWindowBgAlpha(0.6f); // Transparent background
    if (ImGui::Begin("Example: Simple overlay", p_open, window_flags))
    {
        ImGui::Text("%d x %d", WINDOW_WIDTH, WINDOW_HEIGHT);
        ImGui::Text("%.2f ms/frame \n%d FPS", frameTime, (int)(1000.0 / frameTime));
        ImGui::Text("Frame: %u", frame);
        ImGui::Separator();
        ImGui::Text("# of walkers: %d", NUM_PARTICLES);
    }
    ImGui::End();
}

void clear_texture(float *tex, int width, int height, int channels) {
    memset(tex, 0, width * height * channels * sizeof(float));
}

int main() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("DLA", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
    }

    // Initialize rendering context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    SDL_GL_SetSwapInterval(0); // No vsync

    // Initialize GL Extension Wrangler (GLEW)
    glewExperimental = GL_TRUE; // Please expose OpenGL 3.x+ interfaces
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Failed to init GLEW" << std::endl;
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;

    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Setup platform/renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, context);
    ImGui_ImplOpenGL3_Init("#version 430");

    // Compute shader info
    int work_grp_cnt[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    std::cout << "Max global (total) work group counts" << std::endl;
    std::cout << "x: " << work_grp_cnt[0] << ", y: " << work_grp_cnt[1] << ", z: " << work_grp_cnt[2] << std::endl;

    int work_grp_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    std::cout << "Max local (in one shader) work group sizes" << std::endl;
    std::cout << "x: " << work_grp_size[0] << ", y: " << work_grp_size[1] << ", z: " << work_grp_size[2] << std::endl;

    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    std::cout << "Max local work group invocations: " << work_grp_inv << std::endl;

    std::cout << "Total number of particles: " << NUM_PARTICLES << std::endl;

    // Build and compile our shaders
    // The normal vert + frag shader for displaying to quad
    Shader quadProgram("../vert.glsl", "../frag.glsl");

    // Compute shader which updates random walkers and writes to the texture
    ComputeShader computeProgram("../compute.glsl");
    computeProgram.use();


    // Set up vertex data for a fullscreen quad
    float vertices[] = {
            // Positions          // Texture coords
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f,       // Top right
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,       // Bottom right
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,       // Bottom left
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f        // Top left
    };
    // Indexed drawing of quad
    unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
    };

    // Create objects
    unsigned int quadVAO, quadVBO, quadEBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) nullptr);
    glEnableVertexAttribArray(0);
    // Tex coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Shader Storage Buffer Object (SSBO) for storing positions and states of the random walkers
    GLuint walkerSSBO;
    glGenBuffers(1, &walkerSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, walkerSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct Walker), nullptr, GL_STATIC_DRAW);
    // Map data store to CPU address space, allowing us to directly write to it
    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    auto walkers = (struct Walker *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
                                                         NUM_PARTICLES * sizeof(struct Walker),
                                                         bufMask);
    // Initialize with random positions
    for (int i = 0; i < NUM_PARTICLES; i++) {
        walkers[i].x = (int) (random_uniform() * WINDOW_WIDTH);
        walkers[i].y = (int) (random_uniform() * WINDOW_HEIGHT);
        walkers[i].state = 1;
    }

    auto originalWalkers = (struct Walker *) malloc(NUM_PARTICLES * sizeof(struct Walker));
    memcpy(originalWalkers, walkers, NUM_PARTICLES * sizeof(struct  Walker));

    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, walkerSSBO);


    // Create texture for writing to
    GLuint tex_output;
    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    auto tex_data = (float *) malloc((WINDOW_WIDTH * WINDOW_HEIGHT * 4) * sizeof(float));

    put_seed(tex_data, 300, 200, 1, 0, 0);
    put_seed(tex_data, 200, 400, 0, 1, 0);
    put_seed(tex_data, 400, 400, 0, 0, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT,
                 tex_data);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    // Create framebuffer
    unsigned int fbo;
    glGenBuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // MAIN RENDER LOOP
    bool updateWalkers = false; // Flag to determine whether to update the walkers every frame
    bool showMetrics = true;    // Show the debug overlay or not
    bool shouldLoop = true;
    uint64_t performanceFreq = SDL_GetPerformanceFrequency();
    double deltaTime;
    uint64_t currTime;
    uint64_t prevTime = SDL_GetPerformanceCounter();
    unsigned int frame = 0;
    bool mouseDown = false;
    int mouseX;
    int mouseY;
    while (shouldLoop) {
        // Timing
        currTime = SDL_GetPerformanceCounter();
        deltaTime = (1000.0 * (double) (currTime - prevTime)) / (double) performanceFreq;
        prevTime = currTime;

        // Process event queue
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Forward to Imgui
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    shouldLoop = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            shouldLoop = false;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            glViewport(0, 0, event.window.data1, event.window.data2);
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            shouldLoop = false;
                            break;
                        case SDLK_RETURN:
                            updateWalkers = !updateWalkers;
                            break;
                        case SDLK_d:
                            showMetrics = !showMetrics;
                            break;
                        case SDLK_r:
                            // Reset texture and walkers
                            clear_texture(tex_data, WINDOW_WIDTH, WINDOW_HEIGHT, 4);
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT,
                                         tex_data);
                            glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

                            memcpy(walkers, originalWalkers, NUM_PARTICLES * sizeof(struct Walker));
                            glBindBuffer(GL_SHADER_STORAGE_BUFFER, walkerSSBO);
                            glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(struct Walker), walkers, GL_STATIC_DRAW);
                            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, walkerSSBO);

                            if (updateWalkers) {
                                updateWalkers = false;
                            }
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (updateWalkers) {
                        updateWalkers = false;
                    }
                    mouseDown = true;
                    break;
                case SDL_MOUSEBUTTONUP:
                    mouseDown = false;
                    break;
            }
        }

        if (mouseDown) {
            SDL_GetMouseState(&mouseX, &mouseY);
            mouseY = WINDOW_HEIGHT - mouseY;
            std::cout << "Mouse pressed at (" << mouseX << ", " << mouseY << ")" << std::endl;
            put_seed(tex_data, mouseX, mouseY, 1, 1, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT,
                         tex_data);
            glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
        }

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        if (showMetrics) {
            // Draw metrics overlay
            debugOverlay(&showMetrics, deltaTime, frame);
        }


        // Launch compute shaders
        if (updateWalkers) {
            computeProgram.use();
            computeProgram.uniform1ui("u_frame", frame);
            computeProgram.uniform1ui("u_window_width", WINDOW_WIDTH);
            computeProgram.uniform1ui("u_window_height", WINDOW_HEIGHT);
            computeProgram.uniform1ui("u_num_particles", NUM_PARTICLES);
            glDispatchCompute(NUM_PARTICLES / WORK_GRP_SIZE, 1, 1);
        }

        // Causes the trail effect by diminishing the color each frame
//        displayProgram.use();
//        glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
//        glDispatchCompute(WINDOW_WIDTH, WINDOW_HEIGHT, 1);

        // Make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // Render the textured quad to screen
        glClear(GL_COLOR_BUFFER_BIT);
        quadProgram.use();
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, tex_output);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // Render GUI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData((ImGui::GetDrawData()));

        // Swap buffers
        SDL_GL_SwapWindow(window);

        // Increment frame counter
        frame += 1;
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
