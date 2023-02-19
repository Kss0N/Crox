#version 460 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aRGB;

out vec3 color;

void main(){
	gl_Position = vec4(aPos, 0, 1.0f);
	color = aRGB;
}