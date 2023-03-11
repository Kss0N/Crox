#version 460 core

layout(location = 0) out vec4 fragColor;


in VData
{
	vec3 pos;
	vec3 normal;
	vec2 tex;
}fin;

 

uniform vec3 u_camPos; 


uniform vec3 u_lightPos;
uniform vec3 u_lightColor; 

struct Material{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float alpha;
	float shininess;
};

struct Light{
	vec3 pos;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform vec3 u_ambientColor = vec3(0.1, 0.1, 0.1); 
uniform vec3 u_diffuseColor = vec3(0.8, 0.8, 0.8);
uniform vec3 u_specularColor= vec3(0.8, 0.8, 0.8);

uniform float u_alpha = 1.0;
uniform float u_shininess = 1.0;

void main()
{
	vec3 
		lightDir= normalize(u_lightPos - fin.pos),
		viewDir = normalize(u_camPos-fin.pos),
		halfDir = normalize(lightDir + viewDir), 
		reflDir = reflect(-lightDir, fin.normal);

	float 
		diff = max(dot(fin.normal, lightDir), 0.0),
		spec = pow(max(dot(halfDir, fin.normal), 0.0), u_shininess);


	vec3 ambient = u_ambientColor*0.25;
	vec3 diffuse = u_diffuseColor * diff;
	vec3 specular= u_specularColor* (diff == 0 ? 0.0 : spec);

	vec3 color = (ambient + diffuse + specular) * u_lightColor;
	fragColor = vec4(color, u_alpha);
}