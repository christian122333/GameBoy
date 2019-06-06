#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

void main()
{
	mat3 projection = mat3(
        vec3(3.0/4.0, 0.0, 0.0),
        vec3(    0.0, -1.0, 0.0),
        vec3(    0.0, 0.0, 1.0)
    );

	gl_Position = vec4(projection * aPos, 1.0);
	TexCoord = aTexCoord;
}