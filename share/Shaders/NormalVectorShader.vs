/***********************************************************************
NormalVectorShader.vs - Vertex program for the normal vector shader.
Copyright (c) 2026 Oliver Kreylos
***********************************************************************/

varying vec2 texCoord;

void main()
	{
	/* Pass through vertex texture coordinates: */
	texCoord=gl_MultiTexCoord0.xy;
	
	/* Use standard vertex position: */
	gl_Position=ftransform();
	}
