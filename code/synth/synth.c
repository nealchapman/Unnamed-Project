#include "Engine/setup.h"
#include "Patch/patch.h"

int main(void)
{

	//~/Engine/setup.h
	initEngine();

	//~/Patch/patch.h
	initPatch();

	//~/Engine/setup.h
	startEngine();

	while(1);

	return 1;

}
