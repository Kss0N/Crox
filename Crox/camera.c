/**

	@file      camera.c
	@brief     
	@details   
	@author    Jakob Kristersson <jakob.kristerrson@bredband.net> [Kss0N]
	@date      17.04.2023

**/

#include "camera.h"


const vec3 UP = { 0, 1.0f, 0.0 };
const vec3 DEFAULT_DIR = { 0,0,1 };

#define UPDATE_CAMERA_MATRIX(cam) mat4x4_mul(cam->matrix, cam->_projection, cam->_view);

inline void updateView(_In_ struct Camera* c)
{
	vec3 center;
	mat4x4_look_at(c->_view, c->wPos, vec3_add(center, c->wPos, c->wDir), UP);
}


//
// Public Interfaces
//


void camera_worldPosition_set(_Inout_ struct Camera* c, _In_opt_ const vec3 nPos)
{
	if (nPos)
	{
		vec3_dup(c->wPos, nPos);
	}
	else
		vec3_scale(c->wPos, c->wPos, 0);

	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_direction_set(_Inout_ struct Camera* c, _In_opt_ const vec3 nDir)
{
	vec3 norm;

	vec3_dup(c->wDir, nDir ? vec3_norm(norm, nDir) : DEFAULT_DIR);

	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_move_world(_Inout_ struct Camera* c, _In_ const vec3 delta)
{
	vec3_add(c->wPos, c->wPos, delta);

	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_move_local(_Inout_ struct Camera* c, _In_ const vec3 delta)
{
	vec3 temp;
	vec3 right;
	vec3_norm(right, vec3_mul_cross(right, UP, c->wDir));

	vec3_add(c->wPos, c->wPos, vec3_scale(temp, right, delta[0]));
	vec3_add(c->wPos, c->wPos, vec3_scale(temp, UP, delta[1]));
	vec3_add(c->wPos, c->wPos, vec3_scale(temp, c->wDir, delta[2]));

	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_pitch(_Inout_ struct Camera* c, _In_ float dX)
{
	camera_pitch_limited(c, dX, toRadians(360.0f));
}

void camera_pitch_limited(_Inout_ struct Camera* c, _In_ float dX, _In_ float maxAngle)
{
	//vertically:
	//newDir = rotate(dir, -yaw, |dir x up|) = rotate(-yaw * |dir x up|) * dir
	
	vec3 right;
	vec3_norm(right, vec3_mul_cross(right, c->wDir, UP));

	mat4 tmpM;	
	mat4x4_rotate(tmpM, mat4x4_identity(tmpM), right[0], right[1], right[2], dX);

	vec4 newDir;
	mat4x4_mul_vec4(newDir, tmpM, c->wDir);

	if (fabs(vec3_angle(newDir, UP)) - toRadians(90) <= maxAngle)
		vec3_dup(c->wDir, newDir);

	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_yaw(_Inout_ struct Camera* c, _In_ float dY)
{
	// horizontally:
	// dir = rotate(dir, -yaw, Up)
	
	mat4 tmpM;
	mat4x4_rotate(tmpM, mat4x4_identity(tmpM), UP[0], UP[1], UP[2], dY);
	
	vec4 newDir;
	mat4x4_mul_vec4(newDir, 
		tmpM, 
		c->wDir);
	
	vec3_dup(c->wDir, newDir);
	
	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_view_set(_Inout_ struct Camera* c, _In_opt_ const vec3 pos, _In_opt_ const vec3 dir)
{
	if (pos)
		vec3_dup(c->wPos, pos);
	else
		vec3_scale(c->wPos, c->wPos, 0);
	vec3_dup(c->wDir, dir != NULL ? dir : DEFAULT_DIR);

	updateView(c);
	UPDATE_CAMERA_MATRIX(c);
}



void camera_projection_set(_Inout_ struct Camera* c, _In_ const mat4 projection)
{
	mat4x4_dup(c->_projection, projection);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_projection_perspective(_Inout_ struct Camera* c, _In_ float fov, _In_ float ar, _In_ float zNear, _In_ float zFar)
{
	mat4x4_perspective(c->_projection, fov, ar, zNear, zFar);
	UPDATE_CAMERA_MATRIX(c);
}

void camera_projection_orthographic(_Inout_ struct Camera* c, _In_ float left, _In_ float right, _In_ float top, _In_ float bottom, _In_ float zNear, _In_ float zFar)
{
	mat4x4_ortho(c->_projection, left, right, bottom, top, zNear, zFar);
	UPDATE_CAMERA_MATRIX(c);
}
