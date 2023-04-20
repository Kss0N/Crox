#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aRGB;


layout (binding = 0) uniform MatrixBlock {
	mat4 u_matrix; //Projection * view * model
};

layout(location = 0) out vec3 color;

void main(){
	gl_Position = u_matrix * vec4(aPos, 1.0f);
	color = aRGB;
}