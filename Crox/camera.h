#pragma once
#include <linmath.h>
#include <sal.h>

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

void camera_worldPosition_set		(_Inout_ struct Camera* c, _In_opt_ vec3 nPos);	//if new Position is NULL, Camera will be moved to world centerl
void camera_direction_set			(_Inout_ struct Camera* c, _In_opt_ vec3 nDir);	//if new Direction is NULL, Camera will get direction {0, 0, 1}
void camera_move					(_Inout_ struct Camera* c, _In_ vec3 delta);
void camera_pitch					(_Inout_ struct Camera* c, _In_ float dX);
void camera_pitch_limited			(_Inout_ struct Camera* c, _In_ float dX, _In_ float maxAngle);
void camera_yaw						(_Inout_ struct Camera* c, _In_ float dY);

void camera_view_set				(_Inout_ struct Camera* c, _In_ vec3 pos, _In_ vec3 dir);
void camera_projection_set			(_Inout_ struct Camera* c, _In_ mat4 projection);
void camera_projection_perspective	(_Inout_ struct Camera* c, _In_ float fov, _In_ float ar, _In_ float zNear, _In_ float zFar);
void camera_projection_orthographic	(_Inout_ struct Camera* c, _In_ float left, _In_ float right, _In_ float top, _In_ float bottom);
