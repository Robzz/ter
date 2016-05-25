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

using namespace Engine;

typedef std::pair<std::string, ProgramBuilder::UniformType> UniformDescriptor;
struct PovTextures {
    glm::mat4 matrix;
    glm::vec3 position;
    Texture const* colors;
    Texture const* normals;
    Texture const* depth;
    int n;

    PovTextures(glm::mat4 const& mat, glm::vec3 const& pos, Texture const* col, Texture const* norm, Texture const* dep, int n_) :
        matrix(mat),
        position(pos),
        colors(col),
        normals(norm),
        depth(dep),
        n(n_)
    { }

    private:
    PovTextures();
};

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cout << "Usage : " << argv[0] << "<point-of-view-archive> <low-res-mesh>" << std::endl;
        return 1;
    }
    engine_init(argc, argv);

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
        std::cout << "GL_KHR_DEBUG enabled" << std::endl;
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
        si.dropComponents(aiComponent_NORMALS);
        si.readFile(argv[2], SceneImporter::PostProcess::JoinVertices | SceneImporter::PostProcess::RemoveComponent);
        auto mesh = si.instantiateMesh(*si.meshes()[0]);

        // Then, the shader programs
        glm::mat4 projMatrix = glm::perspective<float>(glm::radians(45.f), 1280.f/720.f, 0.1, 1000);
        TransformEuler worldTransform;
        glm::vec3 lightPosition(5, 15, -15);

        std::vector<UniformDescriptor> uniforms, dbg_uniforms;

        Program prog_vdtm = sm.buildProgram().vertexShader("shaders/vdtm.vs")
                                    .fragmentShader("shaders/vdtm.fs")
                                    .uniform("m_objToCamera",        ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_camToClip",          ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_viewpoint1",         ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_viewpoint2",         ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_viewpoint3",         ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_viewpoint_inverse1", ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_viewpoint_inverse2", ProgramBuilder::UniformType::Mat4)
                                    .uniform("m_viewpoint_inverse3", ProgramBuilder::UniformType::Mat4)
                                    .uniform("colorTex1",            ProgramBuilder::UniformType::Int)
                                    .uniform("colorTex2",            ProgramBuilder::UniformType::Int)
                                    .uniform("colorTex3",            ProgramBuilder::UniformType::Int)
                                    .uniform("depthTex1",            ProgramBuilder::UniformType::Int)
                                    .uniform("depthTex2",            ProgramBuilder::UniformType::Int)
                                    .uniform("depthTex3",            ProgramBuilder::UniformType::Int)
                                    .build();
        Program prog_dbg = sm.buildProgram().vertexShader("shaders/vdtm.vs")
                                   .fragmentShader("shaders/vdtm_debug.fs")
                                   .uniform("m_objToCamera",        ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_camToClip",          ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_viewpoint1",         ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_viewpoint2",         ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_viewpoint3",         ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_viewpoint_inverse1", ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_viewpoint_inverse2", ProgramBuilder::UniformType::Mat4)
                                   .uniform("m_viewpoint_inverse3", ProgramBuilder::UniformType::Mat4)
                                   .uniform("colorTex1",            ProgramBuilder::UniformType::Int)
                                   .uniform("colorTex2",            ProgramBuilder::UniformType::Int)
                                   .uniform("colorTex3",            ProgramBuilder::UniformType::Int)
                                   .uniform("depthTex1",            ProgramBuilder::UniformType::Int)
                                   .uniform("depthTex2",            ProgramBuilder::UniformType::Int)
                                   .uniform("depthTex3",            ProgramBuilder::UniformType::Int)
                                   .build();

        window.setResizeCallback([&] (int w, int h) {
            // TODO : wrap the GL call away
            glViewport(0, 0, w, h);
            projMatrix = glm::perspective<float>(45, static_cast<float>(w)/static_cast<float>(h), 0.1, 1000);
        });

        // Load textures from viewpoint archive
        std::ifstream inFile;
        inFile.open(argv[1], std::ios_base::binary | std::ios_base::in);
        boost::archive::binary_iarchive ar(inFile);
        std::vector<Viewpoint> povs;
        ar >> povs;
        std::cout << "Loaded " << povs.size() << " viewpoints from " << argv[1] << std::endl;

        std::vector<PovTextures> pov_textures;
        std::vector<Texture*> textures;
        int i = 0;
        for(auto& pov: povs) {
            textures.push_back(pov.color().to_texture());
            textures.push_back(pov.normal().to_texture());
            textures.push_back(pov.depth().to_depth_texture());
            pov_textures.push_back(PovTextures(pov.matrix(), pov.position(), textures[textures.size()-3], textures[textures.size()-2], textures[textures.size()-1], i));
            ++i;
        }
        std::cout << "POV positions :" << std::endl;
        for(auto& pov: pov_textures)
            std::cout << pov.position << std::endl;

        // Fill the scene
        Camera<TransformMat> camera;
        //camera.look_at(glm::vec3(0, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        camera.translate_local(Direction::Back, 1);
        SceneGraph scene, scene_dbg;
        auto buddha = mesh->instantiate(glm::mat4(1), &prog_vdtm, nullptr);
        scene.addChild(buddha);

        Program* current_prog = &prog_vdtm;
        
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
        window.registerKeyCallback(GLFW_KEY_ESCAPE, [&window] () { window.close(); });
        window.registerKeyCallback(GLFW_KEY_LEFT_CONTROL, [&camera] () { camera.translate_local(Direction::Down, 1); });
        window.registerKeyCallback(' ', [&camera] () { camera.translate_local(Direction::Up, 1); });
        window.registerKeyCallback('W', [&camera] () { camera.translate_local(Direction::Front, 1); });
        window.registerKeyCallback('A', [&camera] () { camera.translate_local(Direction::Left, 1); });
        window.registerKeyCallback('S', [&camera] () { camera.translate_local(Direction::Back, 1); });
        window.registerKeyCallback('D', [&camera] () { camera.translate_local(Direction::Right, 1); });
        window.registerKeyCallback('F', [&] () { current_prog = (current_prog == &prog_dbg) ? &prog_vdtm : &prog_dbg;
                                                 buddha->set_program(current_prog); });

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

        window.registerMouseButtonCallback([] (int button, int action, int mods) {
                std::cout << ((action == GLFW_PRESS) ? "Pressed " : "Released ") << ((button == GLFW_MOUSE_BUTTON_1) ? "left" :
                                                                                     (button == GLFW_MOUSE_BUTTON_2) ? "right" : 
                                                                                     (button == GLFW_MOUSE_BUTTON_3) ? "middle" : "unknown") << " mouse button." << std::endl;
            });

        dynamic_cast<Uniform<glm::mat4>*>(prog_vdtm.getUniform("m_camToClip"))->set(projMatrix);
        dynamic_cast<Uniform<glm::mat4>*>(prog_dbg .getUniform("m_camToClip"))->set(projMatrix);

        // Finally, the render function
        window.setRenderCallback([&] () {
            glm::mat4 cameraMatrix = camera.world_to_camera(),
                      worldMatrix  = worldTransform.matrix();
            current_prog->use();
            buddha->set_transform(worldMatrix);
            // Find 3 closest viewpoints
            glm::vec3 cameraPos = camera.transform().position();
            std::vector<PovTextures const*> pt;
            for(auto& it: pov_textures) {
                pt.push_back(&it);
            }
            std::sort(pt.begin(), pt.end(), [&] (PovTextures const* p1, PovTextures const* p2) { return glm::distance(p1->position, cameraPos) < glm::distance(p2->position, cameraPos); });
            std::cout << "Using viewpoints " << pt[0]->n << ", " << pt[1]->n << " and " << pt[2]->n << std::endl;
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_objToCamera"))->set(cameraMatrix * worldMatrix);
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_viewpoint1"))->set(pt[0]->matrix);
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_viewpoint2"))->set(pt[1]->matrix);
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_viewpoint3"))->set(pt[2]->matrix);
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_viewpoint_inverse1"))->set(glm::inverse(pt[0]->matrix));
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_viewpoint_inverse2"))->set(glm::inverse(pt[1]->matrix));
            dynamic_cast<Uniform<glm::mat4>*>(current_prog->getUniform("m_viewpoint_inverse3"))->set(glm::inverse(pt[2]->matrix));
            glActiveTexture(GL_TEXTURE0 + 1);
            pt[0]->colors->bind();
            dynamic_cast<Uniform<GLint>*>(current_prog->getUniform("colorTex1"))->set(1);
            glActiveTexture(GL_TEXTURE0 + 2);
            pt[1]->colors->bind();
            dynamic_cast<Uniform<GLint>*>(current_prog->getUniform("colorTex2"))->set(2);
            glActiveTexture(GL_TEXTURE0 + 3);
            pt[2]->colors->bind();
            dynamic_cast<Uniform<GLint>*>(current_prog->getUniform("colorTex3"))->set(3);
            glActiveTexture(GL_TEXTURE0 + 4);
            pt[0]->depth->bind();
            dynamic_cast<Uniform<GLint>*>(current_prog->getUniform("depthTex1"))->set(4);
            glActiveTexture(GL_TEXTURE0 + 5);
            pt[1]->depth->bind();
            dynamic_cast<Uniform<GLint>*>(current_prog->getUniform("depthTex2"))->set(5);
            glActiveTexture(GL_TEXTURE0 + 6);
            pt[2]->depth->bind();
            dynamic_cast<Uniform<GLint>*>(current_prog->getUniform("depthTex3"))->set(6);
            glActiveTexture(GL_TEXTURE0);

            // TODO : this too
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            scene.render(); });

        window.mainLoop();

        for(auto tex: textures)
            delete tex;
    }

        // TODO : yep, this too
        glfwTerminate();

        return 0;
    }
