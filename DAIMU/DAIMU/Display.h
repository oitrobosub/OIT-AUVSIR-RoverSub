#pragma once
#include <stdio.h>
#include <fstream>
#include "IMUBuilder.h"

class Display
{
	public:
		Display(bool connection);
		void displayCondensedData();
		void DBToExcel(string cmd, string fileName);
		~Display();

	private:
		bool displayState;

		void checkDisplayConnection(bool connection);
};

