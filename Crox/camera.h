#pragma once
#include <linmath.h>
#include <sal.h>
#define _USE_MATH_DEFINES
#include <math.h>

struct Camera
{
	//plz don't modify directly

	vec3
		wPos,
		wDir;

	mat4 
		matrix,
		_view,
		_projection;
};

void camera_worldPosition_set		(_Inout_ struct Camera* c, _In_opt_ const vec3 nPos);	//if new Position is NULL, Camera will be moved to world centerl
void camera_direction_set			(_Inout_ struct Camera* c, _In_opt_ const vec3 nDir);	//if new Direction is NULL, Camera will get direction {0, 0, 1}
void camera_move_world				(_Inout_ struct Camera* c, _In_ const vec3 delta);	// The move happens in world coordinates. 
void camera_move_local				(_Inout_ struct Camera* c, _In_ const vec3 delta);	// The move happens relative to the camera: E.g. if moved the right, a viewer will interpret it as a right move.
void camera_pitch					(_Inout_ struct Camera* c, _In_ float dX);
void camera_pitch_limited			(_Inout_ struct Camera* c, _In_ float dX, _In_ float maxAngle);
void camera_yaw						(_Inout_ struct Camera* c, _In_ float dY);

void camera_view_set				(_Inout_ struct Camera* c, _In_opt_ const vec3 pos, _In_opt_ const vec3 dir);
void camera_projection_set			(_Inout_ struct Camera* c, _In_ const mat4 projection);
void camera_projection_perspective	(_Inout_ struct Camera* c, _In_ float fov, _In_ float ar, _In_ float zNear, _In_ float zFar);
void camera_projection_orthographic	(_Inout_ struct Camera* c, _In_ float left, _In_ float right, _In_ float top, _In_ float bottom, _In_ float zNear, _In_ float zFar);




inline float toRadians(float deg)
{
	return deg * (float)(180.0 / M_PI);
}