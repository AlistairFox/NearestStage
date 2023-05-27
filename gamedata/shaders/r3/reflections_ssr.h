#if !defined(REFLECTIONS_SSR_INCLUDED)
#define REFLECTIONS_SSR_INCLUDED
	#include "common.h"
	
	// Отражения SSR из мода DWM
	
	float get_depth(float2 tc)
	{
	#ifndef USE_MSAA
		return s_position.Sample(smp_nofilter, tc).z;
	#else
		return s_position.Load(int3(tc * pos_decompression_params2.xy, 0), 0).z;
	#endif
	}
	
	float3 remap_sky(float3 vreflect)
	{
		vreflect /= max(abs(vreflect.x), max(abs(vreflect.y), abs(vreflect.z)));
		if(vreflect.y < 0.99) vreflect.y = vreflect.y * 2 - 1;
		return vreflect;
	}

	bool is_in_range(float val, float min, float max)
	{
		return val >= min && val <= max;
	}
	
	bool is_in_quad(float2 p, float2 lt = float2(0, 0), float2 rb = float2(1, 1))
	{
		return is_in_range(p.x, lt.x, rb.x) && is_in_range(p.y, lt.y, rb.y);
	}

	float calc_vignette(in float2 tc, in float size)
	{
		float4 v = float4(tc, 1 - tc) / size;
		return saturate(v.x * v.y * v.z * v.w);
	}

	#define SSR_STEP_INCREASE 		1.2 
	#define SSR_STEPS 				25 		
	#define SSR_START_RAY		 	1.2 	
	#define SSR_THRESHOLD	 		3
	#define SSR_REFINE_STEPS		5 		
	#define SSR_MARCH_MIN_DIST 		1 		
	#define SSR_MARCH_MAX_DIST 		500 	
	#define SSR_VIGNETTE 			0.3 	
	
	// Поиск пересечения с увеличивающимся шагом
	bool calc_intersection(
		in int steps, in float increase, in float3 pos, in float3 vreflect,
		inout float3 pos_step, inout float2 tc_step, inout float2 steplen)
	{
		for(int i = 0; i < steps; i++)
		{
			pos_step = pos + vreflect * steplen.x;
			tc_step = pos_step.xy / pos_step.z;
			steplen.y *= increase;
			steplen.x += steplen.y;
			if(!is_in_quad(tc_step)) 
				break;
			float depth_hit = get_depth(tc_step);
			// Обрезаем небо и объекты ближе плоскости отражения
			if(depth_hit <= 0.01 || depth_hit + SSR_THRESHOLD <= pos.z) 
				continue;
			// Нашли пересечение
			if(pos_step.z >= depth_hit)
				return true;
		}
		return false;
	}
	
	float4 calc_ssr(float2 tc, float3 P, float3 N)
	{
		// Чтобы каждый раз не проецировать позицию матрицей, будем шагать по tc*depth
		float3 pos = float3(tc, 1) * P.z;
		
		// Рассчет отраженного вектора в tc*depth
		float3 vreflect = normalize(reflect(normalize(P), N));
		vreflect.xy = vreflect.xy + pos_decompression_params.xy * vreflect.z;
		vreflect.xy /= pos_decompression_params.zw * pos_decompression_params2.xy;
		
		// Рассчет начальной длины шага
		float depth_scale = clamp(pos.z, SSR_MARCH_MIN_DIST, SSR_MARCH_MAX_DIST);
		float scale = SSR_START_RAY * sqrt(depth_scale) / SSR_STEPS;
		
		// Считаем первичные отражения
		float2 tc_step = tc; float3 pos_step = pos; float2 steplen = scale;
		bool intersected = calc_intersection(SSR_STEPS, SSR_STEP_INCREASE, pos, vreflect, pos_step, tc_step, steplen);
		
		float4 ssr = 0;
		if(intersected)
		{
			// Уточняем позицию более мелкими шагами
			pos_step -= vreflect * steplen.y;
			steplen.x -= steplen.y * (1 + 1.0 / SSR_STEP_INCREASE); // Переносимся на начало предыдущего шага
			steplen.y /= SSR_REFINE_STEPS;
			calc_intersection(SSR_REFINE_STEPS, 1.0, pos, vreflect, pos_step, tc_step, steplen);
			// Выводим цвет
			ssr.xyz = s_image.Sample(smp_rtlinear, tc_step);
			ssr.w = calc_vignette(tc_step, SSR_VIGNETTE);
		}
		
		return ssr;
	}
#endif // REFLECTIONS_SSR_INCLUDED