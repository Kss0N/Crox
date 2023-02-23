#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aRGB;


layout (location = 0) uniform mat4 u_matrix = mat4(
1,0,0,0,
0,1,0,0,
0,0,1,0,
0,0,0,1
); //Projection * view * model


out vec3 color;

void main(){
	gl_Position = u_matrix * vec4(aPos, 1.0f);
	color = aRGB;
}