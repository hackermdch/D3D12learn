#include "expamle.h"
using namespace std;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
	HMODULE dll = LoadLibrary(TEXT("shcore.dll"));
	if (dll != nullptr) {
		((HRESULT(_stdcall*)(int)) GetProcAddress(dll, "SetProcessDpiAwareness"))(2);
		FreeLibrary(dll);
	}
	App app;
	return app.Run();
}