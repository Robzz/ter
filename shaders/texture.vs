#version 330

in vec3 v_position;
in vec2 v_texCoord;

out vec2 vs_texCoord;

uniform mat4 m_camera;
uniform mat4 m_world;
uniform mat4 m_proj;

void main() {
    gl_Position = m_proj * m_camera * m_world * vec4(v_position, 1);
    vs_texCoord = v_texCoord;
}
