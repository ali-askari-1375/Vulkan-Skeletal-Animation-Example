
#version 460

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 Uv;
layout(location = 3) in uvec4 JointIndices0;
layout(location = 4) in vec4 JointWeights0;
layout(location = 5) in uvec4 JointIndices1;
layout(location = 6) in vec4 JointWeights1;


layout(std430, set = 0, binding = 0) readonly buffer FJointMatrices {
	mat4 JointMatrices[];
};

layout(push_constant) uniform FPushConsts {
	mat4 ProjectionView;
	mat4 Model;
} PushConsts;

layout(location = 0) out vec3 OutPosition;
layout(location = 1) out vec3 OutNormal;

void main()
{
	mat4 SkinMat =
		JointWeights0.x * JointMatrices[JointIndices0.x] +
		JointWeights0.y * JointMatrices[JointIndices0.y] +
		JointWeights0.z * JointMatrices[JointIndices0.z] +
		JointWeights0.w * JointMatrices[JointIndices0.w] +
		JointWeights1.x * JointMatrices[JointIndices1.x] +
		JointWeights1.y * JointMatrices[JointIndices1.y] +
		JointWeights1.z * JointMatrices[JointIndices1.z] +
		JointWeights1.w * JointMatrices[JointIndices1.w];


	gl_Position = PushConsts.ProjectionView * PushConsts.Model * SkinMat * vec4(Position, 1.0f);
	OutPosition = vec3(gl_Position) * gl_Position.w;
	OutNormal = mat3(PushConsts.Model * SkinMat) * Normal;
}


