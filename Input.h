#pragma once
#include <dinput.h>

class DirectInput
{
protected:
	IDirectInput8* input;

	void Initialize(WNDCLASSEX w);
};

class Keyboard :DirectInput
{
private:
	BYTE key[256];
	BYTE oldkey[256];

public:
	IDirectInputDevice8* device;

	void GetInstance(WNDCLASSEX w);
	void Set(HWND hwnd);
	void GetDeviceState();
	void TransferOldkey();
	bool IsInput(const int KEY);
	bool IsTrigger(const int KEY);
	float Move(const int KEY1, const int KEY2, const float spd);
};