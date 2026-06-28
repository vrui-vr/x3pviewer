/***********************************************************************
NormalVectorShader.fs - Fragment program for the normal vector shader.
Copyright (c) 2026 Oliver Kreylos
***********************************************************************/

uniform sampler2D normalVectorTexture;
uniform mat3 normalVectorMatrix;

varying vec2 texCoord;

void main()
	{
	/* Retrieve the normal vector and apply the normal vector matrix: */
	vec3 normal=normalVectorMatrix*texture2D(normalVectorTexture,texCoord).xyz;
	
	/* Encode the normal vector as an RGB color: */
	vec3 color=normal*(1.0/length(normal))+vec3(0.5,0.5,0.5);
	// vec3 color=vec3(normal.xy*0.5+vec2(0.5,0.5),0.5);
	
	gl_FragColor=vec4(color,1.0);
	}
