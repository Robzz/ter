#version 330

in vec3 vs_pos_object_space;
in vec3 vs_pos_camera_space;
in vec3 vs_normal;

in PovTexCoords {
    vec2 pov1;
    vec2 pov2;
    vec2 pov3;
};

uniform mat4 m_viewpoint1, m_viewpoint2, m_viewpoint3; 
uniform mat4 m_viewpoint_inverse1, m_viewpoint_inverse2, m_viewpoint_inverse3; 
uniform sampler2D colorTex1, colorTex2, colorTex3;
uniform sampler2D normalTex1, normalTex2, normalTex3;
uniform sampler2D depthTex1, depthTex2, depthTex3;

void main() {
    gl_FragColor = vec4(1,1,1,1);
}
