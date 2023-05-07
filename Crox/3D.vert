#version 460

layout(location = 0) in vec3 aXYZ;
layout(location = 1) in vec3 aIJK;
layout(location = 2) in vec2 aUV;

layout (std140, binding = 0) uniform MatrixBlock
{
	mat4 u_matrix; 	// Projection * View * Model matrices
	mat4 u_model;	// model matrix
	mat4 u_normal;	// normal = inverse(transpose(model)) matrix
};



layout (location = 0) out VData 
{
	vec3 Pos;		// Vertex pos in world space
	vec3 Normal;	// Normal transformed into world space
	vec2 Tex0;		// Texcoord 
} ;

void main()
{
	gl_Position = u_matrix * vec4(aXYZ, 1.0);

	Pos = vec3(u_model * vec4(aXYZ,1));
	Normal = normalize( mat3(u_normal) * aIJK );
	Tex0 = aUV;
}