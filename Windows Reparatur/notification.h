#pragma once
#include <Windows.h>
#include <string.h>
HRESULT register_notification_activator();
HRESULT send_notification(std::wstring string);