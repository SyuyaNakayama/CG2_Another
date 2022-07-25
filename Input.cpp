#include "Input.h"
#include <cassert>

void DirectInput::Initialize(WNDCLASSEX w)
{
	DirectInput8Create(w.hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&input, nullptr);
}
void Keyboard::GetInstance(WNDCLASSEX w)
{
	Initialize(w);
	input->CreateDevice(GUID_SysKeyboard, &device, NULL);
}
void Keyboard::Set(HWND hwnd)
{
	device->SetDataFormat(&c_dfDIKeyboard);// •W€Œ`Ž®
	device->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
}
void Keyboard::GetDeviceState()
{
	device->GetDeviceState(sizeof(key), key);
}
void Keyboard::TransferOldkey()
{
	for (size_t i = 0; i < sizeof(oldkey); i++) { oldkey[i] = key[i]; }
}
bool Keyboard::IsInput(const int KEY)
{
	if (key[KEY]) { return true; }
	return false;
}
bool Keyboard::IsTrigger(const int KEY)
{
	return (!oldkey[KEY] && key[KEY]);
	return false;
}
float Keyboard::Move(const int KEY1, const int KEY2, const float spd)
{
	return (IsInput(KEY1) - IsInput(KEY2)) * spd;
}