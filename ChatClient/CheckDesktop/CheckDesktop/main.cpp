#include <iostream>

#include "MainForm.h"

int main(void)
{
	MainForm mainForm;

	mainForm.Init();

	mainForm.CreateGUI();

	mainForm.ShowModal();

	return 0;
}