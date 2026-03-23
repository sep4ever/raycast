#version 330 core

uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 u_camPos;
uniform float u_camYaw;
uniform float u_camPitch;

// object counts
uniform int u_sphereCount;
uniform int u_triCount;
uniform int u_boxCount;

// sphere: (x,y,z,r)
uniform vec4 u_spheres[16];

// triangles: each vertex xyz stored in vec4 (w ignored)
uniform vec4 u_triA[16];
uniform vec4 u_triB[16];
uniform vec4 u_triC[16];

// boxes: center xyz, and half-extents
uniform vec4 u_boxCenters[16]; // xyz, w unused
uniform vec4 u_boxHalfSizes[16]; // xyz (half size), w unused

out vec4 fragColor;

const float EPS = 1e-4;
const vec3 ambientCol = vec3(0.08);

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

float intersectTriangle(vec3 ro, vec3 rd, vec3 v0, vec3 v1, vec3 v2) {
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 p = cross(rd, e2);
    float det = dot(e1, p);
    if (abs(det) < 1e-6) return -1.0;
    float invDet = 1.0 / det;
    vec3 tvec = ro - v0;
    float u = dot(tvec, p) * invDet;
    if (u < 0.0 || u > 1.0) return -1.0;
    vec3 q = cross(tvec, e1);
    float v = dot(rd, q) * invDet;
    if (v < 0.0 || u + v > 1.0) return -1.0;
    float t = dot(e2, q) * invDet;
    return t > EPS ? t : -1.0;
}

float intersectAABB(vec3 ro, vec3 rd, vec3 center, vec3 halfSize) {
    vec3 minB = center - halfSize;
    vec3 maxB = center + halfSize;
    float tmin = (minB.x - ro.x) / rd.x;
    float tmax = (maxB.x - ro.x) / rd.x;
    if (tmin > tmax) { float tmp = tmin; tmin = tmax; tmax = tmp; }
    float tymin = (minB.y - ro.y) / rd.y;
    float tymax = (maxB.y - ro.y) / rd.y;
    if (tymin > tymax) { float tmp = tymin; tymin = tymax; tymax = tmp; }
    if ((tmin > tymax) || (tymin > tmax)) return -1.0;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;
    float tzmin = (minB.z - ro.z) / rd.z;
    float tzmax = (maxB.z - ro.z) / rd.z;
    if (tzmin > tzmax) { float tmp = tzmin; tzmin = tzmax; tzmax = tmp; }
    if ((tmin > tzmax) || (tzmin > tmax)) return -1.0;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;
    return tmin > EPS ? tmin : (tmax > EPS ? tmax : -1.0);
}

void main() {
    vec2 uv = (gl_FragCoord.xy / u_resolution) * 2.0 - 1.0;
    uv.x *= u_resolution.x / u_resolution.y;

    vec3 camPos = u_camPos;
    float fov = radians(60.0);
    float scale = tan(fov * 0.5);

    vec3 rayCam = normalize(vec3(uv.x * scale, uv.y * scale, -1.0));
    float cy = cos(u_camYaw), sy = sin(u_camYaw);
    //vec3 rd = normalize(vec3(cy * rayCam.x - sy * rayCam.z, rayCam.y, sy * rayCam.x + cy * rayCam.z));
    // поворот по pitch (X) — u_camPitch: положительный поднимает взгляд вверх
    float cp = cos(u_camPitch);
    float sp = sin(u_camPitch);
    vec3 rd_yaw = vec3(cy * rayCam.x - sy * rayCam.z,
                   rayCam.y,
                   sy * rayCam.x + cy * rayCam.z);
    // 1. forward из yaw/pitch
    vec3 forward = normalize(vec3(
        cos(u_camYaw) * cos(u_camPitch),
        sin(u_camPitch),
        sin(u_camYaw) * cos(u_camPitch)
    ));

    // 2. фиксированный мировой up
    vec3 worldUp = vec3(0.0, 1.0, 0.0);

    // 3. строим базис
    vec3 right = normalize(cross(forward, worldUp));
    vec3 up    = normalize(cross(right, forward));
    vec3 rd = normalize(
        forward +
        uv.x * scale * right +
        uv.y * scale * up
    );
    vec3 ro = camPos;

    float tMin = 1e20;
    vec3 hitNormal = vec3(0.0);
    vec3 hitColor = vec3(0.0);
    bool hit = false;

    // spheres
    for (int i = 0; i < u_sphereCount; ++i) {
        vec4 s = u_spheres[i];
        float t = intersectSphere(ro, rd, s.xyz, s.w);
        if (t > 0.0 && t < tMin) {
            tMin = t;
            vec3 p = ro + rd * t;
            hitNormal = normalize(p - s.xyz);
            hitColor = vec3(0.9, 0.4, 0.3);
            hit = true;
        }
    }

    // triangles
    for (int i = 0; i < u_triCount; ++i) {
        vec3 a = u_triA[i].xyz;
        vec3 b = u_triB[i].xyz;
        vec3 c = u_triC[i].xyz;
        float t = intersectTriangle(ro, rd, a, b, c);
        if (t > 0.0 && t < tMin) {
            tMin = t;
            vec3 p = ro + rd * t;
            hitNormal = normalize(cross(b - a, c - a));
            hitColor = vec3(0.6, 0.6, 0.6);
            hit = true;
        }
    }

    // boxes
    for (int i = 0; i < u_boxCount; ++i) {
        vec3 center = u_boxCenters[i].xyz;
        vec3 halfSize = u_boxHalfSizes[i].xyz;
        float t = intersectAABB(ro, rd, center, halfSize);
        if (t > 0.0 && t < tMin) {
            tMin = t;
            vec3 p = ro + rd * t;
            vec3 local = (p - center) / halfSize;
            vec3 absLocal = abs(local);
            if (absLocal.x > absLocal.y && absLocal.x > absLocal.z) hitNormal = vec3(sign(local.x),0.0,0.0);
            else if (absLocal.y > absLocal.z) hitNormal = vec3(0.0,sign(local.y),0.0);
            else hitNormal = vec3(0.0,0.0,sign(local.z));
            hitColor = vec3(0.6,0.6,0.6);
            hit = true;
        }
    }

    vec3 color = vec3(0.0);
    if (!hit) {
        color = vec3(0.6, 0.8, 1.0) * (0.5 + 0.5 * rd.y);
    } else {
        float diffuse = max(dot(hitNormal, normalize(vec3(-0.5, -1.0, -0.3))), 0.0);
        color = hitColor * (0.2 + 0.8 * diffuse);
    }

    fragColor = vec4(pow(color, vec3(1.0/2.2)), 1.0);
}