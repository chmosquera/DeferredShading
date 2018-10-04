#version 450 core 

out vec4 color;

in vec3 WorldPos;
in vec2 fragTex;
in vec3 fragNor;


uniform vec3 campos;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;


void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
	
	//diffuse light
	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - WorldPos);
	float light = dot(ld,normal);	
	light = clamp(light,0,1);

	//specular light
	vec3 camvec = normalize(campos - WorldPos);
	vec3 h = normalize(camvec+ld);
	float spec = pow(dot(h,normal),5);
	spec = clamp(spec,0,1);
	
	color.rgb = texturecolor *light + vec3(1,1,1)*spec;
	color.a=1;
}
