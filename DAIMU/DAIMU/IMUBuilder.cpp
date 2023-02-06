#include "IMUBuilder.h"

IMUBuilder::IMUBuilder() 
{ 
	openDB();
}

IMUBuilder::IMUBuilder(int debugMode)
{
	ibDebugMode = debugMode;
	openDB();
}

void IMUBuilder::IMUPopulator(string IMUs)
{
	//context tracks the starting pos of the next substring
	char* context = NULL;
	char* context2 = NULL;
	char* IMUsCopy = _strdup(IMUs.c_str());
	//First token has if display is present
	char* token = strtok_s(IMUsCopy, "::", &context);
	//This grabs the string of IMU information
	token = strtok_s(NULL, ",,", &context);
	while (token != NULL)
	{
		//Copy of token since token needs to continue splitting on " "
		//tokenCopy can split the substring token on "::"
		char* tokenCopy = _strdup(token);
		int IMUID = atoi(strtok_s(tokenCopy, "::", &context2));
		char* IMUType = strtok_s(NULL, "::", &context2);
		char* IMUName = strtok_s(NULL, "::", &context2);
		float Weight = atof(strtok_s(NULL, "::", &context2));

		internalMeasurementUnit IMU = getIMU(IMUID);
		if (IMU.IMUName == "")
			setIMU(IMUID, IMUType, IMUName, Weight);
		else
		{
			//context2 is the remainder of the passed in IMU string - aka the data piece with a prepended ':'
			char* data = context2;
			string Data = data;
			Data.erase(0, 1);
			setIMUData(IMUID, Data);
		}
		token = strtok_s(NULL, ",,", &context);
	}
	free(IMUsCopy);
}

IMUBuilder::internalMeasurementUnit IMUBuilder::getIMU(int IMUID)
{
	//Matches first IMU/IMUID combo.
	auto it = std::find_if(IMUVector.begin(), IMUVector.end(),
		[IMUID](const internalMeasurementUnit& IMU) { return IMU.IMUID == IMUID; });
	if (it != IMUVector.end())
		return *it;
	else
		return internalMeasurementUnit();
}

void IMUBuilder::setIMU(int IMUID, string IMUType, string IMUName, float Weight)
{
	internalMeasurementUnit IMU = internalMeasurementUnit{};
	IMU.IMUID = IMUID;
	IMU.IMUType = IMUType;
	IMU.IMUName = IMUName;
	IMU.Weight = Weight;
	IMUVector.push_back(IMU);

	//Place IMU into database
	sqlite3_stmt* stmt;
	string cmd = "INSERT INTO IMUTable (IMUID, IMUType, IMUName, Weight) VALUES (?, ?, ?, ?);";
	sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, IMUID);
	sqlite3_bind_text(stmt, 2, IMUType.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, IMUName.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 4, Weight); 
	//execute created stmt
	sqlite3_step(stmt);
	//clean up stmt
	sqlite3_finalize(stmt);

	//NOTE: Debug print
	cmd = "SELECT * FROM IMUTable;";
	sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, NULL);
	//execute created stmt
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		int IMUID = sqlite3_column_int(stmt, 0);
		const char* IMUType = (const char*)sqlite3_column_text(stmt, 1);
		const char* IMUName = (const char*)sqlite3_column_text(stmt, 2);
		double Weight = sqlite3_column_double(stmt, 3);
		//Using printf here to avoid potential interleaving issues with cout
		printf("Current DB contents:\nIMUID: %d, IMUType: %s, IMUName: %s, Weight: %f\n", IMUID, IMUType, IMUName, Weight);
	}
	//clean up stmt
	sqlite3_finalize(stmt);
}

string IMUBuilder::getIMUData(int IMUID)
{
	//Matches first IMU/IMUID combo.
	auto it = std::find_if(IMUVector.begin(), IMUVector.end(),
		[IMUID](const internalMeasurementUnit& IMU) { return IMU.IMUID == IMUID; });
	return it->Data;
}

void IMUBuilder::setIMUData(int IMUID, string Data)
{
	if (verifyData(Data) == 0)
	{
		//Matches first IMU/IMUID combo.
		auto it = std::find_if(IMUVector.begin(), IMUVector.end(),
			[IMUID](const internalMeasurementUnit& IMU) { return IMU.IMUID == IMUID; });
		it->Data = Data;

		//Update the IMU in the sqlite database
		string cmd = "UPDATE IMUTable SET Data = ? WHERE IMUID = ?;";
		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, NULL);
		sqlite3_bind_text(stmt, 1, Data.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_int(stmt, 2, IMUID);
		//execute created stmt
		sqlite3_step(stmt);
		//clean up stmt
		sqlite3_finalize(stmt);

		if (ibDebugMode > 0)
		{
			cmd = "SELECT * FROM IMUTable;";
			sqlite3_prepare_v2(db, cmd.c_str(), -1, &stmt, NULL);
			//execute created stmt
			while (sqlite3_step(stmt) == SQLITE_ROW)
			{
				int IMUID = sqlite3_column_int(stmt, 0);
				const char* IMUType = (const char*)sqlite3_column_text(stmt, 1);
				const char* IMUName = (const char*)sqlite3_column_text(stmt, 2);
				double Weight = sqlite3_column_double(stmt, 3);
				const char* Data = (const char*)sqlite3_column_text(stmt, 4);

				//Using printf here to avoid potential interleaving issues with cout
				printf("IMUID: %d, IMUType: %s, IMUName: %s, Weight: %f, Data: %s\n", IMUID, IMUType, IMUName, Weight, Data);
			}
			//clean up stmt
			sqlite3_finalize(stmt);
		}
	}
}

int IMUBuilder::verifyData(string Data)
{
	return 0;
}

void IMUBuilder::openDB()
{
	//rc = return code -> this is common sqlite3 naming convention
	int rc = 1;
	while(rc != 0)
	{
		rc = sqlite3_open("DAIMU.db", &db);
		string tableCreation = "CREATE TABLE IF NOT EXISTS IMUTable (IMUID INT, IMUType TEXT, IMUName TEXT, Weight REAL, Data TEXT);";
		rc = sqlite3_exec(db, tableCreation.c_str(), NULL, NULL, NULL);
	}
}

IMUBuilder::~IMUBuilder()
{
	string tableCleanUp = "DELETE * FROM IMUTable;";
	sqlite3_exec(db, tableCleanUp.c_str(), NULL, NULL, NULL);
	int rc = 1;
	while (rc != 0)
	{
		rc = sqlite3_close(db);
	}
}