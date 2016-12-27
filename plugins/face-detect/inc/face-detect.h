#pragma once

int __stdcall FaceDetect(const char* filename, const char* window_title, void* parent_hwnd);
bool __stdcall FaceDestroyWindow(const char* window_title);
int __stdcall ImageFaceDetect(const char* filename);