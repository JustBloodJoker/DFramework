#include "TestBaseWindow.h"

void RunOneWindow() {
	auto wnd1 = new TestBaseWindow(L"wnd1", 100, 100, false);
	std::thread t1([wnd1]() { wnd1->__START(); });
	t1.join();
	delete wnd1;
}

void RunMultipleWindow() {
	auto wnd1 = new TestBaseWindow(L"wnd1", 100, 100, false);
	auto wnd2 = new TestBaseWindow(L"wnd2", 300, 300, false);
	auto wnd3 = new TestBaseWindow(L"wnd3", 600, 600, false);

	std::thread t1([wnd1]() { wnd1->__START(); });
	std::thread t2([wnd2]() { wnd2->__START(); });
	std::thread t3([wnd3]() { wnd3->__START(); });
	
	t1.join();
	t2.join();
	t3.join();

	delete wnd1;
	delete wnd2;
	delete wnd3;
}

int main() {

	RunOneWindow();

	RunMultipleWindow();
}
