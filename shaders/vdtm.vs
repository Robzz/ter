#version 330

// Run all computations in object space

in vec4 v_position;
in vec3 v_normal;

out vec3 vs_normal;

uniform mat4 m_camera;
uniform mat4 m_world;
uniform mat4 m_proj;
uniform mat3 m_normalTransform;

void main() {
    gl_Position = m_proj * m_camera * m_world * v_position;
    vs_normal = m_normalTransform * v_normal;
}
