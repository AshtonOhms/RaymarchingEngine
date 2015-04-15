
#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable

#define MAX_STEPS 40
#define EPSILON 0.005f

float distBox(float4 point, float4 box) {
	float4 d = fabs(point) - box;
	return min(max(d.x, max(d.y, d.z)), 0.0f) + length(max(d, 0.0f));
}

float distSphere(float4 point, float radius) {
	return length(point) - radius;
}

float distScene(float4 point) {
	float dist = max(-distBox(point, (float4) (0.5f, 0.5f, 1.2f, 0.0f) ),
		distSphere(point, 1.0f));

	dist = max(dist, -distBox(point, (float4) (0.5f, 1.2f, 0.5f, 0.0f) ));
	dist = max(dist, -distBox(point, (float4) (1.2f, 0.5f, 0.5f, 0.0f) ));

	return dist;
	//return distBox(point, (float4) (0.5f, 0.5f, 0.5f, 0.0f));
}

float4 getNormal(float4 point) {
	float h = 0.0001f;

	return normalize( (float4) (
		distScene(point + (float4) (h, 0.0f, 0.0f, 0.0f) ) - distScene(point - (float4) (h, 0.0f, 0.0f, 0.0f)),
		distScene(point + (float4) (0.0f, h, 0.0f, 0.0f) ) - distScene(point - (float4) (0.0f, h, 0.0f, 0.0f)),
		distScene(point + (float4) (0.0f, 0.0f, h, 0.0f) ) - distScene(point - (float4) (0.0f, 0.0f, h, 0.0f)), 0.0f) );
}

float4 rayMarch(float4 rayOrigin, float4 rayDirection) {
	float t = 0.0f;

	float4 color = (float4) (0.0f, 0.0f, 0.0f, 0.0f);

	for(int i = 0; i < MAX_STEPS; ++i) {
		float4 p = rayOrigin + rayDirection * t;
		float d = distScene(p);

		if (fabs(d) < EPSILON) {
			//color = (float4) (1.0f, 0.0f, 0.0f, 0.0f);
			float4 normal = getNormal(p);
			float cosTheta = clamp(dot(normal, 1), 0.0f, 1.0f);

			color = fabs(normal); //(float4) (0.5f, 0.5f, 0.5f, 0.0f) * cosTheta + (float4) (0.3f, 0.3f, 0.3f, 0.0f);
			break;
		}

		t += d;
	}

	return color;
}

__kernel void render(
	__global unsigned int* output,
	const unsigned int width,
	const unsigned int height,
	const float time) {

	int xPixel = get_global_id(0);
	int yPixel = height - get_global_id(1);

	float timef = time / 1000.0f;

	//Camera math shennanigans
	float4 eye = (float4) (0.0f, 0.0f, -4.0f, 0.0f);
	float4 up = (float4) (0.0f, 1.0f, 0.0f, 0.0f);
	float4 right = (float4) (1.0f, 0.0f, 0.0f, 0.0f);
	float4 forward = (float4) (0.0f, 0.0f, 1.0f, 0.0f);

	float focalLength = 2.0f;

	eye.y += sin(timef);
	eye.x += cos(timef);

	float aRatio = (float) width / height;

	float u = xPixel * 2.0f / width - 1.0f;
	float v = yPixel * 2.0f / height - 1.0f;
	u *= aRatio;

	float4 rayOrigin = eye + forward * focalLength + right * u + up * v;
	float4 rayDirection = normalize(rayOrigin - eye);

	float4 color = rayMarch(rayOrigin, rayDirection);

	unsigned int pixel = 0;
	pixel += ((unsigned int) floor(255 * color.z ));
	pixel += ((unsigned int) floor(255 * color.y )) << 8;
	pixel += ((unsigned int) floor(255 * color.x )) << 16;

	output[yPixel * width + xPixel] = pixel;
}