#ifndef POV_H
#define POV_H

#include "image.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <glm/glm.hpp>
#include <string>

class Viewpoint {
public:
    friend boost::serialization::access;
    Viewpoint();
    Viewpoint(glm::mat4 const& position, Engine::Image const& colorMap, Engine::Image const& depthMap, Engine::Image const& normalMap);
    ~Viewpoint();

    glm::mat4     const& position();
    Engine::Image const& color();
    Engine::Image const& depth();
    Engine::Image const& normal();

protected:
    glm::mat4     m_position;
    Engine::Image m_colorMap;
    Engine::Image m_depthMap;
    Engine::Image m_normalMap;

private:
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
            ar & m_position[0].x & m_position[0].y & m_position[0].z & m_position[0].w
               & m_position[1].x & m_position[1].y & m_position[1].z & m_position[1].w
               & m_position[2].x & m_position[2].y & m_position[2].z & m_position[2].w
               & m_position[3].x & m_position[3].y & m_position[3].z & m_position[3].w
               & m_colorMap & m_depthMap & m_normalMap;
    }
};

#endif // POV_H
