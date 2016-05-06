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
    vs_pos_camera_space = vec3(m_objToCamera * v_position);
    vs_normal = v_normal;
    vec3 p1 = ;
}
