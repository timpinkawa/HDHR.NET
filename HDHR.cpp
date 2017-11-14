// HDHR.cpp : main project file.

#define DLL_IMPORT
#include "stdafx.h"
#include <hdhomerun.h>

using namespace System;
using namespace System::Runtime::InteropServices;
namespace HDHomeRunWrapper
{

public ref class StreamEventArgs : System::ComponentModel::CancelEventArgs
{
public:
	StreamEventArgs(int bytesReceived)
	{
		this->bytesReceived = bytesReceived;
	}
	property int BytesReceived
	{
		int get()
		{
			return bytesReceived;
		}
	}
private:
	int bytesReceived;

};

public delegate void StreamEventHandler(Object^ sender, StreamEventArgs^ e);

public ref class HDHomeRun
{
private:
	struct hdhomerun_device_t* hd;

public:
	event StreamEventHandler^ StreamReceived;
	HDHomeRun(String^ deviceStr)
	{
		
		WORD version = MAKEWORD(2, 2);
		WSADATA wsaData;

		WSAStartup(version, &wsaData);

		IntPtr p = Marshal::StringToHGlobalAnsi(deviceStr);
		hd = hdhomerun_device_create_from_str(static_cast<char*>(p.ToPointer()), NULL);
		Marshal::FreeHGlobal(p);
		uint32_t id = ::hdhomerun_device_get_device_id_requested(hd);
		bool valid = ::hdhomerun_discover_validate_device_id(id);
		
	}

	~HDHomeRun()
	{
		::hdhomerun_device_destroy(hd);
		WSACleanup();
	}

	void StartReceive(String^ tuner)
	{
		IntPtr p = Marshal::StringToHGlobalAnsi(tuner);
		const char* tuner_str = static_cast<const char*>(p.ToPointer());
		::hdhomerun_device_set_tuner_from_str(hd, tuner_str);
		::hdhomerun_device_stream_start(hd);
		Marshal::FreeHGlobal(p);
	}

	cli::array<byte>^ ReceiveBuffer()
	{
		size_t actual_size;

		uint8_t* ptr = ::hdhomerun_device_stream_recv(hd, VIDEO_DATA_BUFFER_SIZE_1S, &actual_size);
		if(ptr != NULL)
		{
			cli::array<byte>^ mBuffer = gcnew cli::array<byte>(actual_size);
			Marshal::Copy((IntPtr)ptr, mBuffer, 0, actual_size);
			return mBuffer;
		}
		return gcnew cli::array<byte>(0);
	}

	void StopReceive()
	{
		::hdhomerun_device_stream_stop(hd);
	}

	void Save(String^ tuner, System::IO::Stream^ output)
	{
		IntPtr p = Marshal::StringToHGlobalAnsi(tuner);
		const char* tuner_str = static_cast<const char*>(p.ToPointer());
		::hdhomerun_device_set_tuner_from_str(hd, tuner_str);
		::hdhomerun_device_stream_start(hd);

		int iterations = 0;
		while(iterations < 500)
		{
			size_t actual_size;

			uint8_t* ptr = ::hdhomerun_device_stream_recv(hd, VIDEO_DATA_BUFFER_SIZE_1S, &actual_size);
			if(ptr != NULL)
			{
				cli::array<unsigned char>^ mBuffer = gcnew cli::array<unsigned char>(actual_size);
				Marshal::Copy((IntPtr)ptr, mBuffer, 0, actual_size);
				output->Write(mBuffer, 0, actual_size);
				StreamEventArgs^ e = gcnew StreamEventArgs((int)actual_size);
				StreamReceived(this, e);
				if(e->Cancel)
					break;
				iterations++;
			}
			Sleep(50);
		}
		::hdhomerun_device_stream_stop(hd);
		Marshal::FreeHGlobal(p);
	}

	String^ GetVariable(String^ name)
	{
		char* pValue = NULL;
		char* pError = NULL;
		IntPtr p = Marshal::StringToHGlobalAnsi(name);

		int rc = ::hdhomerun_device_get_var(hd, static_cast<char*>(p.ToPointer()), &pValue, &pError);
		Marshal::FreeHGlobal(p);
		if(rc == 1)
			return gcnew String(pValue);
		else
			throw gcnew Exception(gcnew String(pError));
	}

	Boolean SetVariable(String^ name, String^ value)
	{
		char* pOutValue = NULL;
		char* pError = NULL;
		IntPtr pName = Marshal::StringToHGlobalAnsi(name);
		IntPtr pValue = Marshal::StringToHGlobalAnsi(value);
		int rc = ::hdhomerun_device_set_var(hd, static_cast<char*>(pName.ToPointer()), static_cast<char*>(pValue.ToPointer()), &pOutValue, &pError);

		Marshal::FreeHGlobal(pName);
		Marshal::FreeHGlobal(pValue);

		return rc == 1;
	}

	property String^ Model
	{
		String^ get()
		{
			const char* model = ::hdhomerun_device_get_model_str(hd);
			String^ modelStr = gcnew String(model);
			return modelStr;
		}
	}
};
}
int main(int argc, char* argv[])
{
	//Sprocket::HDHR^ h = gcnew Sprocket::HDHR("sup");
	//String^ lol = h->Model;
	return 0;
}
