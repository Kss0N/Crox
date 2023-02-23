#version 460 core

layout(location = 0) out vec4 fragColor;

void main()
{
	vec3 color = vec3(1.0f, 1.0f, 1.0f);
	fragColor = vec4(color, 1.0f);
}