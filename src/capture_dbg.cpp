#include <iostream>
#include <fstream>
#include <vector>
#include "gl_core_3_3.h"
#include "debug.h"
#include "troll_engine.h"
#include "image.h"

#include <sstream>
#include "pov.h"

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cout << "Usage : " << argv[0] << "<point-of-view-archive> <debug-output-dir>" << std::endl;
        return 1;
    }
    Engine::engine_init(argc, argv);

    FreeImage_SetOutputMessage([] (FREE_IMAGE_FORMAT fif, const char *message) {
            if(fif != FIF_UNKNOWN) {
            std::cerr << "Format : " << FreeImage_GetFormatFromFIF(fif) << std::endl;
            }
            std::cerr << (message) << std::endl; });

    // Load textures from viewpoint archive
    std::ifstream inFile;
    inFile.open(argv[1], std::ios_base::binary | std::ios_base::in);
    boost::archive::binary_iarchive ar(inFile);
    std::vector<Viewpoint> povs;
    ar >> povs;
    std::cout << "Loaded " << povs.size() << " viewpoints from " << argv[1] << std::endl;

    std::vector<Engine::Texture*> textures;
    std::string dir(argv[2]),
                colortex_prefix(dir + "/colorTex_"),
                normaltex_prefix(dir + "/normalTex_"),
                depthtex_prefix(dir + "/depthTex_");
    int i = 0;
    for(auto& pov: povs) {
        std::cout << "POV number " << i << std::endl << pov.position() << std::endl;
        std::string suffix = std::to_string(i) + ".png";
        pov.color().save(colortex_prefix + suffix, Engine::Image::Format::Png);
        pov.normal().save(normaltex_prefix + suffix, Engine::Image::Format::Png);
        pov.depth().save(depthtex_prefix + suffix, Engine::Image::Format::Png);
        ++i;
    }

    for(auto tex: textures)
        delete tex;

    return 0;
}
