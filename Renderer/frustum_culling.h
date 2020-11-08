#pragma once

#include <array>

// Note: You are free to make adjustments/additions to the declarations provided here.
using namespace DirectX;

namespace end
{
	struct sphere_t { float3 center; float radius; }; //Alterative: using sphere_t = float4;

	struct aabb_t { float3 min; float3 max; }; //Alternative: aabb_t { float3 min; float3 max; };

	struct plane_t { float3 normal; float offset; };  //Alterative: using plane_t = float4;

	using frustum_t = std::array<plane_t, 6>;

	// Create the frustum
	void create_frustum(XMVECTOR* verticesFrustum, float4 colorN = { 1.0f, 0.0f, 0.0f, 1.0f }, float4 colorF = {1.0f, 0.0f, 0.0f, 1.0f})
	{
		// 'Near Plane' Edge Creation
		debug_renderer::add_line((float3&)verticesFrustum[0], (float3&)verticesFrustum[3], colorN);
		debug_renderer::add_line((float3&)verticesFrustum[1], (float3&)verticesFrustum[2], colorN);
		debug_renderer::add_line((float3&)verticesFrustum[0], (float3&)verticesFrustum[1], colorN);
		debug_renderer::add_line((float3&)verticesFrustum[3], (float3&)verticesFrustum[2], colorN);

		// 'Far Plane' Edge Creation
		debug_renderer::add_line((float3&)verticesFrustum[7], (float3&)verticesFrustum[4], colorF);
		debug_renderer::add_line((float3&)verticesFrustum[6], (float3&)verticesFrustum[5], colorF);
		debug_renderer::add_line((float3&)verticesFrustum[4], (float3&)verticesFrustum[5], colorF);
		debug_renderer::add_line((float3&)verticesFrustum[6], (float3&)verticesFrustum[7], colorF);

		// 'Left Plane' Edge Creation
		debug_renderer::add_line((float3&)verticesFrustum[0], (float3&)verticesFrustum[4], colorN);
		debug_renderer::add_line((float3&)verticesFrustum[1], (float3&)verticesFrustum[5], colorN);

		// 'Right Plane' Edge Creation
		debug_renderer::add_line((float3&)verticesFrustum[7], (float3&)verticesFrustum[3], colorN);
		debug_renderer::add_line((float3&)verticesFrustum[6], (float3&)verticesFrustum[2], colorN);
	}

	// Get the average of four verts.
	XMVECTOR four_vert_avg(XMVECTOR vec0, XMVECTOR vec1, XMVECTOR vec2, XMVECTOR vec3)
	{
		return {
			(XMVectorGetX(vec0) + XMVectorGetX(vec1) + XMVectorGetX(vec2) + XMVectorGetX(vec3)) / 4.0f,
			(XMVectorGetY(vec0) + XMVectorGetY(vec1) + XMVectorGetY(vec2) + XMVectorGetY(vec3)) / 4.0f,
			(XMVectorGetZ(vec0) + XMVectorGetZ(vec1) + XMVectorGetZ(vec2) + XMVectorGetZ(vec3)) / 4.0f
		};
	}

	// Calculates the plane of a triangle from three points.
	plane_t calculate_plane(float3 a, float3 b, float3 c)
	{
		plane_t ret;
		ret.normal = (float3&)XMVector3Normalize(XMVector3Cross((XMVECTOR&)(b - a), (XMVECTOR&)(c - b)));
		ret.offset = XMVectorGetX(XMVector3Dot((XMVECTOR&)ret.normal, (XMVECTOR&)a));

		return ret;
	}

	// Calculates a frustum (6 planes) from the input view parameter.
	//
	// Calculate the eight corner points of the frustum. 
	// Use your debug renderer to draw the edges.
	// 
	// Calculate the frustum planes.
	// Use your debug renderer to draw the plane normals as line segments.
	void calculate_frustum(frustum_t& frustum, const view_t& view, matrix world)
	{
		// Multiply the view by the projection matrix to move it into screen space.
		matrix viewProj = (matrix&)view.view_mat * (matrix&)view.proj_mat;

		// A setup of vertices for a box in Homogeneous space
		XMVECTOR simpleView[8] = {
				{-1.0f, -1.0f, 0.0f, 1.0f},
				{-1.0f,  1.0f, 0.0f, 1.0f},
				{ 1.0f,  1.0f, 0.0f, 1.0f},
				{ 1.0f, -1.0f, 0.0f, 1.0f},
				{-1.0f, -1.0f, 1.0f, 1.0f},
				{-1.0f,  1.0f, 1.0f, 1.0f},
				{ 1.0f,  1.0f, 1.0f, 1.0f},
				{ 1.0f, -1.0f, 1.0f, 1.0f}
		};
		// Take the inverse of the viewProj matrix
		XMVECTOR det;
		matrix viewProjWorld = XMMatrixInverse(&det, viewProj);
		for (int i = 0; i < 8; i++)
		{
			// Multiply the vertices to get them into screen space.
			simpleView[i] = XMVector4Transform(simpleView[i], viewProjWorld);
			// Homogeneous to Cartesian, divide by w.
			simpleView[i] /= XMVectorGetW(simpleView[i]);
		}

		// Create the Frustum
		create_frustum(simpleView);

		// Near Plane Norm
		plane_t nearPlane = calculate_plane((float3&)simpleView[3], (float3&)simpleView[2], (float3&)simpleView[1]);
		XMVECTOR pos = four_vert_avg(simpleView[0], simpleView[1], simpleView[2], simpleView[3]);
		nearPlane.normal += (float3&)pos;
		debug_renderer::add_line((float3&)pos, nearPlane.normal, { 0.0f, 1.0f, 1.0f, 1.0f });

		// Far Plane Norm
		plane_t farPlane = calculate_plane((float3&)simpleView[7], (float3&)simpleView[5], (float3&)simpleView[6]);
		pos = four_vert_avg(simpleView[4], simpleView[5], simpleView[6], simpleView[7]);
		farPlane.normal += (float3&)pos;
		debug_renderer::add_line((float3&)pos, farPlane.normal, { 0.0f, 1.0f, 1.0f, 1.0f });

		// Left Plane Norm
		plane_t leftPlane = calculate_plane((float3&)simpleView[4], (float3&)simpleView[1], (float3&)simpleView[5]);
		pos = four_vert_avg(simpleView[0], simpleView[1], simpleView[4], simpleView[5]);
		leftPlane.normal += (float3&)pos;
		debug_renderer::add_line((float3&)pos, leftPlane.normal, { 0.0f, 1.0f, 1.0f, 1.0f });
		
		// Top Plane Norm
		plane_t topPlane = calculate_plane((float3&)simpleView[6], (float3&)simpleView[1], (float3&)simpleView[2]);
		pos = four_vert_avg(simpleView[1], simpleView[2], simpleView[5], simpleView[6]);
		topPlane.normal += (float3&)pos;
		debug_renderer::add_line((float3&)pos, topPlane.normal, { 0.0f, 1.0f, 1.0f, 1.0f });
		
		// Right Plane Norm
		plane_t rightPlane = calculate_plane((float3&)simpleView[6], (float3&)simpleView[2], (float3&)simpleView[7]);
		pos = four_vert_avg(simpleView[3], simpleView[2], simpleView[6], simpleView[7]);
		rightPlane.normal += (float3&)pos;
		debug_renderer::add_line((float3&)pos, rightPlane.normal, { 0.0f, 1.0f, 1.0f, 1.0f });

		// Bottom Plane Norm
		plane_t bottomPlane = calculate_plane((float3&)simpleView[3], (float3&)simpleView[0], (float3&)simpleView[7]);
		pos = four_vert_avg(simpleView[4], simpleView[7], simpleView[0], simpleView[3]);
		bottomPlane.normal += (float3&)pos;
		debug_renderer::add_line((float3&)pos, bottomPlane.normal, { 0.0f, 1.0f, 1.0f, 1.0f });

		// Move the planes into world space since the Frustum doesn't get calculated with the lines
		for (int i = 0; i < 8; i++)
		{
			simpleView[i] = XMVector3Transform((XMVECTOR&)simpleView[i], world);
		}
		// Near Plane Norm
		nearPlane = calculate_plane((float3&)simpleView[3], (float3&)simpleView[2], (float3&)simpleView[1]);
		// Far Plane Norm
		farPlane = calculate_plane((float3&)simpleView[7], (float3&)simpleView[5], (float3&)simpleView[6]);
		// Left Plane Norm
		leftPlane = calculate_plane((float3&)simpleView[4], (float3&)simpleView[1], (float3&)simpleView[5]);
		// Top Plane Norm
		topPlane = calculate_plane((float3&)simpleView[6], (float3&)simpleView[1], (float3&)simpleView[2]);
		// Right Plane Norm
		rightPlane = calculate_plane((float3&)simpleView[6], (float3&)simpleView[2], (float3&)simpleView[7]);
		// Bottom Plane Norm
		bottomPlane = calculate_plane((float3&)simpleView[3], (float3&)simpleView[0], (float3&)simpleView[7]);

		// Set Frustum Planes
		frustum[0] = nearPlane;
		frustum[1] = farPlane;
		frustum[2] = leftPlane;
		frustum[3] = topPlane;
		frustum[4] = rightPlane;
		frustum[5] = bottomPlane;
	}

	// Calculates which side of a plane the sphere is on.
	//
	// Returns -1 if the sphere is completely behind the plane.
	// Returns 1 if the sphere is completely in front of the plane.
	// Otherwise returns 0 (Sphere overlaps the plane)
	int classify_sphere_to_plane(const sphere_t& sphere, const plane_t& plane)
	{
		float cmp = XMVectorGetX(XMVector3Dot((XMVECTOR&)sphere.center, (XMVECTOR&)plane.normal)) - plane.offset;

		if (cmp > sphere.radius)
			return 1;
		else if (cmp < -sphere.radius)
			return -1;
		else
			return 0;
	}

	// Calculates which side of a plane the aabb is on.
	//
	// Returns -1 if the aabb is completely behind the plane.
	// Returns 1 if the aabb is completely in front of the plane.
	// Otherwise returns 0 (aabb overlaps the plane)
	// MUST BE IMPLEMENTED UsING THE PROJECTED RADIUS TEST
	int classify_aabb_to_plane(const aabb_t& aabb, const plane_t& plane)
	{
		float3 cp = (aabb.min + aabb.max); // Add the points together
		cp *= 0.5f; // Divide by 2
		float3 e = aabb.max - cp;

		float radius = e.x * fabsf(plane.normal.x) + e.y * fabsf(plane.normal.y) + e.z * fabsf(plane.normal.z);

		return classify_sphere_to_plane({ cp, radius }, plane);
	}

	// Determines if the aabb is inside the frustum.
	//
	// Returns false if the aabb is completely behind any plane.
	// Otherwise returns true.
	bool aabb_to_frustum(const aabb_t& aabb, const frustum_t& frustum)
	{
		if (classify_aabb_to_plane(aabb, frustum[0]) == -1) return false;
		if (classify_aabb_to_plane(aabb, frustum[1]) == -1) return false;
		if (classify_aabb_to_plane(aabb, frustum[2]) == -1) return false;
		if (classify_aabb_to_plane(aabb, frustum[3]) == -1) return false;
		if (classify_aabb_to_plane(aabb, frustum[4]) == -1) return false;
		if (classify_aabb_to_plane(aabb, frustum[5]) == -1) return false;

		return true;
	}
}