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
#include "planet.h"
#include "obj.h"
#include "fbo.h"
#include "image.h"
#include "transform.h"

#include "pov.h"

typedef std::pair<std::string, Engine::ProgramBuilder::UniformType> UniformDescriptor;
struct PovTextures {
    glm::mat4 position;
    Engine::Texture const& colors;
    Engine::Texture const& normals;
    Engine::Texture const& depth;

    PovTextures(glm::mat4 const& pos, Engine::Texture const& col, Engine::Texture const& norm, Engine::Texture const& dep) noexcept :
        position(pos),
        colors(col),
        normals(norm),
        depth(dep)
    { }

    private:
    PovTextures();
    void operator=(PovTextures const&);
};

void init_libs(int argc, char** argv);
Engine::Program buildShaderProgram(std::string const& vs_file, std::string const& fs_file, std::vector<UniformDescriptor> const& uniforms);
void bind_input_callbacks(Engine::Window& window, Engine::Camera<Engine::TransformEuler>& cam, Engine::TransformEuler& worldTransform);

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
    if(argc != 3) {
        std::cout << "Usage : " << argv[0] << "<point-of-view-archive> <low-res-mesh>" << std::endl;
        return 1;
    }
    Engine::engine_init(argc, argv);
    Engine::Obj obj = Engine::ObjReader().file(argv[2]).read();
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

        std::vector<UniformDescriptor> uniforms;

        uniforms.push_back(UniformDescriptor("m_objToCamera",        Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_cameraToObj",        Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_viewpoint1",         Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_viewpoint2",         Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_viewpoint3",         Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_viewpoint_inverse1", Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_viewpoint_inverse2", Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("m_viewpoint_inverse3", Engine::ProgramBuilder::mat4));
        uniforms.push_back(UniformDescriptor("colorTex1",            Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("colorTex2",            Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("colorTex3",            Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("normalTex1",           Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("normalTex2",           Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("normalTex3",           Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("depthTex1",            Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("depthTex2",            Engine::ProgramBuilder::int_));
        uniforms.push_back(UniformDescriptor("depthTex3",            Engine::ProgramBuilder::int_));
        Engine::Program prog_vdtm(buildShaderProgram("shaders/vdtm.vs", "shaders/vdtm.fs", uniforms));

        window.setResizeCallback([&] (int w, int h) {
            // TODO : wrap the GL call away
            glViewport(0, 0, w, h);
            projMatrix = glm::perspective<float>(45, static_cast<float>(w)/static_cast<float>(h), 0.1, 1000);
        });

        // Setup vertex attributes
        Engine::VAO vao_vdtm, vao_normals;
        GLuint posIndex    = static_cast<unsigned int>(prog_vdtm.getAttributeLocation("v_position"));
        GLuint normalIndex = static_cast<unsigned int>(prog_vdtm.getAttributeLocation("v_normal"));
        vao_vdtm.enableVertexAttribArray(posIndex);
        vao_vdtm.vertexAttribPointer(coords, posIndex, 4, 0, 0);
        vao_vdtm.enableVertexAttribArray(normalIndex);
        vao_vdtm.vertexAttribPointer(normals, normalIndex, 3, 0, 0);

        // Load textures from viewpoint archive
        std::ifstream inFile;
        inFile.open(argv[1], std::ios_base::binary | std::ios_base::in);
        boost::archive::binary_iarchive ar(inFile);
        std::vector<Viewpoint> povs;
        ar >> povs;
        std::cout << "Loaded " << povs.size() << " viewpoints from " << argv[1] << std::endl;

        std::vector<PovTextures> pov_textures;
        std::vector<Engine::Texture> textures;
        for(auto& pov: povs) {
            textures.push_back(pov.color().to_texture());
            textures.push_back(pov.normal().to_texture());
            textures.push_back(pov.depth().to_texture());
            pov_textures.push_back(PovTextures(pov.position(), textures[textures.size()-3], textures[textures.size()-2], textures[textures.size()-1]));
                                              
        }

        // Fill the scene
        Engine::Camera<Engine::TransformEuler> camera;
        //camera.look_at(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        camera.translate_local(Engine::Direction::Back, 1);
        Engine::SceneGraph scene;
        Engine::IndexedObject* buddha = new Engine::IndexedObject(glm::mat4(1), &prog_vdtm, &indices, &vao_vdtm,
                                                                  mesh->get_attribute<unsigned int>("indices")->size(), Engine::Texture::noTexture(),
                                                                  GL_UNSIGNED_INT);
        scene.addChild(buddha);

        Engine::Program* current_prog = &prog_vdtm;
        Engine::VAO* current_vao = &vao_vdtm;
        
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
