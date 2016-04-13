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
    Viewpoint(Engine::Image const& colorMap, Engine::Image const& depthMap, Engine::Image const& normalMap);
    ~Viewpoint();

    Engine::Image const& color();
    Engine::Image const& depth();
    Engine::Image const& normal();

protected:
    glm::vec3     m_position;
    Engine::Image m_colorMap;
    Engine::Image m_depthMap;
    Engine::Image m_normalMap;

private:
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version) {
            ar & m_position.x & m_position.y & m_position.z & m_colorMap & m_depthMap & m_normalMap;
    }
};

#endif // POV_H
