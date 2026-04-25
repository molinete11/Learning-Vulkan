
#version 450


layout (location = 0) in vec3 vertCords;
layout(location = 0) out vec3 fragColor;


void main(){
    gl_Position = vec4(vertCords.xyz, 1.0);
    fragColor = vec3(1.0, 1.0, 1.0);
}