#version 330

const float RED_LUM_CONSTANT = 0.2126;
const float GREEN_LUM_CONSTANT = 0.7152;
const float BLUE_LUM_CONSTANT = 0.0722;

uniform sampler2D texture0;
uniform vec2 lightPosition;

in vec2 fragTexCoord;
in vec2 fragPosition;

out vec4 finalColor;

// Adjustable attenuation parameters
const float LINEAR_TERM    = 0.00003; // linear term
const float QUADRATIC_TERM = 0.00003; // quadratic term
const float MIN_BRIGHTNESS = 0.05;    // avoid total darkness

float attenuate(float distance, float linearTerm, float quadraticTerm)
{
    float attenuation = 1.0 / (1.0 + 
                               linearTerm * distance + 
                               quadraticTerm * distance * distance);

    return max(attenuation, MIN_BRIGHTNESS);
}

void main()
{
    float dist = distance(lightPosition, fragPosition);
    float brightness = attenuate(dist, LINEAR_TERM, QUADRATIC_TERM);
    vec4 color = texture(texture0, fragTexCoord);
    
    // === LOGIC-BASED SHADER EFFECTS (if-statements) ===
    vec3 finalColorRGB = color.rgb * brightness;
    
    // Enhanced lighting effect for areas very close to player (upgrade selection feedback)
    if (dist < 100.0)
    {
        float closeBoost = 1.0 - (dist / 100.0);
        if (closeBoost > 0.0)
        {
            finalColorRGB = finalColorRGB * (1.0 + closeBoost * 0.2);
        }
    }
    
    // Extra darkening for areas very far from light source
    if (dist > 500.0)
    {
        float farFactor = (dist - 500.0) / 300.0;
        if (farFactor > 1.0) farFactor = 1.0;
        finalColorRGB = finalColorRGB * (1.0 - farFactor * 0.4);
    }
    
    finalColor = vec4(finalColorRGB, color.a);
}
