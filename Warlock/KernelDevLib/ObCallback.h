#pragma once
#include <ntifs.h>

struct ObCallback
{
public:
	ObCallback::ObCallback(POBJECT_TYPE* ObType, POB_PRE_OPERATION_CALLBACK Func)
	{
		OB_OPERATION_REGISTRATION obRegister;
		obRegister.ObjectType = ObType;
		obRegister.Operations = OB_OPERATION_HANDLE_CREATE;
		obRegister.PreOperation = Func;
		obRegister.PostOperation = nullptr;

		OB_CALLBACK_REGISTRATION cbRegister;
		cbRegister.Version = OB_FLT_REGISTRATION_VERSION;
		cbRegister.OperationRegistrationCount = 1;
		cbRegister.Altitude = RTL_CONSTANT_STRING(L"489274.72039488934257");
		cbRegister.RegistrationContext = nullptr;
		cbRegister.OperationRegistration = &obRegister;

		status = ObRegisterCallbacks(&cbRegister, &hCallbacks);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ObRegisterCallbacks (%x)\n", status);
		}
	}

	NTSTATUS GetLastStatus() const { return status; };

	~ObCallback()
	{
		if (hCallbacks)
			ObUnRegisterCallbacks(hCallbacks);
	}
private:
	HANDLE hCallbacks = nullptr;
	NTSTATUS status;
};