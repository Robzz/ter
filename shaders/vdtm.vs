#version 330

// Run all computations in object space

in vec4 v_position;
in vec3 v_normal;

uniform mat4 m_objToCamera;
uniform mat4 m_cameraToObj;
uniform mat4 m_viewpoint1, m_viewpoint2, m_viewpoint3; 

out PovTexCoords {
    vec2 pov1;
    vec2 pov2;
    vec2 pov3;
};
out vec3 vs_pos_object_space;
out vec3 vs_pos_camera_space;
out vec3 vs_normal;

void main() {
    vs_pos_object_space = vec3(v_position);
    gl_Position = m_objToCamera * v_position;
    vs_normal = v_normal;
    pov1 = (m_viewpoint1 * v_position).xy;
    pov2 = (m_viewpoint2 * v_position).xy;
    pov3 = (m_viewpoint3 * v_position).xy;
}
