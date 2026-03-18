#version 330 core

uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_camPos;   // позиция камеры (x, y, z)
uniform float u_camYaw;  // поворот вокруг вертикали (в радианах)

out vec4 fragColor;

const float EPS = 1e-4;
const vec3 ambientCol = vec3(0.08);

// пересечение луча со сферой, возвращает положительный t или -1
float intersectSphere(vec3 ro, vec3 rd, vec3 center, float r) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - r*r;
    float disc = b*b - c;
    if (disc < 0.0) return -1.0;
    float sq = sqrt(disc);
    float t = -b - sq;
    if (t > EPS) return t;
    t = -b + sq;
    return t > EPS ? t : -1.0;
}

// пересечение луча с горизонтальной плоскостью y = planeY
float intersectPlaneY(vec3 ro, vec3 rd, float planeY) {
    if (abs(rd.y) < 1e-6) return -1.0;
    float t = (planeY - ro.y) / rd.y;
    return t > EPS ? t : -1.0;
}

// нормаль для сферы и плоскости
vec3 normalSphere(vec3 p, vec3 center) { return normalize(p - center); }
vec3 normalPlaneY() { return vec3(0.0, 1.0, 0.0); }

// проверка тени: возвращает 0..1 (1 - полностью освещён)
float shadowFactor(vec3 pt, vec3 lightDir, vec3 spheres[4], float radii[4], int sphCount) {
    // смещаемся от поверхности немного по нормали (вызов должен передать корректную нормаль)
    vec3 ro = pt + lightDir * EPS * 4.0; // небольшой сдвиг в сторону источника света
    for (int i = 0; i < sphCount; ++i) {
        float t = intersectSphere(ro, lightDir, spheres[i], radii[i]);
        if (t > 0.0) return 0.12; // можно менять глубину тени
    }
    return 1.0;
}

vec3 shadePoint(vec3 ro, vec3 rd, vec3 hitPos, vec3 n, vec3 viewDir,
                vec3 lightPos, vec3 spheres[4], float radii[4], int sphCount)
{
    vec3 color = vec3(0.0);
    vec3 lampDir = normalize(lightPos - hitPos);
    float distToLight = length(lightPos - hitPos);

    float sh = shadowFactor(hitPos, lampDir, spheres, radii, sphCount);

    // diffuse
    float diff = max(dot(n, lampDir), 0.0);
    // specular
    vec3 refl = reflect(-lampDir, n);
    float spec = pow(max(dot(refl, viewDir), 0.0), 32.0);

    // simple material colors: vary by object by position (demo)
    vec3 base = vec3(0.8, 0.45, 0.3);
    color += ambientCol * base;
    color += sh * diff * base;
    color += sh * spec * vec3(1.0);

    // clamp
    return clamp(color, 0.0, 1.0);
}

void main() {
    // set up camera basis (camera looks toward negative Z in camera space)
    vec2 uv = (gl_FragCoord.xy / u_resolution) * 2.0 - 1.0;
    uv.x *= u_resolution.x / u_resolution.y;

    // camera parameters
    vec3 camPos = u_camPos;
    float fov = radians(60.0);
    float scale = tan(fov * 0.5);

    // ray in camera space
    vec3 rayCam = normalize(vec3(uv.x * scale, uv.y * scale, -1.0));

    // rotate ray by camera yaw around Y
    float cy = cos(u_camYaw);
    float sy = sin(u_camYaw);
    vec3 rd = normalize(vec3(cy * rayCam.x - sy * rayCam.z,
                             rayCam.y,
                             sy * rayCam.x + cy * rayCam.z));

    // scene: spheres and plane
    const int sphCount = 3;
    vec3 spheres[4];
    float radii[4];
    spheres[0] = vec3(camPos.x + 200.0,  20.0, camPos.z - 300.0); radii[0] = 80.0;
    spheres[1] = vec3(camPos.x + 0.0,   40.0, camPos.z - 400.0); radii[1] = 50.0;
    spheres[2] = vec3(camPos.x - 150.0, 30.0, camPos.z - 250.0); radii[2] = 40.0;

    // light
    vec3 lightPos = vec3(camPos.x + 100.0 * cos(u_time * 0.6), 200.0, camPos.z - 200.0 + 120.0 * sin(u_time * 0.5));

    // find nearest hit
    float tMin = 1e20;
    int hitType = 0; // 0 - none, 1 - sphere, 2 - floor
    int hitIndex = -1;

    // spheres
    for (int i = 0; i < sphCount; ++i) {
        float t = intersectSphere(camPos, rd, spheres[i], radii[i]);
        if (t > 0.0 && t < tMin) { tMin = t; hitType = 1; hitIndex = i; }
    }
    // plane at y=0
    float tp = intersectPlaneY(camPos, rd, 0.0);
    if (tp > 0.0 && tp < tMin) { tMin = tp; hitType = 2; hitIndex = -1; }

    vec3 finalColor = vec3(0.0);

    if (hitType == 0) {
        // sky gradient
        float v = 0.5 * (rd.y + 1.0);
        finalColor = mix(vec3(0.2, 0.35, 0.6), vec3(0.9, 0.95, 1.0), pow(clamp(v,0.0,1.0), 1.2));
    } else {
        vec3 hitPos = camPos + rd * tMin;
        vec3 normal;
        vec3 viewDir = normalize(-rd);

        if (hitType == 1) {
            normal = normalSphere(hitPos, spheres[hitIndex]);
        } else {
            normal = normalPlaneY();
        }

        // base shading
        finalColor = shadePoint(camPos, rd, hitPos, normal, viewDir, lightPos, spheres, radii, sphCount);

        // simple reflection (one bounce)
        float metalness = 0.25; // 0..1
        if (metalness > 0.0) {
            vec3 reflectDir = reflect(rd, normal);
            // offset origin to avoid self-intersection
            vec3 ro2 = hitPos + normal * EPS * 4.0;
            // test reflection against spheres only (cheap)
            float tRef = 1e20;
            for (int i = 0; i < sphCount; ++i) {
                float tt = intersectSphere(ro2, reflectDir, spheres[i], radii[i]);
                if (tt > 0.0 && tt < tRef) tRef = tt;
            }
            if (tRef < 1e19) {
                vec3 hit2 = ro2 + reflectDir * tRef;
                vec3 n2 = normalize(hit2 - spheres[0]); // approximate normal (demo)
                float reflShade = max(dot(normalize(-lightPos + hit2), n2), 0.0);
                finalColor = mix(finalColor, vec3(0.9,0.9,0.95) * reflShade, metalness);
            }
        }
    }

    // gamma
    finalColor = pow(finalColor, vec3(1.0/2.2));

    fragColor = vec4(finalColor, 1.0);
}