#include "Display.h"

Display::Display(bool connection)
{
	checkDisplayConnection(connection);
}

void Display::displayCondensedData()
{
	if (displayState == 1)
	{
		IMUBuilder imuBuilder;
		printf("Display exists");
		string cmd = "SELECT * FROM OptTable;";
		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
		//execute created stmt
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			const char* OptForAccuracy = (const char*)sqlite3_column_text(stmt, 1);
			const char* OptForPositionF = (const char*)sqlite3_column_text(stmt, 2);
			const char* OptForPositionR = (const char*)sqlite3_column_text(stmt, 3);
			const char* OptForPositionS = (const char*)sqlite3_column_text(stmt, 4);
			const char* OptForPositionP = (const char*)sqlite3_column_text(stmt, 5);

			//Using printf here to avoid potential interleaving issues with cout
			printf("OptForAccuracy: %s, OptForPositionF: %s, OptForPositionR: %s, OptForPositionS: %s, OptForPositionP: %s\n", OptForAccuracy, OptForPositionF, OptForPositionR, OptForPositionS, OptForPositionP);
		}
		//clean up stmt
		sqlite3_finalize(stmt);
	}
	return;
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
		const unsigned char *smt;
		for (int i = 0; i < cols; i++) //Access error at i = 4
		{
			smt = sqlite3_column_text(stmt, i);
			printf("Statement %s\n", smt); //Null at i = 4
			if (smt == NULL)
				file << "NULL";
			else
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

void Display::checkDisplayConnection(bool connection)
{
	displayState = connection;
	return;
}

Display::~Display()
{
}