#pragma once
#include <stdio.h>
#include <fstream>
#include "IMUBuilder.h"

class Display
{
	public:
		Display();
		void displayCondensedData();
		void DBToExcel(string cmd, string fileName);
		~Display();

	private:
		bool displayState;

		bool checkDisplayConnection();
};

