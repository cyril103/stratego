#version 330
in vec3 normal;
in vec4 color;
in vec3 worldPosition;
in vec2 texCoord;
uniform vec4 colDiffuse;
uniform sampler2D texture0;
uniform vec3 eyePosition;
uniform float roughness;
uniform float metallic;
uniform float surfaceKind;
out vec4 finalColor;
float woodNoise(vec2 p){
    vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
    float a=fract(sin(dot(i,vec2(127.1,311.7)))*43758.5453);
    float b=fract(sin(dot(i+vec2(1,0),vec2(127.1,311.7)))*43758.5453);
    float c=fract(sin(dot(i+vec2(0,1),vec2(127.1,311.7)))*43758.5453);
    float d=fract(sin(dot(i+vec2(1,1),vec2(127.1,311.7)))*43758.5453);
    return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
}
vec3 lamp(vec3 n,vec3 v,vec3 l,vec3 tint,vec3 base) {
    l=normalize(l);
    float diffuse=max(dot(n,l),0.0);
    vec3 h=normalize(v+l);
    float exponent=mix(100.0,12.0,roughness);
    float nh=max(dot(n,h),0.0);
    // The broad lobe approximates reflection of a large studio softbox.
    float highlight=.82*pow(nh,exponent)+.18*pow(nh,6.0);
    vec3 f0=mix(vec3(0.065),base,metallic);
    vec3 fresnel=f0+(1.0-f0)*pow(1.0-max(dot(v,h),0.0),5.0);
    return tint*(base*diffuse*(1.0-metallic*.35)+fresnel*highlight*3.2*(1.0-roughness*.65));
}
void main() {
    vec3 n=normalize(normal),v=normalize(eyePosition-worldPosition);
    vec3 base=pow(max(texture(texture0,texCoord).rgb*colDiffuse.rgb*color.rgb,vec3(0.0)),vec3(2.2));
    if(surfaceKind>0.5&&surfaceKind<1.5){
        vec2 p=worldPosition.xz;
        float bend=woodNoise(p*.35)*2.0;
        float grain=woodNoise(vec2(p.x*.35,p.y*24.0+bend));
        float fine=woodNoise(vec2(p.x*2.0,p.y*130.0+bend*5.0));
        float seams=smoothstep(.47,.495,abs(fract((p.y+14.0)/2.8)-.5));
        base*=.76+.26*grain+.08*fine;
        if(worldPosition.y<-.45)base*=1.0-.22*seams;
    }else if(surfaceKind>1.5){
        vec2 p=worldPosition.xz*220.0;
        float weave=sin(p.x)*sin(p.y);
        base*=.96+.04*weave;
    }
    vec3 ambient=mix(vec3(.18,.20,.24),vec3(.46,.51,.60),n.y*.5+.5);
    vec3 light=base*ambient;
    light+=lamp(n,v,vec3(-.55,1.0,.65),vec3(1.55,1.43,1.30),base);
    light+=lamp(n,v,vec3(.85,.45,.3),vec3(.48,.57,.76),base);
    light+=lamp(n,v,vec3(.1,.7,-1.0),vec3(.70,.75,.85),base);
    light=vec3(1.0)-exp(-light*1.12);
    finalColor=vec4(pow(max(light,vec3(0.0)),vec3(1.0/2.2)),colDiffuse.a);
}
