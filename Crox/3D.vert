#version 460 core

layout(location = 0) in vec3 aXYZ;
layout(location = 1) in vec3 aIJK;
layout(location = 2) in vec2 aUV;

layout(location = 0) uniform mat4 u_matrix =  mat4(
1,0,0,0,
0,1,0,0,
0,0,1,0,
0,0,0,1
); // = Projection * View * Model 
layout(location = 1) uniform mat4 u_model = mat4(
1,0,0,0,
0,1,0,0,
0,0,1,0,
0,0,0,1);
layout(location = 2) uniform mat3 u_normalMatrix = mat3(
1,0,0,
0,1,0,
0,0,1); // = mat3(transpose(inverse(model)))

out VData 
{
	vec3 pos;
	vec3 normal;
	vec2 tex;
} vout;

void main()
{
	gl_Position = u_matrix * vec4(aXYZ, 1.0f);

	vout.pos = vec3(u_model * vec4(aXYZ,1));
	vout.normal = normalize(u_normalMatrix * (aIJK));
	vout.tex = aUV;
}