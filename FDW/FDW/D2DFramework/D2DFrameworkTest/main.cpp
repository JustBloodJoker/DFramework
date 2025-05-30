#include "BezierCurveTestRender.h"


void StandaloneTest() {
	BezierCurveTestRender* ren = new BezierCurveTestRender();

	ren->__START();

	delete ren;
}


int main() {
	StandaloneTest();


	return 0;
}