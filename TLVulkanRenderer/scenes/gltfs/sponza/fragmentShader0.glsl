precision highp float;
uniform vec4 u_ambient;
uniform sampler2D u_diffuse;
uniform sampler2D u_normal;
uniform vec4 u_emission;
uniform vec4 u_specular;
uniform float u_shininess;
uniform float u_transparency;
varying vec3 v_positionEC;
varying vec3 v_normal;
varying vec2 v_texcoord_0;

vec3 applyNormalMap(vec3 geomnor, vec3 normap) {
    normap = normap * 2.0 - 1.0;
    vec3 up = normalize(vec3(0.001, 1, 0.001));
    vec3 surftan = normalize(cross(geomnor, up));
    vec3 surfbinor = cross(geomnor, surftan);
    return normap.y * surftan + normap.x * surfbinor + normap.z * geomnor;
}

void main(void) {
  vec3 normal = applyNormalMap(normalize(v_normal), texture2D(u_normal, v_texcoord_0).rgb);
  vec4 diffuse = texture2D(u_diffuse, v_texcoord_0);
  vec3 diffuseLight = vec3(0.0, 0.0, 0.0);
  vec3 specular = u_specular.rgb;
  vec3 specularLight = vec3(0.0, 0.0, 0.0);
  vec3 emission = u_emission.rgb;
  vec3 ambient = u_ambient.rgb;
  vec3 viewDir = -normalize(v_positionEC);
  vec3 ambientLight = vec3(0.0, 0.0, 0.0);
  ambientLight += vec3(0.2, 0.2, 0.2);
  vec3 l = vec3(0.0, 0.0, 1.0);
  diffuseLight += vec3(1.0, 1.0, 1.0) * max(dot(normal,l), 0.);
  vec3 h = normalize(l + viewDir);
  float specularIntensity = max(0., pow(max(dot(normal, h), 0.), u_shininess));
  specularLight += vec3(1.0, 1.0, 1.0) * specularIntensity;
  vec3 color = vec3(0.0, 0.0, 0.0);
  color += diffuse.rgb * diffuseLight;
  color += specular * specularLight;
  color += emission;
  color += ambient * ambientLight;
  gl_FragColor = vec4(color * diffuse.a, diffuse.a * u_transparency);
}