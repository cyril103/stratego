#version 330
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec4 vertexColor;
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float materialKind;
uniform float time;
out vec3 normal;
out vec4 color;
out vec3 worldPosition;
// Keep this water field identical in the vertex and fragment shaders.
// Value + analytic gradient; quintic interpolation avoids creases at cell edges.
vec3 waterNoise(vec2 p) {
    vec2 cell=floor(p),f=fract(p);
    vec2 u=f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du=30.0*f*f*(f*(f-2.0)+1.0);
    vec2 k=vec2(127.1,311.7);
    float a=fract(sin(dot(cell,k))*43758.5453);
    float b=fract(sin(dot(cell+vec2(1,0),k))*43758.5453);
    float c=fract(sin(dot(cell+vec2(0,1),k))*43758.5453);
    float d=fract(sin(dot(cell+vec2(1,1),k))*43758.5453);
    float crossTerm=a-b-c+d;
    return vec3(a+(b-a)*u.x+(c-a)*u.y+crossTerm*u.x*u.y,
                du*vec2(b-a+crossTerm*u.y,c-a+crossTerm*u.x));
}
vec3 waterSurface(vec2 p,float t) {
    // Slowly warp the sampling domain, then mix differently oriented currents.
    // World coordinates also give the two lakes different patterns.
    vec3 warp=waterNoise(p*1.35+vec2(t*.035,-t*.026));
    vec2 bend=vec2(.32,-.21);
    vec2 q=p+bend*(warp.x-.5);
    mat2 jacobian=mat2(1.0)+outerProduct(bend,warp.yz*1.35);
    mat2 r1=mat2(.94,.342,-.342,.94);
    mat2 r2=mat2(.601,-.799,.799,.601);
    mat2 r3=mat2(-.276,.961,-.961,-.276);
    vec3 a=waterNoise(r1*q*4.3+vec2(t*.19,-t*.08));
    vec3 b=waterNoise(r2*q*9.1+vec2(-t*.13,t*.21)+vec2(19.2,7.4));
    vec3 c=waterNoise(r3*q*18.7+vec2(t*.23,t*.16)+vec2(-8.7,31.6));
    float height=.008*(a.x-.5)+.003*(b.x-.5)+.0008*(c.x-.5);
    vec2 gradient=transpose(r1)*a.yz*(.008*4.3)
                 +transpose(r2)*b.yz*(.003*9.1)
                 +transpose(r3)*c.yz*(.0008*18.7);
    return vec3(height,transpose(jacobian)*gradient);
}
void main(){
 vec3 p=vertexPosition;
 if(materialKind>1.5&&materialKind<2.5){
  vec2 local=p.xz-vec2(p.x<0?-2:2,0);float a=atan(local.y,local.x),seed=p.x<0?.6:2.1;
  float radius=(.82+.045*sin(3*a+seed)+.025*cos(5*a-seed))/pow(pow(abs(cos(a)),4)+pow(abs(sin(a)),4),.25);
  float fade=smoothstep(0,.16,1-length(local)/radius);
  p.y+=fade*waterSurface(p.xz,time).x;
 }
 normal=normalize(vec3(matNormal*vec4(vertexNormal,0)));color=vertexColor;
 worldPosition=vec3(matModel*vec4(p,1));gl_Position=mvp*vec4(p,1);
}
