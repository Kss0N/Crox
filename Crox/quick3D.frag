#version 460

layout(location = 0) out vec4 fragColor;


layout(location = 0) in VData
{
	vec3 pos;
	vec3 normal;
	vec2 tex;
}fin;

 

layout (std140, binding = 1) uniform CameraBlock {
	vec3 u_camPos; 
};

layout (std140, binding = 2) uniform LightingBlock {
	vec3 u_lightPos;
	vec3 u_lightColor; 
};

vec3 u_ambientColor  = vec3(0.1, 0.1, 0.1); 
vec3 u_diffuseColor  = vec3(0.8, 0.8, 0.8);
vec3 u_specularColor = vec3(0.8, 0.8, 0.8);
float u_alpha		  = 1.0;
float u_shininess	  = 1.45;
 
void main()
{
	vec3 
		lightDir= normalize(u_lightPos - fin.pos),
		viewDir = normalize(u_camPos-fin.pos),
		halfDir = normalize(lightDir + viewDir), 
		reflDir = reflect(-lightDir, fin.normal);

	float 
		diff = max(dot(fin.normal, lightDir), 0.0),
		spec = pow(max(dot(halfDir, reflDir), 0.0), u_shininess);


	vec3 
		ambient = u_ambientColor*0.3,
		diffuse = u_diffuseColor * diff,
		specular= u_specularColor* (diff == 0 ? 0.0 : spec);

	vec3 color = (ambient + diffuse + specular) * u_lightColor;
	fragColor = vec4(color, u_alpha);
}