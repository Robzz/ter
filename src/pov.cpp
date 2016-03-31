#include "pov.h"

Viewpoint::Viewpoint() :
    m_colorMap(),
    m_depthMap(),
    m_normalMap()
{ }

Viewpoint::Viewpoint(Engine::Image const& colorMap, Engine::Image const& depthMap, Engine::Image const& normalMap) :
    m_colorMap(colorMap),
    m_depthMap(depthMap),
    m_normalMap(normalMap)
{ }

Viewpoint::~Viewpoint() { }

Engine::Image const& Viewpoint::color()  { return m_colorMap; }
Engine::Image const& Viewpoint::depth()  { return m_depthMap; }
Engine::Image const& Viewpoint::normal() { return m_normalMap; }
