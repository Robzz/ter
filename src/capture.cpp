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
#include "obj.h"
#include "fbo.h"
#include "image.h"
#include "transform.h"

#include "pov.h"

typedef std::pair<std::string, Engine::ProgramBuilder::UniformType> UniformDescriptor;

Engine::Program buildShaderProgram(std::string const& vs_file, std::string const& fs_file, std::vector<UniformDescriptor> const& uniforms);
void save_pov(std::vector<Viewpoint>& povs,
              Engine::SceneGraph& scene,
              Engine::IndexedObject& buddha,
              Engine::Window& window,
              Engine::Camera<Engine::TransformMat>& camera,
              glm::mat4 const& projOrig, 
              Engine::Program& colorProg, Engine::Program& normalProg,
              Engine::FBO& fbo,
              Engine::Texture& colorTex, Engine::Texture& normalTex, Engine::Texture& depthTex);

// Build the shader program used in the project
Engine::Program buildShaderProgram(std::string const& vs_file, std::string const& fs_file, std::vector<UniformDescriptor> const& uniforms) {
    std::ifstream fvs(vs_file);
    std::ifstream ffs(fs_file);
    Engine::VertexShader vs(fvs);
    if(!vs) {
        std::cerr << "Vertex shader compile error : " << std::endl << vs.info_log() << std::endl;
        exit(EXIT_FAILURE);
    }
    Engine::FragmentShader fs(ffs);
    if(!fs) {
        std::cerr << "Fragment shader compile error : " << std::endl << fs.info_log() << std::endl;
        exit(EXIT_FAILURE);
    }
    Engine::ProgramBuilder pb = Engine::ProgramBuilder().attach_shader(vs)
                                                        .attach_shader(fs);
    for(auto it = uniforms.begin() ; it != uniforms.end() ; ++it) {
        pb = pb.with_uniform((*it).first, (*it).second);
    }
    Engine::Program p = pb.link();

    if(!p) {
        std::cerr << "Program link error : " << std::endl << p.info_log() << std::endl;
        exit(EXIT_FAILURE);
    }
    return p;
}

void save_pov(std::vector<Viewpoint>& povs,
              Engine::SceneGraph& scene,
              Engine::IndexedObject& buddha,
              Engine::Window& window,
              Engine::Camera<Engine::TransformMat>& camera,
              glm::mat4 const& projOrig, 
              Engine::Program& colorProg, Engine::Program& normalProg,
              Engine::FBO& fbo,
              Engine::Texture& colorTex, Engine::Texture& normalTex, Engine::Texture& depthTex) {
    // Render color and depth to texture
    glm::mat4 projMatrix = glm::perspective(45., 1., 1., 100.);
    glViewport(0, 0, 512, 512);
    colorProg.use();
    dynamic_cast<Engine::Uniform<glm::mat4>*>(colorProg.getUniform("m_camera"))->set(camera.world_to_camera());
    dynamic_cast<Engine::Uniform<glm::mat3>*>(colorProg.getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(1)));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene.render();
    window.swapBuffers();

    fbo.attach(Engine::FBO::Read, Engine::FBO::Color, colorTex);
    std::vector<unsigned char> color(Engine::FBO::readPixels<unsigned char>(Engine::FBO::Bgr, Engine::FBO::Ubyte, 512, 512));

    Engine::Image colorImg(Engine::Image::from_rgb(color, 512, 512, true));
    Engine::Image depthImg(Engine::Image::from_greyscale(Engine::FBO::readPixels<unsigned short>(Engine::FBO::DepthComponent, Engine::FBO::Ushort, 512, 512), 512, 512, true));

    // Render normals to texture
    fbo.attach(Engine::FBO::Draw, Engine::FBO::Color, normalTex);
    normalProg.use();
    dynamic_cast<Engine::Uniform<glm::mat4>*>(normalProg.getUniform("m_camera"))->set(camera.world_to_camera());
    dynamic_cast<Engine::Uniform<glm::mat3>*>(normalProg.getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(1)));
    buddha.set_program(&normalProg);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene.render();
    window.swapBuffers();

    fbo.attach(Engine::FBO::Read, Engine::FBO::Color, normalTex);
    std::vector<unsigned char> normal(Engine::FBO::readPixels<unsigned char>(Engine::FBO::Bgr, Engine::FBO::Ubyte, 512, 512));
    Engine::Image normalImg(Engine::Image::from_rgb(normal, 512, 512));
    
    povs.push_back(Viewpoint(camera.world_to_camera(), camera.transform().position(), colorImg, depthImg, normalImg));
    
    // Restore previous state
    Engine::FBO::bind_default(Engine::FBO::Both);
    buddha.set_program(&colorProg);
    glViewport(0, 0, window.width(), window.height());
}

int main(int argc, char** argv) {
    Engine::engine_init(argc, argv);
    if(argc != 2) {
        std::cout << "Usage : " << argv[0] << " <mesh>" << std::endl;
        return 1;
    }
    Engine::Obj obj = Engine::ObjReader().file(argv[1]).read();
    Engine::Mesh* mesh = obj.get_group("default");
    {
        // First, create the window
        Engine::WindowBuilder wb;
        Engine::Window window = wb.size(1280, 720)
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
        #ifdef DEBUG
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageCallback(&gl_cb, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, true);
        #endif

        FreeImage_SetOutputMessage([] (FREE_IMAGE_FORMAT fif, const char *message) {
                if(fif != FIF_UNKNOWN) {
                std::cerr << "Format : " << FreeImage_GetFormatFromFIF(fif) << std::endl;
                }
                std::cerr << (message) << std::endl; });

        window.track_fps(false);
        std::cout << window.context_info() << std::endl;
        window.showCursor(false);

        Engine::VBO coords, normals, indices;
        coords.upload_data(mesh->get_attribute<glm::vec4>("vertices")->data());
        normals.upload_data(mesh->get_attribute<glm::vec3>("normals")->data());
        indices.upload_data(mesh->get_attribute<unsigned int>("indices")->data());
        
        // Then, the shader programs
        glm::mat4 projMatrix = glm::perspective<float>(glm::radians(45.f), 1280.f/720.f, 0.1, 1000);
        Engine::TransformEuler worldTransform;
        glm::vec3 lightPosition(5, 15, -15);

        std::vector<UniformDescriptor> uniforms_p1, uniforms_p2;

        uniforms_p1.push_back(UniformDescriptor("m_proj", Engine::ProgramBuilder::mat4));
        uniforms_p1.push_back(UniformDescriptor("m_world", Engine::ProgramBuilder::mat4));
        uniforms_p1.push_back(UniformDescriptor("m_camera", Engine::ProgramBuilder::mat4));
        uniforms_p1.push_back(UniformDescriptor("m_normalTransform", Engine::ProgramBuilder::mat3));
        uniforms_p1.push_back(UniformDescriptor("ambient_intensity", Engine::ProgramBuilder::float_));
        Engine::Program prog_phong(buildShaderProgram("shaders/per_fragment.vs", "shaders/per_fragment.fs", uniforms_p1));
        dynamic_cast<Engine::Uniform<glm::mat4>*>(prog_phong.getUniform("m_proj"))->set(projMatrix);
        dynamic_cast<Engine::Uniform<float>*>(prog_phong.getUniform("ambient_intensity"))->set(0.2);
        //dynamic_cast<Uniform<glm::mat4>*>(prog_phong.getUniform("m_world"))->set(worldMatrix);

        uniforms_p2.push_back(UniformDescriptor("m_camera",Engine::ProgramBuilder::mat4));
        uniforms_p2.push_back(UniformDescriptor("m_world",Engine::ProgramBuilder::mat4));
        uniforms_p2.push_back(UniformDescriptor("m_proj",Engine::ProgramBuilder::mat4));
        uniforms_p2.push_back(UniformDescriptor("m_normalTransform",Engine::ProgramBuilder::mat3));
        Engine::Program prog_normals(buildShaderProgram("shaders/normals.vs", "shaders/normals.fs", uniforms_p2));
        dynamic_cast<Engine::Uniform<glm::mat4>*>(prog_normals.getUniform("m_proj"))->set(projMatrix);
        //dynamic_cast<Uniform<glm::mat4>*>(prog_normals.getUniform("m_world"))->set(worldMatrix);

        // Setup vertex attributes
        Engine::VAO vao_phong, vao_normals;
        GLuint posIndex    = static_cast<unsigned int>(prog_phong.getAttributeLocation("v_position"));
        GLuint normalIndex = static_cast<unsigned int>(prog_phong.getAttributeLocation("v_normal"));
        vao_phong.enableVertexAttribArray(posIndex);
        vao_phong.vertexAttribPointer(coords, posIndex, 4, 0, 0);
        vao_phong.enableVertexAttribArray(normalIndex);
        vao_phong.vertexAttribPointer(normals, normalIndex, 3, 0, 0);
        posIndex    = static_cast<unsigned int>(prog_normals.getAttributeLocation("v_position"));
        normalIndex = static_cast<unsigned int>(prog_normals.getAttributeLocation("v_normal"));
        vao_normals.enableVertexAttribArray(posIndex);
        vao_normals.vertexAttribPointer(coords, posIndex, 4, 0, 0);
        vao_normals.enableVertexAttribArray(normalIndex);
        vao_normals.vertexAttribPointer(normals, normalIndex, 3, 0, 0);

        // Fill the scene
        Engine::Camera<Engine::TransformMat> camera;
        //camera.look_at(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        camera.translate_local(Engine::Direction::Back, 1);
        Engine::SceneGraph scene;
        Engine::IndexedObject* buddha = new Engine::IndexedObject(glm::mat4(1), &prog_phong, &indices, &vao_phong,
                                                                  mesh->get_attribute<unsigned int>("indices")->size(), nullptr,
                                                                  GL_UNSIGNED_INT);
        scene.addChild(buddha);

        Engine::Program* current_prog = &prog_phong;
        Engine::VAO* current_vao = &vao_phong;
        window.registerKeyCallback('P', [&] () {
            current_prog = (current_prog == &prog_phong) ? &prog_normals : &prog_phong;
            current_vao = (current_prog == &prog_phong) ? &vao_normals : &vao_phong;
            buddha->set_program(current_prog);
            buddha->set_vao(current_vao);
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
        Engine::Texture colorTex, depthTex, normalTex;
        colorTex.texData (GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 512, 512, nullptr);
        depthTex.texData (GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_FLOAT, 512, 512, nullptr);
        normalTex.texData(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, 512, 512, nullptr);
        Engine::Texture::unbind();
        Engine::FBO fbo;
        fbo.bind(Engine::FBO::Both);   
        glViewport(0,0,1280,720);
        GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, drawBuffers);
        fbo.attach(Engine::FBO::Draw, Engine::FBO::Color, colorTex);
        fbo.attach(Engine::FBO::Draw, Engine::FBO::Depth, depthTex);
        assert(Engine::FBO::is_complete(Engine::FBO::Draw));
        Engine::FBO::bind_default(Engine::FBO::Both);

        // Install input callbacks
        std::vector<Viewpoint> povs;
        window.registerKeyCallback(GLFW_KEY_ESCAPE, [&window] () { window.close(); });
        window.registerKeyCallback('R', [&camera] () { camera.translate_local(Engine::Direction::Down, 1); });
        window.registerKeyCallback('F', [&camera] () { camera.translate_local(Engine::Direction::Up, 1); });
        window.registerKeyCallback('W', [&camera] () { camera.translate_local(Engine::Direction::Front, 1); });
        window.registerKeyCallback('A', [&camera] () { camera.translate_local(Engine::Direction::Left, 1); });
        window.registerKeyCallback('S', [&camera] () { camera.translate_local(Engine::Direction::Back, 1); });
        window.registerKeyCallback('D', [&camera] () { camera.translate_local(Engine::Direction::Right, 1); });
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
            dynamic_cast<Engine::Uniform<glm::mat4>*>(prog_phong.getUniform("m_proj"))->set(projMatrix);
            dynamic_cast<Engine::Uniform<glm::mat4>*>(prog_normals.getUniform("m_proj"))->set(projMatrix);
        });

        // Finally, the render function
        window.setRenderCallback([&] () {
            glm::mat4 cameraMatrix = camera.world_to_camera(),
                      worldMatrix  = worldTransform.matrix();
            current_prog->use();
            buddha->set_transform(worldMatrix);
            dynamic_cast<Engine::Uniform<glm::mat4>*>(current_prog->getUniform("m_camera"))->set(cameraMatrix);
            dynamic_cast<Engine::Uniform<glm::mat3>*>(current_prog->getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(worldMatrix)));

            glm::vec3 pos_cam = camera.transform().position();

            std::cout << "Camera pos : " << pos_cam << std::endl;

            // TODO : this too
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            scene.render(); });

        window.mainLoop();

        delete mesh;
    }

        // TODO : yep, this too
        glfwTerminate();

        return 0;
    }
