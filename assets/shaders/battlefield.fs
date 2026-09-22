#version 330
in vec3 normal;
in vec4 color;
in vec3 worldPosition;
uniform vec4 colDiffuse;
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform vec3 eyePosition;
uniform float materialKind;
uniform float time;
out vec4 finalColor;
const float PI=3.14159265;
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3-2*f);vec2 k=vec2(127.1,311.7);return mix(mix(fract(sin(dot(i,k))*43758.5453),fract(sin(dot(i+vec2(1,0),k))*43758.5453),f.x),mix(fract(sin(dot(i+vec2(0,1),k))*43758.5453),fract(sin(dot(i+vec2(1,1),k))*43758.5453),f.x),f.y);}
vec3 environment(vec3 d,float rough){d=normalize(d);vec2 uv=vec2(atan(d.z,d.x)/(2*PI)+.68,acos(clamp(d.y,-1,1))/PI);return min(textureLod(texture2,uv,rough*6).rgb,vec3(24))*.55;}
vec3 fresnel(vec3 f,float c){return f+(1-f)*pow(1-clamp(c,0,1),5);}
vec3 lamp(vec3 n,vec3 v,vec3 l,vec3 radiance,vec3 base,vec3 f0,float rough,float metal){vec3 h=normalize(v+l);float nl=max(dot(n,l),0),nv=max(dot(n,v),.001),nh=max(dot(n,h),0),vh=max(dot(v,h),0);float a=rough*rough,a2=a*a,den=nh*nh*(a2-1)+1;float d=a2/(PI*den*den+.00001),k=(rough+1)*(rough+1)/8;float g=nv/(nv*(1-k)+k)*nl/(nl*(1-k)+k);vec3 f=fresnel(f0,vh);return ((1-f)*base*(1-metal)/PI+d*g*f/max(4*nv*nl,.001))*radiance*nl;}
float heightAt(vec2 uv){return dot(texture(texture0,uv,.8).rgb,vec3(.25,.55,.2));}
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
 vec3 n=normalize(normal),v=normalize(eyePosition-worldPosition);vec2 p=worldPosition.xz;
 bool water=materialKind>1.5&&materialKind<2.5,gold=materialKind>3.5&&materialKind<4.5,enamel=materialKind>4.5,wood=materialKind>2.5&&materialKind<3.5;
 vec2 uv=p*(wood?.22:materialKind>.5?1.5:.18);
 if(wood&&abs(n.y)<.5)uv=(abs(n.x)>.5?worldPosition.zy:worldPosition.xy)*vec2(.22,.8);
 vec3 albedo=texture(texture0,uv,.8).rgb*colDiffuse.rgb*color.rgb;float rough=.85,metal=0;float edge=0,depth=0;
 if(gold){albedo=vec3(.96,.72,.34)*(.86+.14*noise(worldPosition.xz*30));rough=.23+.07*noise(p*75);metal=1;}
 else if(enamel){albedo=vec3(.04,.09,.12);rough=.19;metal=.15;}
 else if(water){
  vec2 local=p-vec2(p.x<0?-2:2,0);float angle=atan(local.y,local.x),seed=p.x<0?.6:2.1;
  float radius=(.82+.045*sin(3*angle+seed)+.025*cos(5*angle-seed))/pow(pow(abs(cos(angle)),4)+pow(abs(sin(angle)),4),.25);
  edge=1-length(local)/radius;depth=smoothstep(0,.65,edge);
  vec3 waves=waterSurface(p,time);
  vec2 grad=waves.yz*smoothstep(0,.16,edge);
  n=normalize(vec3(-grad.x,1,-grad.y));
  vec3 bed=pow(texture(texture0,p*1.5+grad*.12,2.0).rgb,vec3(2.2));vec3 transmission=exp(-vec3(4.8,1.7,1.1)*(.3+depth*1.8));
  vec3 base=bed*transmission*.38+vec3(.005,.055,.065)*(1-transmission);
  float caustic=pow(clamp(1.0-length(waves.yz)*16.0,0.0,1.0),6.0);
  base+=vec3(.07,.12,.10)*caustic*(1-depth);albedo=pow(base,vec3(1/2.2));rough=.13;
 }else{
  float e=.002;vec2 bump=vec2(heightAt(uv+vec2(e,0))-heightAt(uv-vec2(e,0)),heightAt(uv+vec2(0,e))-heightAt(uv-vec2(0,e)));
  vec3 tangent=abs(n.y)>.5?vec3(1,0,0):normalize(cross(vec3(0,1,0),n));vec3 bitangent=normalize(cross(n,tangent));n=normalize(n-(tangent*bump.x+bitangent*bump.y)*(wood?.4:.45));
  if(wood){rough=.23+.1*heightAt(uv);albedo*=.78;}else if(materialKind>.5)rough=.7;
  else{float road=1-smoothstep(.12,.40,abs(p.x-.35*sin(p.y*.85))),wear=smoothstep(.52,.78,noise(p*.65))*.45;vec3 earth=texture(texture1,p*.5).rgb*vec3(.95,.84,.67);albedo=mix(albedo,earth,max(road*.64,wear));}
 }
 vec3 base=pow(max(albedo,vec3(0)),vec3(2.2)),f0=water?vec3(.0204):mix(vec3(.04),base,metal);
 vec3 lit=base*(vec3(.22,.25,.28)+environment(n,1)*.18)*(1-metal);
 lit+=lamp(n,v,normalize(vec3(-.5,1,.55)),vec3(3.8,3.15,2.35),base,f0,rough,metal);
 lit+=lamp(n,v,normalize(vec3(.6,.5,-.8)),vec3(.7,1.05,1.6),base,f0,rough,metal);
 vec3 reflected=environment(reflect(-v,n),rough);
 if(water){float f=.20+.75*pow(1-max(dot(n,v),0),4);lit=mix(lit,reflected,f);lit+=lamp(n,v,normalize(vec3(-.20,.85,-.55)),vec3(.22,.26,.29),vec3(0),f0,.25,1);}
 else{lit+=reflected*fresnel(f0,max(dot(n,v),0))*(gold?1.3:1);if(wood||enamel)lit+=environment(reflect(-v,n),.13)*fresnel(vec3(.04),max(dot(n,v),0))*.3;}
 lit=clamp((lit*(2.51*lit+.03))/(lit*(2.43*lit+.59)+.14),0,1);
 finalColor=vec4(pow(lit,vec3(1/2.2)),colDiffuse.a);
}
