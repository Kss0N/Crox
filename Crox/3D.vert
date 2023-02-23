#version 460 core

layout(location = 0) in vec3 aXYZ;
//layout(location = 1) in vec3 aIJK;
//layout(location = 2) in vec2 aUV;

layout(location = 0) uniform mat4 u_matrix =  mat4(
1,0,0,0,
0,1,0,0,
0,0,1,0,
0,0,0,1
); // = Projection * View * Model 

void main()
{
	gl_Position = u_matrix * vec4(aXYZ, 1.0f);
}