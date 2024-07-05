
#version 460

layout(location = 0) in vec3 OutPosition;
layout(location = 1) in vec3 OutNormal;

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform FPushConsts {
	layout(offset = 128) vec3 Color;
} PushConsts;


void main()
{
	vec3 LightDirection = normalize(vec3(0.0f, -1.0f, 0.0f));

	float Mod = dot(normalize(OutNormal), LightDirection);
	vec3 Color = PushConsts.Color * ((Mod * 0.5f) + 0.5f);

	FragColor = vec4(Color, 1.0f) ;
}
