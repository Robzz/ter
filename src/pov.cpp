#include "pov.h"

Viewpoint::Viewpoint() :
    m_matrix(),
    m_position(),
    m_colorMap(),
    m_depthMap(),
    m_normalMap()
{ }

Viewpoint::Viewpoint(glm::mat4 const& matrix, glm::vec3 const& pos, Engine::Image const& colorMap, Engine::Image const& depthMap, Engine::Image const& normalMap) :
    m_matrix(matrix),
    m_position(pos),
    m_colorMap(colorMap),
    m_depthMap(depthMap),
    m_normalMap(normalMap)
{ }

Viewpoint::~Viewpoint() { }

glm::mat4 const& Viewpoint::matrix()     { return m_matrix; }
glm::vec3 const& Viewpoint::position()   { return m_position; }
Engine::Image const& Viewpoint::color()  { return m_colorMap; }
Engine::Image const& Viewpoint::depth()  { return m_depthMap; }
Engine::Image const& Viewpoint::normal() { return m_normalMap; }
