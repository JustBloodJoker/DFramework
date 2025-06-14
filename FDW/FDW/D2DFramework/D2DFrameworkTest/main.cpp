#include "MyUIApp.h"
#include "BezierCurveTestRender.h"


void StandaloneTest() {
    BezierCurveTestRender* ren = new BezierCurveTestRender();

    ren->__START();

    delete ren;
}

void UITest() {
	MyUIApp* app = new MyUIApp();
	app->__START();
	delete app;
}

int main() {

    StandaloneTest();
	UITest();
	
	return 0;
}