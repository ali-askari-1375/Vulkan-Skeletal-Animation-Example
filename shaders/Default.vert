
#version 460

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;


layout(push_constant) uniform FPushConsts {
	mat4 ProjectionView;
	mat4 Model;
} PushConsts;

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec3 OutNormal;

void main()
{
	gl_Position = PushConsts.ProjectionView * PushConsts.Model * vec4(Position, 1.0f);
	OutPosition = vec3(gl_Position) * gl_Position.w;
	OutNormal = mat3(PushConsts.Model) * Normal;
}


