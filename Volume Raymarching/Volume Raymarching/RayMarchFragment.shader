#version 460 core

out vec4 fragColor;

uniform float nearPlane;
uniform float farPlane;
uniform int viewRayNumSteps;
uniform int shadowRayNumSteps;
uniform float absorption;
uniform vec3 sunColor;
uniform vec3 sunPos;
uniform vec2 resolution;
uniform float phaseLobeStrength;

uniform sampler3D volume;
uniform vec3 volumeBoxMin;
uniform vec3 volumeBoxMax;

// Camera (view) transforms
uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;

// Volume (model) transforms
uniform mat4 modelMatrix;
uniform mat4 invModelMatrix;

bool intersectAABB(vec3 rayOrigin, vec3 rayDir,
                   vec3 boxMin, vec3 boxMax,
                   out float tEnter, out float tExit)
{
    // Avoid division by zero
    vec3 invDir = 1.0 / max(abs(rayDir), vec3(1e-6)) * sign(rayDir);

    vec3 t0 = (boxMin - rayOrigin) * invDir;
    vec3 t1 = (boxMax - rayOrigin) * invDir;

    vec3 tMinVec = min(t0, t1);
    vec3 tMaxVec = max(t0, t1);

    tEnter = max(max(tMinVec.x, tMinVec.y), tMinVec.z);
    tExit  = min(min(tMaxVec.x, tMaxVec.y), tMaxVec.z);

    // Clamp to positive values and enforce ordering
    if (tExit < tEnter || tExit < 0.0)
        return false;

    tEnter = max(tEnter, 0.0);
    return true;
}

vec3 normalizeVolumePosition(vec3 pos, vec3 boxMin, vec3 boxMax)
{
    return (pos - boxMin) / (boxMax - boxMin);
}

bool checkInVolume(vec3 position, vec3 boxMin, vec3 boxMax)
{
    return !(position.x > boxMax.x || position.x < boxMin.x ||
             position.y > boxMax.y || position.y < boxMin.y ||
             position.z > boxMax.z || position.z < boxMin.z);
}

float sampleDensity(vec3 position)
{
    return texture(volume, position).r;
}

float shadowRay(vec3 sunDir, vec3 pos, int numSteps,
                vec3 boxMin, vec3 boxMax)
{
    float transmittance = 1.0;
    float tEnter, tExit;
    if (intersectAABB(pos, sunDir, boxMin, boxMax, tEnter, tExit))
    {
        float stepSize = tExit / numSteps;
        for (int i = 0; i < numSteps; i++)
        {
            vec3 position = pos + sunDir * stepSize * i;
            if (transmittance < 0.001 || !checkInVolume(position, boxMin, boxMax))
                break;

            float density = sampleDensity(normalizeVolumePosition(position, boxMin, boxMax));
            transmittance *= exp(-density * stepSize * absorption);
        }
    }
    return transmittance;
}

float phaseHG(float mu, float g)
{
    // Henyey–Greenstein phase function (anisotropic scattering)
    return (1.0 - g * g) / (4.0 * 3.14159265 * pow(1.0 + g * g - 2.0 * g * mu, 1.5));
}

void main()
{
    // Convert pixel to normalized device coordinates
    vec2 uv = gl_FragCoord.xy / resolution;
    uv = uv * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;

    // Ray setup in CAMERA space
    vec3 rayOriginCam = vec3(0.0);
    vec3 rayDirCam = normalize(vec3(uv, -1.0));

    // Transform ray to WORLD space
    vec3 rayOriginWorld = (invViewMatrix * vec4(rayOriginCam, 1.0)).xyz;
    vec3 rayDirWorld = normalize((invViewMatrix * vec4(rayDirCam, 0.0)).xyz);

    // Transform ray to VOLUME LOCAL space
    vec3 rayOriginLocal = (invModelMatrix * vec4(rayOriginWorld, 1.0)).xyz;
    vec3 rayDirLocal = normalize((invModelMatrix * vec4(rayDirWorld, 0.0)).xyz);

    // Ray-box intersection
    float tEnter, tExit;
    if (intersectAABB(rayOriginLocal, rayDirLocal, volumeBoxMin, volumeBoxMax, tEnter, tExit))
    {
        float stepSize = (tExit - tEnter) / viewRayNumSteps;
        float viewTransmittance = 1.0;
        vec3 color = vec3(0.0);

        // Transform sun position into local space once
        vec3 sunPosLocal = (invModelMatrix * vec4(sunPos, 1.0)).xyz;

        // Raymarch through the volume
        for (int i = 0; i < viewRayNumSteps; i++)
        {
            vec3 localPos = rayOriginLocal + rayDirLocal * (tEnter + stepSize * i);

            // Early exit if beyond visible range
            vec3 worldPos = (modelMatrix * vec4(localPos, 1.0)).xyz;
            if (length(worldPos - rayOriginWorld) > farPlane)
                break;

            float density = sampleDensity(normalizeVolumePosition(localPos, volumeBoxMin, volumeBoxMax));
            density = clamp(density, 0.0, 1.0);

            vec3 sunDirLocal = normalize(sunPosLocal - localPos);
            float lightTransmittance = shadowRay(sunDirLocal, localPos, shadowRayNumSteps,
                                                 volumeBoxMin, volumeBoxMax);

            float phase = phaseHG(dot(rayDirLocal, sunDirLocal), phaseLobeStrength);
            color += density * lightTransmittance * stepSize * viewTransmittance * sunColor * phase;
            viewTransmittance *= exp(-density * stepSize * absorption);
        }

        fragColor = vec4(color, 1.0); // linear color, no gamma correction
    }
    else
    {
        fragColor = vec4(0.0);
    }
}
