#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "gl_core_3_3.h"
#include "debug.h"
#include "mcgyver.h"
#include "window.h"
#include "vbo.h"
#include "vao.h"
#include "program.h"
#include "scenegraph.h"
#include "camera.h"
#include "texture.h"
#include "planet.h"
#include "obj.h"
#include "fbo.h"
#include "image.h"
#include "transform.h"

#include "pov.h"

typedef std::pair<std::string, Engine::ProgramBuilder::UniformType> UniformDescriptor;

void init_libs(int argc, char** argv);
Engine::Program buildShaderProgram(std::string const& vs_file, std::string const& fs_file, std::vector<UniformDescriptor> const& uniforms);
void bind_input_callbacks(Engine::Window& window, Engine::Camera<Engine::TransformEuler>& cam, Engine::TransformEuler& worldTransform);

// TODO : wrap this in the lib
// Initialize GLEW and GLFW
void init_libs(int argc, char** argv) {
    if (!glfwInit()) {
        std::cerr << "Error : cannot initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback([] (int error, const char* description) { std::cerr << description << std::endl; });
}

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
              Engine::Object& buddha,
              Engine::Window& window,
              Engine::Camera<Engine::TransformEuler>& camera,
              Engine::Program& colorProg, Engine::Program& normalProg,
              Engine::FBO& fbo,
              Engine::Texture& colorTex, Engine::Texture& normalTex, Engine::Texture& depthTex) {
    // Render color and depth to texture
    colorProg.use();
    dynamic_cast<Engine::Uniform<glm::mat4>*>(colorProg.getUniform("m_camera"))->set(camera.world_to_camera());
    dynamic_cast<Engine::Uniform<glm::mat3>*>(colorProg.getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(1)));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene.render();
    window.swapBuffers();

    fbo.attach(Engine::FBO::Read, Engine::FBO::Color, colorTex);
    std::vector<unsigned char> color(Engine::FBO::readPixels<unsigned char>(Engine::FBO::Bgr, Engine::FBO::Ubyte, window.width(), window.height()));

    Engine::Image colorImg(Engine::Image::from_rgb(color, window.width(), window.height()));
    Engine::Image depthImg(Engine::Image::from_greyscale<unsigned char>(Engine::FBO::readPixels<unsigned char>(Engine::FBO::DepthComponent, Engine::FBO::Ubyte, window.width(), window.height()),
                           window.width(), window.height()));

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
    std::vector<unsigned char> normal(Engine::FBO::readPixels<unsigned char>(Engine::FBO::Bgr, Engine::FBO::Ubyte, window.width(), window.height()));
    Engine::Image normalImg(Engine::Image::from_rgb(normal, window.width(), window.height()));
    
    povs.push_back(Viewpoint(colorImg, depthImg, normalImg));
    
    // Restore previous state
    Engine::FBO::bind_default(Engine::FBO::Both);
    buddha.set_program(&colorProg);
}

void bind_input_callbacks(Engine::Window& window, Engine::Camera<Engine::TransformEuler>& cam, Engine::TransformEuler& worldTransform) {
    window.registerKeyCallback(GLFW_KEY_ESCAPE, [&window] () { window.close(); });
    window.registerKeyCallback(GLFW_KEY_LEFT_CONTROL, [&cam] () { cam.translate_local(Engine::Direction::Down, 1); });
    window.registerKeyCallback(' ', [&cam] () { cam.translate_local(Engine::Direction::Up, 1); });
    window.registerKeyCallback('W', [&cam] () { cam.translate_local(Engine::Direction::Front, 1); });
    window.registerKeyCallback('A', [&cam] () { cam.translate_local(Engine::Direction::Left, 1); });
    window.registerKeyCallback('S', [&cam] () { cam.translate_local(Engine::Direction::Back, 1); });
    window.registerKeyCallback('D', [&cam] () { cam.translate_local(Engine::Direction::Right, 1); });
    window.registerKeyCallback('Q', [&cam] () { cam.rotate_local(Engine::Axis::Z, -0.15); });
    window.registerKeyCallback('E', [&cam] () { cam.rotate_local(Engine::Axis::Z, 0.15); });

    window.registerMousePosCallback([&worldTransform] (double x, double y) {
            static float prev_x = 0, prev_y = 0;
            float xoffset = x - prev_x, yoffset = y - prev_y;
            prev_x = x; prev_y = y;
            const float f = 0.01;
            xoffset *= f;
            yoffset *= f;
            worldTransform.rotate_local(Engine::Axis::Y, xoffset);
            worldTransform.rotate_local(Engine::Axis::X, yoffset);
        });

    window.registerMouseButtonCallback([&cam] (int button, int action, int mods) {
            std::cout << ((action == GLFW_PRESS) ? "Pressed " : "Released ") << ((button == GLFW_MOUSE_BUTTON_1) ? "left" :
                                                                                 (button == GLFW_MOUSE_BUTTON_2) ? "right" : 
                                                                                 (button == GLFW_MOUSE_BUTTON_3) ? "middle" : "unknown") << " mouse button." << std::endl;
        });
}

int main(int argc, char** argv) {
    Engine::mcgyver_init(argc, argv);
    Engine::Obj obj = Engine::ObjReader().file("assets/buddha_11k.obj").read();
    Engine::Mesh* mesh = obj.get_group("default");

    std::cout << "Got " << obj.groups().size() << std::endl;
    for(auto& p: obj.groups()) {
        std::cout << "Group " << p.first << " :" << std::endl
                  << p.second->get_attribute<glm::vec4>("vertices")->size()   << " vertices"  << std::endl
                  << p.second->get_attribute<glm::vec2>("texCoords")->size()  << " texCoords" << std::endl
                  << p.second->get_attribute<glm::vec3>("normals")->size()    << " normals"   << std::endl
                  << p.second->get_attribute<unsigned int>("indices")->size() << " indices"   << std::endl;
    }

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

        window.setResizeCallback([&] (int w, int h) {
            // TODO : wrap the GL call away
            glViewport(0, 0, w, h);
            projMatrix = glm::perspective<float>(45, static_cast<float>(w)/static_cast<float>(h), 0.1, 1000);
            dynamic_cast<Engine::Uniform<glm::mat4>*>(prog_phong.getUniform("m_proj"))->set(projMatrix);
            dynamic_cast<Engine::Uniform<glm::mat4>*>(prog_normals.getUniform("m_proj"))->set(projMatrix);
        });

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
        Engine::Camera<Engine::TransformEuler> camera;
        //camera.look_at(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        camera.translate_local(Engine::Direction::Back, 1);
        Engine::SceneGraph scene;
        Engine::IndexedObject* buddha = new Engine::IndexedObject(glm::mat4(1), &prog_phong, &indices, &vao_phong,
                                                                  mesh->get_attribute<unsigned int>("indices")->size(), Engine::Texture::noTexture(),
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

        // Install input callbacks
        bind_input_callbacks(window, camera, worldTransform);

        // Setup render to texture
        Engine::Texture colorTex, depthTex, normalTex;
        colorTex.texData (GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, window.width(), window.height(), nullptr);
        depthTex.texData (GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT, window.width(), window.height(), nullptr);
        normalTex.texData(GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, window.width(), window.height(), nullptr);
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

        // Finally, the render function
        window.setRenderCallback([&] () {
            glm::mat4 cameraMatrix = camera.world_to_camera(),
                      worldMatrix  = worldTransform.matrix();
            current_prog->use();
            buddha->set_transform(worldMatrix);
            dynamic_cast<Engine::Uniform<glm::mat4>*>(current_prog->getUniform("m_camera"))->set(cameraMatrix);
            dynamic_cast<Engine::Uniform<glm::mat3>*>(current_prog->getUniform("m_normalTransform"))->set(glm::inverseTranspose(glm::mat3(worldMatrix)));

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
