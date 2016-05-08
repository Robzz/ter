#version 330

// Run all computations in object space

in vec4 v_position;
in vec3 v_normal;

uniform mat4 m_camToClip;
uniform mat4 m_objToCamera;
uniform mat4 m_viewpoint1, m_viewpoint2, m_viewpoint3; 

out PovTexCoords {
    vec2 pov1;
    vec2 pov2;
    vec2 pov3;
};
out vec4 vs_pos_object_space;
out vec3 vs_normal;

vec2 pos_to_uv(vec4 p, mat4 m) {
    p = m * p;
    p = p / p.w;
    //return p.xy;
    return (p.xy)/2 + 0.5;
}

void main() {
    vs_pos_object_space = v_position;
    gl_Position = m_camToClip * m_objToCamera * v_position;
    vs_normal = v_normal;
    pov1 = pos_to_uv(v_position, m_viewpoint1);
    pov2 = pos_to_uv(v_position, m_viewpoint2);
    pov3 = pos_to_uv(v_position, m_viewpoint3);
}
