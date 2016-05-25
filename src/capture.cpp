#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "gl_core_3_3.h"
#include "debug.h"
#include "troll_engine.h"
#include "window.h"
#include "vbo.h"
#include "vao.h"
#include "program.h"
#include "scenegraph.h"
#include "camera.h"
#include "texture.h"
#include "fbo.h"
#include "image.h"
#include "transform.h"
#include "sceneimporter.h"

#include "pov.h"
#include <typeinfo>

using namespace Engine;

typedef std::pair<std::string, ProgramBuilder::UniformType> UniformDescriptor;

void save_pov(std::vector<Viewpoint>& povs,
              SceneGraph& scene,
              DrawableNode& buddha,
              Window& window,
              Camera<TransformMat>& camera,
              glm::mat4 const& projOrig, 
              Program& colorProg, Program& normalProg,
              FBO& fbo,
              Texture& colorTex, Texture& normalTex, Texture& depthTex) {
    // Render color and depth to texture
    glm::mat4 projMatrix = glm::perspective(45., 1., 1., 100.);
    glViewport(0, 0, 512, 512);
    colorProg.use();
    dynamic_cast<Uniform<glm::mat4>*>(colorProg.getUniform("m_camera"))->set(camera.world_to_camera());
    dynamic_cast<Uniform<glm::mat3>*>(colorProg.getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(1)));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene.render();
    window.swapBuffers();

    fbo.attach(FBO::Read, FBO::Color, colorTex);
    std::vector<unsigned char> color(FBO::readPixels<unsigned char>(FBO::Bgr, FBO::Ubyte, 512, 512));

    Image colorImg(Image::from_rgb(color, 512, 512, true));
    Image depthImg(Image::from_greyscale(FBO::readPixels<unsigned short>(FBO::DepthComponent, FBO::Ushort, 512, 512), 512, 512, true));

    // Render normals to texture
    fbo.attach(FBO::Draw, FBO::Color, normalTex);
    normalProg.use();
    dynamic_cast<Uniform<glm::mat4>*>(normalProg.getUniform("m_camera"))->set(camera.world_to_camera());
    dynamic_cast<Uniform<glm::mat3>*>(normalProg.getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(1)));
    buddha.set_program(&normalProg);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene.render();
    window.swapBuffers();

    fbo.attach(FBO::Read, FBO::Color, normalTex);
    std::vector<unsigned char> normal(FBO::readPixels<unsigned char>(FBO::Bgr, FBO::Ubyte, 512, 512));
    Image normalImg(Image::from_rgb(normal, 512, 512));
    
    povs.push_back(Viewpoint(camera.world_to_camera(), camera.transform().position(), colorImg, depthImg, normalImg));
    
    // Restore previous state
    FBO::bind_default(FBO::Both);
    buddha.set_program(&colorProg);
    glViewport(0, 0, window.width(), window.height());
}

int main(int argc, char** argv) {
    engine_init(argc, argv);
    if(argc != 2) {
        std::cout << "Usage : " << argv[0] << " <mesh>" << std::endl;
        return 1;
    }
    {
        // First, create the window
        WindowBuilder wb;
        Window window = wb.size(1280, 720)
                                .title("Buddha")
                                .vsync(false)
                                .debug()
                                .build();
        if (!window) {
            std::cerr << "Error : cannot create window" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        //ogl_CheckExtensions();
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(&gl_cb, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);

        FreeImage_SetOutputMessage([] (FREE_IMAGE_FORMAT fif, const char *message) {
                if(fif != FIF_UNKNOWN) {
                std::cerr << "Format : " << FreeImage_GetFormatFromFIF(fif) << std::endl;
                }
                std::cerr << (message) << std::endl; });

        window.track_fps(false);
        std::cout << window.context_info() << std::endl;
        window.showCursor(false);

        ShaderManager sm;
        SceneImporter si;
        si.readFile(argv[1], SceneImporter::PostProcess::JoinVertices);
        auto mesh = si.instantiateMesh(*si.meshes()[0]);

        // Then, the shader programs
        glm::mat4 projMatrix = glm::perspective<float>(glm::radians(45.f), 1280.f/720.f, 0.1, 1000);
        TransformEuler worldTransform;
        glm::vec3 lightPosition(5, 15, -15);

        auto prog_phong = sm.buildProgram().vertexShader("shaders/per_fragment.vs")
                                           .fragmentShader("shaders/per_fragment.fs")
                                           .uniform("m_proj", ProgramBuilder::UniformType::Mat4)
                                           .uniform("m_world", ProgramBuilder::UniformType::Mat4)
                                           .uniform("m_camera", ProgramBuilder::UniformType::Mat4)
                                           .uniform("m_normalTransform", ProgramBuilder::UniformType::Mat3)
                                           .uniform("ambient_intensity", ProgramBuilder::UniformType::Float)
                                           .build();
        dynamic_cast<Uniform<glm::mat4>*>(prog_phong.getUniform("m_proj"))->set(projMatrix);
        dynamic_cast<Uniform<float>*>(prog_phong.getUniform("ambient_intensity"))->set(0.2);
        //dynamic_cast<Uniform<glm::mat4>*>(prog_phong.getUniform("m_world"))->set(worldMatrix);

        auto prog_normals = sm.buildProgram().vertexShader("shaders/per_fragment.vs")
                                             .fragmentShader("shaders/per_fragment.fs")
                                             .uniform("m_proj", ProgramBuilder::UniformType::Mat4)
                                             .uniform("m_world", ProgramBuilder::UniformType::Mat4)
                                             .uniform("m_camera", ProgramBuilder::UniformType::Mat4)
                                             .uniform("m_normalTransform", ProgramBuilder::UniformType::Mat3)
                                             .uniform("ambient_intensity", ProgramBuilder::UniformType::Float)
                                             .build();
        dynamic_cast<Uniform<glm::mat4>*>(prog_normals.getUniform("m_proj"))->set(projMatrix);
        //dynamic_cast<Uniform<glm::mat4>*>(prog_normals.getUniform("m_world"))->set(worldMatrix);

        // Fill the scene
        Camera<TransformMat> camera;
        //camera.look_at(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        camera.translate_local(Direction::Back, 1);
        SceneGraph scene;
        auto buddha = mesh->instantiate(glm::mat4(1), &prog_phong, nullptr, GL_UNSIGNED_SHORT);
        scene.addChild(buddha);

        Program* current_prog = &prog_phong;
        window.registerKeyCallback('P', [&] () {
            current_prog = (current_prog == &prog_phong) ? &prog_normals : &prog_phong;
            buddha->set_program(current_prog);
        });
        
        // TODO : this must go
        // Some more GL related stuff
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthRange(0.0f, 1.0f);
        glClearColor(0.4f, 0.4f, 0.4f, 1.f);
        glClearDepth(1.0f);

        // Setup render to texture
        Texture colorTex, depthTex, normalTex;
        colorTex.texData (GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 512, 512, nullptr);
        depthTex.texData (GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, 512, 512, nullptr);
        normalTex.texData(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 512, 512, nullptr);
        Texture::unbind();
        FBO fbo;
        fbo.bind(FBO::Both);   
        glViewport(0,0,1280,720);
        GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);
        fbo.attach(FBO::Draw, FBO::Color, colorTex);
        fbo.attach(FBO::Draw, FBO::Depth, depthTex);
        assert(FBO::is_complete(FBO::Draw));
        FBO::bind_default(FBO::Both);

        // Install input callbacks
        std::vector<Viewpoint> povs;
        window.registerKeyCallback(GLFW_KEY_ESCAPE, [&window] () { window.close(); });
        window.registerKeyCallback('R', [&camera] () { camera.translate_local(Direction::Down, 1); });
        window.registerKeyCallback('F', [&camera] () { camera.translate_local(Direction::Up, 1); });
        window.registerKeyCallback('W', [&camera] () { camera.translate_local(Direction::Front, 1); });
        window.registerKeyCallback('A', [&camera] () { camera.translate_local(Direction::Left, 1); });
        window.registerKeyCallback('S', [&camera] () { camera.translate_local(Direction::Back, 1); });
        window.registerKeyCallback('D', [&camera] () { camera.translate_local(Direction::Right, 1); });
        window.registerKeyCallback('Q', [&] () { save_pov(povs, scene, *buddha, window,
                                                          camera, projMatrix, prog_phong, prog_normals, fbo,
                                                          colorTex, normalTex, depthTex); });
        window.registerKeyCallback('E', [&povs] () {
                if(povs.size() == 0) {
                    std::cerr << "I won't save this, no viewpoints have been captured." << std::endl;
                }
                else {
                    std::ofstream outFile;
                    outFile.open("pov_archive", std::ios::trunc | std::ios::binary);
                    boost::archive::binary_oarchive ar(outFile);
                    ar & povs;
                }
            });
        window.registerMousePosCallback([&camera] (double x, double y) {
                static float prev_x = 0, prev_y = 0;
                float xoffset = x - prev_x, yoffset = y - prev_y;
                prev_x = x; prev_y = y;
                const float f = 0.01;
                xoffset *= f;
                yoffset *= f;
                camera.rotate(glm::vec3(0, 1, 0), xoffset);
                camera.rotate(glm::vec3(1, 0, 0), yoffset);
            });
        window.registerMouseButtonCallback([&camera] (int button, int action, int mods) {
                std::cout << ((action == GLFW_PRESS) ? "Pressed " : "Released ") << ((button == GLFW_MOUSE_BUTTON_1) ? "left" :
                                                                                     (button == GLFW_MOUSE_BUTTON_2) ? "right" : 
                                                                                     (button == GLFW_MOUSE_BUTTON_3) ? "middle" : "unknown") << " mouse button." << std::endl;
            });
        window.setResizeCallback([&] (int w, int h) {
            // TODO : wrap the GL call away
            glViewport(0, 0, w, h);
            projMatrix = glm::perspective<float>(45, static_cast<float>(w)/static_cast<float>(h), 0.1, 1000);
            dynamic_cast<Uniform<glm::mat4>*>(prog_phong.getUniform("m_proj"))->set(projMatrix);
            dynamic_cast<Uniform<glm::mat4>*>(prog_normals.getUniform("m_proj"))->set(projMatrix);
        });

        // Finally, the render function
        window.setRenderCallback([&] () {
            glm::mat4 cameraMatrix = camera.world_to_camera(),
                      worldMatrix  = worldTransform.matrix();
            current_prog->use();
            buddha->set_transform(worldMatrix);
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_camera"))->set(cameraMatrix);
            dynamic_cast<Uniform<glm::mat3>*>(current_prog->getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(worldMatrix)));

            glm::vec3 pos_cam = camera.transform().position();

            // TODO : this too
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            scene.render(); });

        window.mainLoop();

    }

        // TODO : yep, this too
        glfwTerminate();

        return 0;
    }
