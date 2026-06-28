/***********************************************************************
LightingShader.fs - Fragment program for the lighting shader.
Copyright (c) 2026 Oliver Kreylos
***********************************************************************/

uniform sampler2D normalVectorTexture;
uniform mat3 normalVectorMatrix;
uniform vec3 lightDirection;

varying vec2 texCoord;

void main()
	{
	/* Retrieve the normal vector and apply the normal vector matrix: */
	vec3 normal=normalize(normalVectorMatrix*texture2D(normalVectorTexture,texCoord).xyz);
	
	/* Evaluate the Phong lighting equation: */
	vec3 ambientc=vec3(0.1,0.1,0.1);
	
	float diff=max(dot(normal,lightDirection),0.0);
	vec3 diffc=vec3(0.5,0.5,0.5)*diff;
	
	float spec=diff>0.0?max(dot(reflect(lightDirection,normal),vec3(0.0,0.0,-1.0)),0.0):0.0;
	vec3 specc=vec3(0.5,0.5,0.5)*pow(spec,64.0);
	
	gl_FragColor=vec4(ambientc+diffc+specc,1.0);
	}
