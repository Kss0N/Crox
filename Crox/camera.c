/**

	@file      camera.c
	@brief     
	@details   
	@author    Jakob Kristersson <jakob.kristerrson@bredband.net> [Kss0N]
	@date      17.04.2023

**/

#include "camera.h"
#define _USE_MATH_DEFINES
#include <math.h>

const vec3 UP = { 0, 1.0f, 1.0 };
const vec3 DEFAULT_DIR = { 0,0,1 };

#define UPDATE_CAMERA_MATRIX(cam) mat4x4_mul(cam->matrix, cam->_projection, cam->_view);

void camera_worldPosition_set(_Inout_ struct Camera* c, _In_opt_ vec3 nPos)
{
	if (nPos)
	{
		vec3_dup(c->wPos, nPos);
	}
	else
		vec3_scale(c->wPos, c->wPos, 0);

	UPDATE_CAMERA_MATRIX(c);
}

void camera_direction_set(_Inout_ struct Camera* c, _In_opt_ vec3 nDir)
{
	if (nDir)
	{
		vec3_dup(c->wDir, nDir);
	}
	else
		vec3_dup(c->wDir, DEFAULT_DIR);

	UPDATE_CAMERA_MATRIX(c);
}

void camera_move(_Inout_ struct Camera* c, _In_ vec3 delta)
{
	vec3_add(c->wDir, c->wDir, delta);
	
	UPDATE_CAMERA_MATRIX(c);
}

void camera_pitch(_Inout_ struct Camera* c, _In_ float dX)
{

	UPDATE_CAMERA_MATRIX(c);
}

void camera_pitch_limited(_Inout_ struct Camera* c, _In_ float dX, _In_ float maxAngle)
{

	UPDATE_CAMERA_MATRIX(c);
}

void camera_yaw(_Inout_ struct Camera* c, _In_ float dY)
{

	UPDATE_CAMERA_MATRIX(c);
}

void camera_view_set(_Inout_ struct Camera* c, _In_ vec3 pos, _In_ vec3 dir)
{

	vec3 center;

	mat4x4_look_at(c->_view, pos, vec3_add(center, pos, dir), UP);

	UPDATE_CAMERA_MATRIX(c);
}

void camera_projection_set(_Inout_ struct Camera* c, _In_ mat4 projection)
{

	UPDATE_CAMERA_MATRIX(c);
}

void camera_projection_perspective(_Inout_ struct Camera* c, _In_ float fov, _In_ float ar, _In_ float zNear, _In_ float zFar)
{
	mat4x4_perspective(c->_projection, fov, ar, zNear, zFar);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_projection_orthographic(_Inout_ struct Camera* c, _In_ float left, _In_ float right, _In_ float top, _In_ float bottom)
{

	UPDATE_CAMERA_MATRIX(c);
}
