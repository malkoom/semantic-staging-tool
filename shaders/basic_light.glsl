#version 330

// Entradas interpoladas desde el vertex shader por defecto de raylib.
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

// Raylib rellena estos uniformes desde el material del modelo.
// texture0 es la imagen de la textura y colDiffuse su color/tinte base.
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// La cámara sigue siendo un uniforme porque puede moverse durante la escena.
uniform vec3 viewPos;

out vec4 finalColor;

// Luz direccional definida completamente en este shader. El vector apunta
// desde el fragmento hacia la luz, por lo que ilumina desde arriba y delante.
const vec3 LIGHT_DIRECTION = normalize(vec3(0.45, 0.80, 0.35));
const vec3 LIGHT_COLOR = vec3(1.0, 0.98, 0.92);
const vec3 AMBIENT_COLOR = vec3(0.58, 0.62, 0.70);
const float SPECULAR_STRENGTH = 0.18;
const float SHININESS = 24.0;

void main()
{
    vec4 albedo = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    vec3 normal = normalize(fragNormal);
    vec3 viewDirection = normalize(viewPos - fragPosition);

    // Phong: ambiente generoso para preservar detalle en las caras en sombra,
    // más las componentes difusa y especular de la luz direccional.
    vec3 ambient = AMBIENT_COLOR;
    float diffuseFactor = max(dot(normal, LIGHT_DIRECTION), 0.0);
    vec3 diffuse = diffuseFactor * LIGHT_COLOR;

    vec3 reflectedDirection = reflect(-LIGHT_DIRECTION, normal);
    float specularFactor = diffuseFactor *
        pow(max(dot(viewDirection, reflectedDirection), 0.0), SHININESS);
    vec3 specular = SPECULAR_STRENGTH * specularFactor * LIGHT_COLOR;

    finalColor = vec4(albedo.rgb * (ambient + diffuse) + specular, albedo.a);
}
