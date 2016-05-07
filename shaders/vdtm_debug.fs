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
uniform sampler2D depthTex1, depthTex2, depthTex3;

void main() {
    float zPov1 = texture2D(depthTex1, pov1).r,
          zPov2 = texture2D(depthTex2, pov2).r,
          zPov3 = texture2D(depthTex3, pov3).r;
    vec3 p1 = vec3(m_viewpoint_inverse1 * vec4(pov1, zPov1, 1)),
         p2 = vec3(m_viewpoint_inverse2 * vec4(pov2, zPov2, 1)),
         p3 = vec3(m_viewpoint_inverse3 * vec4(pov3, zPov3, 1));
    if(length(vs_pos_object_space - p1) < length(vs_pos_object_space - p2)) {
        if(length(vs_pos_object_space - p1) < length(vs_pos_object_space - p3)) {
            gl_FragColor = vec4(1, 0, 0, 1);
            gl_FragDepth = zPov1;
        }
        else {
            gl_FragColor = vec4(0, 0, 1, 1);
            gl_FragDepth = zPov3;
        }
    }
    else {
        if(length(vs_pos_object_space - p2) < length(vs_pos_object_space - p3)) {
            gl_FragColor = vec4(0, 1, 0, 1);
            gl_FragDepth = zPov2;
        }
        else {
            gl_FragColor = vec4(0, 0, 1, 1);
            gl_FragDepth = zPov3;
        }
    }
}
