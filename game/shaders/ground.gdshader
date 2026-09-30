shader_type spatial;
render_mode diffuse_burley;
uniform vec4 sand : source_color = vec4(0.62,0.55,0.37,1.0);
uniform vec2 lamp_position;
uniform vec2 lamp_direction = vec2(1.0,0.0);
uniform bool lamp_on = false;
uniform bool lamp_day = false;
uniform float sky_blend = 1.0;
varying vec3 world_position;
void vertex() { world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
void fragment() {
 vec2 p = world_position.xz;
 float noise = fract(sin(dot(floor(p*14.0), vec2(12.9898,78.233)))*43758.5453);
 vec3 base = sand.rgb * (0.93 + noise*0.10);
 vec2 d = p-lamp_position;
 float dist = length(d);
 float cone = smoothstep(0.858,0.888,dot(normalize(d+vec2(0.00001)),lamp_direction)) * (1.0-smoothstep(6.55,6.8,dist));
 float halo = 1.0-smoothstep(1.14,1.25,dist);
 float beam = max(cone,halo) * float(lamp_on);
 vec3 night_color = vec3(0.13,0.31,0.36);
 vec3 day_color = vec3(0.95,0.72,0.34);
 base = mix(base, lamp_day ? day_color : night_color, beam*0.76);
 ALBEDO=base;
 ROUGHNESS=0.95;
 EMISSION=(lamp_day ? day_color*0.26 : night_color*0.2)*beam;
}
