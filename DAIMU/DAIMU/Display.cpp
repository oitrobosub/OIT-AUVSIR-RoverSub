#include "Display.h"

Display::Display()
{
	displayState = checkDisplayConnection();
}

void Display::displayCondensedData()
{
	if (displayState == 0)
	{
		printf("Display exists");
	}
}

void Display::DBToExcel(string cmd, string fileName)
{
	IMUBuilder imuBuilder; 
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
	//Place each item from the DB into a cell in a .csv with the columns named accoring to the DB table
	//Open file for write
	std::ofstream file(fileName);

	//Write column names
	int cols = sqlite3_column_count(stmt);
	for (int i = 0; i < cols; i++) 
	{
		file << sqlite3_column_name(stmt, i);
		//If not at final column name
		if (i < cols - 1) 
		{
			file << ",";
		}
	}
	file << "\n";

	//Write data from each row into the file
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		for (int i = 0; i < cols; i++) 
		{
			file << sqlite3_column_text(stmt, i);
			//If not at final column name
			if (i < cols - 1)
			{
				file << ",";
			}
		}
		file << "\n";
	}
	//clean up stmt
	sqlite3_finalize(stmt);
}

bool Display::checkDisplayConnection()
{
	return false;
}

Display::~Display()
{
}