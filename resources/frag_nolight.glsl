#version 450 core 

out vec4 color;

in vec2 fragTex;

layout(location = 0) uniform sampler2D tex;

void main()
{
	vec3 texturecolor = texture(tex, fragTex, 0).rgb;
	float xp = 1.0/1980.0;

	// ----------
	// do blur
	// ----------
	/*
	texturecolor.r = pow(texturecolor.r, 1);
	texturecolor.g = pow(texturecolor.g, 1);
	texturecolor.b = pow(texturecolor.b, 1);
	*/

	//**********
	// smear
	//**********
	/*for (int i = -50; i <= 50; i++) {
		if (i == 0) continue;
		vec3 col = texture(tex, fragTex + vec2(float(i) * xp, 0.0), 0).rgb;
		texturecolor += col * (1.0/float(i));
	}*/


	//**********
	// gauss
	//**********
	float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 }; 
	
	for (int i = -4; i <= 4; i++) {
		if (i == 0) continue;		
		vec3 col = texture(tex, fragTex + vec2(float(i) * xp, 0.0), 0).rgb;
		int wi = abs(i);
		texturecolor += col * weights[wi];
	}

	
	color.rgb = texturecolor;
	color.a=1;
}
