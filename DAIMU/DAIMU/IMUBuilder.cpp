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
			//Using printf here to avoid potential interleaving issues with cout
			printf("Current DB contents:\nIMUID: %d, IMUType: %s, IMUName: %s, Weight: %f\n", IMUID, IMUType, IMUName, Weight);
		}
		//clean up stmt
		sqlite3_finalize(stmt);
	}	
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
	if (verifyData(Data, IMUID) == 0)
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
	else if (verifyData(Data, IMUID) == -1)
		printf("Null value present, data thrown out.");
	else
		printf("At least one value not in expected range, data thrown out.");
}

int IMUBuilder::verifyData(string Data, int IMUID)
{
	int retval = 0;
	//Matches first IMU/IMUID combo.
	auto it = std::find_if(IMUVector.begin(), IMUVector.end(),
		[IMUID](const internalMeasurementUnit& IMU) { return IMU.IMUID == IMUID; });
	//Tokenize Data string to pull each piece of data out EX string: Rx: -2.43 Ry: 1.45 Rz: .89
	char* context = NULL;
	char* context2 = NULL;
	char* ocontext = NULL;
	char* ocontext2 = NULL;
	char* DataCopy = _strdup(Data.c_str());
	char* oDataCopy = _strdup(it->Data.c_str());
	//First token is the data type & value
	char* token = strtok_s(DataCopy, " ", &context);
	char* otoken = strtok_s(oDataCopy, " ", &ocontext);
	while (token != NULL)
	{
		char* tokenCopy = _strdup(token);
		char* otokenCopy = _strdup(otoken);

		char* type = strtok_s(tokenCopy, ":", &context2);
		long double value = stold(strtok_s(NULL, ":", &context2));
		//if there is an old value, do this, else set otype to type and ovalue to value
		char* otype = type;
		long double ovalue = value;
		//If there is a previous set of data, get its data set to compare to
		if (strncmp(DataCopy, oDataCopy, 2) == 0)
		{
			otype = strtok_s(otokenCopy, ":", &ocontext2);
			ovalue = stold(strtok_s(NULL, ":", &ocontext2));
		}

		//One of the values is null, we can not trust this whole instance
		if (value == NULL)
			return -1;

		//NOTE: for all values below please consult the manuals of the specific units you are using. They
		// will specify limits to guide where error is definately occuring.

		//it is a gyro, or sonar direction/beamwidth - degree measurements can not be beyond 360
		if (type == "Rx" || "Ry" || "Rz" || "Sd" || "Sb")
		{
			if (value > 360 || value < -360)
				retval++;
			//Change greater than 270deg within processing/ping time is not realistic
			else if (abs(ovalue - value) > 270)
				retval++;
		}
		//it is an accelerometer - bot can not move faster than 10m/s
		else if (type == "Ax" || "Ay" || "Az")
		{
			if (value > 10 || value < -10)
				retval++;
			//Change greater than 3.5m/s within processing/ping time is not realistic
			else if (abs(ovalue - value) > 3.5)
				retval++;
		}
		//it is a barometer - pressure will not be greater than 1 bar
		else if (type == "Bar")
		{
			if (value > 3 || value < -1)
				retval++;
			//Change greater than .75 bar within processing/ping time is not realistic
			else if (abs(ovalue - value) > .75)
				retval++;
		}
		//it is a magnetometer - using the WitMotion WC901C +/-4900-microTesla is the range of measurement
		else if (type == "Mx" || "My" || "Mz")
		{
			if (value > 5000 || value < -5000)
				retval++;
			//Change greater than 2250 microTesla within processing/ping time is not realistic
			else if (abs(ovalue - value) > 2250)
				retval++;
		}
		//it is an altimeter - OIT is ~1248m, bot should not be going 100m below sea level
		//to go further, using the Lidar-Lite v3 range is .05m to 40m
		else if (type == "H")
		{
			if (value > 40 || value < -.05)
				retval++;
			//Change greater than 10m within processing/ping time is not realistic
			else if (abs(ovalue - value) > 10)
				retval++;
		}
		//it is a sonar array - specifically object depth from sub, ours has a ~500m range and should only face front
		else if (type == "Sd")
		{
			if (value > 500 || value < 0)
				retval++;
		}
		//it is a sonar array - specifically object distance from sub, ours has a ~500m range and should only face front
		else if (type == "Sr")
		{
			if (value > 500 || value < 0)
				retval++;
		}
		//it is a sonar array - specifically object angle from sub
		else if (type == "Sa")
		{
			if (value > 360 || value < -360)
				retval++;
		}
		token = strtok_s(NULL, " ", &context);
		otoken = strtok_s(NULL, " ", &ocontext);
	}
	free(DataCopy);
	free(oDataCopy);
	return retval;
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

void IMUBuilder::clearTables()
{
	int rc = 1;
	char* errmsg;
	string tableCleanUp = "DELETE FROM IMUTable;";
	rc = sqlite3_exec(db, tableCleanUp.c_str(), NULL, NULL, &errmsg);
	if (rc != 0)
		printf("Error deleting from IMUTable: %s\n", errmsg);
	tableCleanUp = "DELETE FROM OptTable;";
	rc = sqlite3_exec(db, tableCleanUp.c_str(), NULL, NULL, &errmsg);
	if (rc != 0)
		printf("Error deleting from OptTable: %s\n", errmsg);
	if (ibDebugMode > 0)
	{
		sqlite3_stmt* stmt;
		string cmd = "SELECT * FROM IMUTable;";
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

IMUBuilder::~IMUBuilder()
{
	int rc = 1;
	while (rc != 0)
	{
		rc = sqlite3_close(db);
	}
}