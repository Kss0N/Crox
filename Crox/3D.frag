#version 460
#extension GL_EXT_shader_16bit_storage : enable 


layout(location = 0) out vec4 fragColor;


layout(location = 0) in VData
{
	vec3 Pos;
	vec3 Normal;
	vec2 Tex0;
};

 

layout (std140, binding = 1) uniform CameraBlock {
	vec3 u_camPos; 
};

layout (std140, binding = 2) uniform LightingBlock {
	vec3 u_lightPos;
	vec3 u_lightColor; 
};

layout (std140, binding = 3) uniform MaterialBlock 
{
	vec3  u_ambientColor;  
	float u_alpha;		  
	vec3  u_diffuseColor; 
	float u_shininess;	  
	vec3  u_specularColor;
	int _unused;

	//Following are used with Atlas: (for each of them the (x,y) are offsets, (z,w) are scaling factors)

	vec4  
		u_ambientMapLoc,
	    u_diffuseMapLoc,
	   u_specularMapLoc,
	      u_alphaMapLoc,
	  u_shininessMapLoc;
};

layout(binding = 0) uniform sampler2D atlas;

 
void main()
{
	
	// may be sampled from atlas or from material block:
	
	vec3 
		ambientColor = u_ambientColor,
		diffuseColor = u_diffuseColor,
	   specularColor = u_specularColor,
	          normal = Normal;
	float 
		       alpha = u_alpha,
		   shininess = u_shininess;

	// blinn-phong shading:

	vec3 
		lightDir = normalize(u_lightPos - Pos),
		 viewDir = normalize(  u_camPos - Pos),
		 halfDir = normalize(lightDir + viewDir), 
		 reflDir = reflect (-lightDir, normal);

	float 
		diff = max(dot(normal, lightDir), 0.0),
		spec = pow(max(dot(halfDir, reflDir), 0.0), shininess);

	vec3 
		ambient = u_ambientColor * 0.3,
		diffuse = u_diffuseColor * diff,
		specular= u_specularColor*(diff == 0 ? 0.0 : spec);

	vec3 color = (ambient + diffuse + specular) * u_lightColor;
	fragColor = vec4(color, alpha);
}