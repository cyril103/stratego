#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
out vec3 normal;
out vec4 color;
out vec3 worldPosition;
out vec2 texCoord;
void main() {
    normal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    color = vertexColor;
    texCoord = vertexTexCoord;
    worldPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
