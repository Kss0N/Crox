#version 460 core

layout(location = 0) out vec4 fragColor;


in VData
{
	vec3 pos;
	vec3 normal;
	vec2 tex;
}fin;

// XYZs 

uniform vec3 u_camPos; 
uniform vec3 u_lightPos;

// RGBs

uniform vec3 u_lightColor;
uniform vec3 u_ambientColor = vec3(0.1, 0.1, 0.1); 
uniform vec3 u_diffuseColor = vec3(0.8, 0.8, 0.8);
uniform vec3 u_specularColor= vec3(0.8, 0.8, 0.8);

uniform float u_alpha = 1.0f;
uniform float u_shininess = 1.0f;

void main()
{
	vec3 color = vec3(1.0f, 1.0f, 1.0f);

	vec3 lightDir = normalize(u_lightPos - fin.pos);
	vec3 viewDir = normalize(u_camPos-fin.pos);
	vec3 reflDir = reflect(-lightDir, fin.normal);
	float diff = max(dot(fin.normal, lightDir), 1.0f);
	float spec = pow(max(dot(viewDir, reflDir), 0.0), u_shininess);

	vec3 ambient = .1*u_ambientColor * u_lightColor;
	vec3 diffuse = u_diffuseColor * diff;
	vec3 specular= u_specularColor* spec * u_lightColor;

	color = (ambient + diffuse + specular) * color;
	fragColor = vec4(color, u_alpha);
}