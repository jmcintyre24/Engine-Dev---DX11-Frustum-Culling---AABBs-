#pragma once

#include "math_types.h"

// Interface to the debug renderer
namespace end
{
	namespace debug_renderer
	{
		void add_line(float3 point_a, float3 point_b, float4 color_a, float4 color_b);

		inline void add_line(float3 p, float3 q, float4 color) { add_line(p, q, color, color); }

		void clear_lines();

		const colored_vertex* get_line_verts();

		size_t get_line_vert_count();

		size_t get_line_vert_capacity();
	}
}