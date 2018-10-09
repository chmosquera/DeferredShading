#version  450 core
//layout(location = 0) out vec4 gColor;
//layout(location = 1) out vec4 gViewpos;
//layout(location = 2) out vec4 gNormal;

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;

uniform vec3 campos;

layout(location = 0) uniform sampler2D tex;

out vec4 color;

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
	
	//diffuse light
	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - fragPos);
	float light = dot(ld,normal);	
	light = clamp(light,0,1);

	//specular light
	vec3 camvec = normalize(campos - fragPos);
	vec3 h = normalize(camvec+ld);
	float spec = pow(dot(h,normal),5);
	spec = clamp(spec,0,1)*0.3;
	
	color.rgb = texturecolor *light + vec3(1,1,1)*spec;
	color.a=1;
}