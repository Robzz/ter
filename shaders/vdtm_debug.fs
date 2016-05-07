#version 330

in vec4 vs_pos_object_space;
in vec4 vs_pos_camera_space;
in vec3 vs_normal;

in PovTexCoords {
    vec2 pov1;
    vec2 pov2;
    vec2 pov3;
};

uniform mat4 m_viewpoint1, m_viewpoint2, m_viewpoint3; 
uniform mat4 m_viewpoint_inverse1, m_viewpoint_inverse2, m_viewpoint_inverse3; 
uniform sampler2D depthTex1, depthTex2, depthTex3;

vec2 clip_to_NDC(vec2 v) {
    // NOTE : this should NOT be hardcoded, the camera to NDC conversion must
    // use the parameters passed to glViewport. These are constant in this app,
    // so we afford the easy way
    return v/2 +0.5;
}

void main() {
    float zPov1 = texture2D(depthTex1, clip_to_NDC(pov1)).r,
          zPov2 = texture2D(depthTex2, clip_to_NDC(pov2)).r,
          zPov3 = texture2D(depthTex3, clip_to_NDC(pov3)).r;
    vec4 p1_h = m_viewpoint1 * vec4(pov1, zPov1, 1),
         p2_h = m_viewpoint2 * vec4(pov2, zPov2, 1),
         p3_h = m_viewpoint3 * vec4(pov3, zPov3, 1);
    vec3 p1 = (p1_h / p1_h.w).xyz,
         p2 = (p2_h / p2_h.w).xyz,
         p3 = (p3_h / p3_h.w).xyz,
         pos_obj = (vs_pos_object_space / vs_pos_object_space.w).xyz;
    if(length(pos_obj - p1) < length(pos_obj - p2)) {
        if(length(pos_obj - p1) < length(pos_obj - p3)) {
            gl_FragColor = vec4(1, 0, 0, 1);
            gl_FragDepth = zPov1;
        }
        else {
            gl_FragColor = vec4(0, 0, 1, 1);
            gl_FragDepth = zPov3;
        }
    }
    else {
        if(length(pos_obj - p2) < length(pos_obj - p3)) {
            gl_FragColor = vec4(0, 1, 0, 1);
            gl_FragDepth = zPov2;
        }
        else {
            gl_FragColor = vec4(0, 0, 1, 1);
            gl_FragDepth = zPov3;
        }
    }
}
