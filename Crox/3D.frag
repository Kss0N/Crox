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
	     u_normalMapLoc,
	      u_alphaMapLoc,
	  u_shininessMapLoc;
};

layout(binding = 0) uniform sampler2D atlas;


vec4 getColor(vec4 mapInfo, vec4 defColor)
{
	// (width, height) == (0,0) means there's no texture
	if(mapInfo[2] == 0.0) return defColor;

	return texture(atlas, mapInfo.xy + mapInfo.zw * Tex0.xy);
}
float getAlpha(vec4 mapInfo, float def)
{
	return (mapInfo[2] == 0.0) ? def : 1 - texture(atlas, mapInfo.xy + mapInfo.zw * Tex0.xy).x;
}
 
void main()
{
	
	// may be sampled from atlas or from material block:
	
	vec3 
		ambientColor = getColor( u_ambientMapLoc, vec4( u_ambientColor, 0)).xyz,
		diffuseColor = getColor( u_diffuseMapLoc, vec4( u_diffuseColor, 0)).xyz,
	   specularColor = getColor(u_specularMapLoc, vec4(u_specularColor, 0)).xyz,
	          normal = Normal;
	float 
		       alpha = getAlpha(u_alphaMapLoc, u_alpha),
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
		ambient = ambientColor * 0.3,
		diffuse = diffuseColor * diff,
		specular= specularColor*(diff == 0 ? 0.0 : spec);

	vec3 color = (ambient + diffuse + specular) * u_lightColor;
	fragColor = vec4(color, alpha);
}