#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

void printRect(RECT rect) {
	cout << "origin = " << rect.left << ',' << rect.top << "; size = " << rect.right - rect.left << 'x' << rect.bottom - rect.top;
}

void printMonitor(HMONITOR monitor) {
	MONITORINFOEXW mi;
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfoW(monitor, &mi)) {
		cout << "Error...\n";
		cout << monitor << '\n';
		exit(1);
	}

	wcout << L"Device name: " << mi.szDevice;
	cout << "\nMonitor rect: ";
	printRect(mi.rcMonitor);
	cout << "\nWork area rect: ";
	printRect(mi.rcWork);
	cout << "\n";
}

int main() {
	cout << "Position this window on some monitor and then press Enter: " << flush;
	string _;
	getline(cin, _);

	vector<HMONITOR> allMonitors;
	auto enumProc = [](HMONITOR monitor, HDC hdc, LPRECT pRect, LPARAM vecAddr) -> BOOL {
		reinterpret_cast<vector<HMONITOR>*>(vecAddr)->push_back(monitor);
		return TRUE;
	};

	HMONITOR thisWindowMonitor = MonitorFromWindow(GetConsoleWindow(), MONITOR_DEFAULTTONEAREST);
	EnumDisplayMonitors(NULL, NULL, enumProc, reinterpret_cast<LPARAM>(&allMonitors));

	for (auto monitor : allMonitors) {
		cout << '\n';
		if (monitor == thisWindowMonitor) {
			cout << "(current monitor) ";
		}

		printMonitor(monitor);
	}

	system("pause");
}