#include "CondenseData.h"

CondenseData::CondenseData()
{
	//Set up table if it does not exist
	IMUBuilder imuBuilder;
	//rc = return code -> this is common sqlite3 naming convention
	int rc = 1;
	while (rc != 0)
	{
		string tableCreation = "CREATE TABLE IF NOT EXISTS OptTable (ID INT, OptForAccuracy TEXT, OptForPositionF TEXT, OptForPositionR TEXT, OptForPositionS TEXT, OptForPositionP TEXT);";
		rc = sqlite3_exec(imuBuilder.db, tableCreation.c_str(), NULL, NULL, NULL);
	}
	//Set row 0 ID in database
	int ID = 0;
	sqlite3_stmt* stmt;
	string cmd = "INSERT INTO OptTable (ID) VALUES (?);";
	sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, ID);
	//execute created stmt
	sqlite3_step(stmt);
	//clean up stmt
	sqlite3_finalize(stmt);
}

CondenseData::CondenseData(int debugMode)
{
	cdDebugMode = debugMode;
	//Set up table if it does not exist
	IMUBuilder imuBuilder;
	//rc = return code -> this is common sqlite3 naming convention
	int rc = 1; 
	while (rc != 0)
	{
		string tableCreation = "CREATE TABLE IF NOT EXISTS OptTable (ID INT, OptForAccuracy TEXT, OptForPositionF TEXT, OptForPositionR TEXT, OptForPositionS TEXT, OptForPositionP TEXT);";
		rc = sqlite3_exec(imuBuilder.db, tableCreation.c_str(), NULL, NULL, NULL);
	}
	//Set row 0 ID in database
	int ID = 0;
	sqlite3_stmt* stmt;
	string cmd = "INSERT INTO OptTable (ID) VALUES (?);";
	sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
	sqlite3_bind_int(stmt, 1, ID);
	//execute created stmt
	sqlite3_step(stmt);
	//clean up stmt
	sqlite3_finalize(stmt);
}

void CondenseData::condenser()
{
	int ID = 0;
	IMUBuilder::internalMeasurementUnit IMU = accessData(ID);
	//While IMU struct is populated
	while (IMU.Weight != NULL)
	{
		//Form and sort list by weight
		basicSortIMUs(IMU);
		//access IMU
		ID++;
		IMU = accessData(ID);
	}
	//optimize the IMUs in the list
	for (auto it = IMUList.begin(); it != IMUList.end(); it++)
	{
		IMU = it->first;
		string position = it->second;
		optimizeOverAll(IMU);
		optimizeOnPosition(IMU, position);
	}
	//TODO: here accuracyOpt is fully built. push_back onto the accuracyOptList then another push_back for each part of accuracyOpt?
	accuracyOptList.push_back(accuracyOpt);
	/*for (auto it = begin(accuracyOpt); it != end(accuracyOpt); ++it)
		accuracyOptList[i].push_back();*/
	optCount++;
	if (optCount == 3)
	{
		adjustOpt(optCount);
		optCount = 0;
	}
	return;
}


void CondenseData::adjustOpt(int optCount)
{
	//get acOpt from accuracyOptList - should encompass whole first IMUList
	vector<std::pair<string, long double>> oldAccuracyOpt = accuracyOptList.front();
	//iterate through oldAcOpt and acOpt at the same time doing the following:
	//NOTE: This only works since no IMUs can be added after robot starts up. So the optimizations will have the same data in the same order
	for (auto it = begin(oldAccuracyOpt), it2 = begin(accuracyOpt); it != end(oldAccuracyOpt) && it2 != end(accuracyOpt); ++it, ++it2) {
		//it = oldAccuracyOpt, it2 = accuracyOpt
		long double oldAccuracyOptValue = it->second;
		long double accuracyOptValue = it2->second;
		//average = ((average * numValues) - valueToRemove) / (numValues - 1);
		long double newAccuracyOptValue = ((accuracyOptValue * optCount) - oldAccuracyOptValue) / (optCount - 1);
		//replace most recent acOpt value with newly created acOpt value
		it2->second = newAccuracyOptValue;
	}
	//remove oldest accuracy optimization
	accuracyOptList.erase(accuracyOptList.begin());
}

void CondenseData::optimizeOverAll(IMUBuilder::internalMeasurementUnit IMU)
{
	//Tokenize Data string to pull each piece of data out EX string: Rx: -2.43 Ry: 1.45 Rz: .89
	char* context = NULL;
	char* context2 = NULL;
	char* IMUDataCopy = _strdup(IMU.Data.c_str());
	//First token is the data type & value
	char* token = strtok_s(IMUDataCopy, " ", &context);
	while (token != NULL)
	{
		char* tokenCopy = _strdup(token);

		char* type = strtok_s(tokenCopy, ":", &context2);
		long double value = stold(strtok_s(NULL, ":", &context2));

		//Check for existing entry of same type of data
		auto it = std::find_if(accuracyOpt.begin(), accuracyOpt.end(),
			[type](std::pair<string, long double>& pair) {return pair.first == type; });
		//If it does exist, create one datapoint optimized for accuracy
		if (it != accuracyOpt.end())
		{
			//NOTE: ASSUMPTION - it->second is the old value, it is multiplied by a weight of .99 as I expect
			// the stored to be only 99% accurate. If this guess is wrong, change the .99 to allow for more
			// or less error. If you want it to be even more accurate, we can brainstorm a struct to use but it 
			// would cause data bloat.

			long double newValue = ((it->second * .99) + (value * IMU.Weight)) / (.99 + IMU.Weight);
			it->second = newValue;
		}
		//If it does not exist, place the pair into the vector and assume it is accurate
		else
			accuracyOpt.push_back({ type, value });

		//next data type/value pair
		token = strtok_s(NULL, " ", &context);
	}
	//TODO: pushing each imu, not total list of imus
	//accuracyOptList.push_back(accuracyOpt);
	free(IMUDataCopy);

	storeData("accuracyOpt");
}


void CondenseData::optimizeOnPosition(IMUBuilder::internalMeasurementUnit IMU, string position)
{
	//Used to determine which position optimization will be done on the IMU
	int loopCount = 0;
	//NOTE: The way this is implemented, all the position optimization work will be done here and now.
	//The reason for this is that any program that reaches out for a value will get it immediately. No 
	//delay for calculation at that point. In other words, this function is frontheavy.

	//Tokenize Data string to pull each piece of data out EX string: Rx: -2.43 Ry: 1.45 Rz: .89
	char* context = NULL;
	char* context2 = NULL;
	char* IMUDataCopy = _strdup(IMU.Data.c_str());
	//First token is the data type & value
	char* token = strtok_s(IMUDataCopy, " ", &context);
	while (token != NULL)
	{
		char* tokenCopy = _strdup(token);

		char* type = strtok_s(tokenCopy, ":", &context2);
		long double sValue = stold(strtok_s(NULL, ":", &context2)); //TODO: may need to place this in while to avoid reassignment
		double newWeight = IMU.Weight;

		// ALSO: make it just overwrite for the requested pos, not update. This is based on immediate position so do that.
		//Run once for each possible position
		while (loopCount < 4)
		{
			long double value = sValue;
			//Weight value by position
			if (position == "F")//Front
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value;
						break;
					case 1://Calculating port opt
						value = value * .5;
						newWeight = .5;
						break;
					case 2://Calculating rear opt
						value = value * .25;
						newWeight = .75;
						break;
					case 3://Calculating starboard opt
						value = value * .5;
						newWeight = .5;
						break;
				}
			}
			else if (position == "P")//Port (left)
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * .5;
						newWeight = .5;
						break;
					case 1://Calculating port opt
						value = value;
						break;
					case 2://Calculating rear opt
						value = value * .5;
						newWeight = .5;
						break;
					case 3://Calculating starboard opt
						value = value * .25;
						newWeight = .75;
						break;
				}
			}
			else if (position == "R")//Rear
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * .25;
						newWeight = .75;
						break;
					case 1://Calculating port opt
						value = value * .5;
						newWeight = .5;
						break;
					case 2://Calculating rear opt
						value = value;
						break;
					case 3://Calculating starboard opt
						value = value * .5;
						newWeight = .5;
						break;
				}
			}
			else if (position == "S")//Starboard (right)
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * .5;
						newWeight = .5;
						break;
					case 1://Calculating port opt
						value = value * .25;
						newWeight = .75;
						break;
					case 2://Calculating rear opt
						value = value * .5;
						newWeight = .5;
						break;
					case 3://Calculating starboard opt
						value = value;
						break;
				}
			}
			//Check for existing entry of same type of data
			auto it = std::find_if(positionOpt.begin(), positionOpt.end(),
				[type](std::tuple<string, string, long double>& tpl) {return std::get<1>(tpl) == type; });
			//If it does exist, use a weighted average after biasing the current value by position
			if (it != positionOpt.end())
			{
				//NOTE: ASSUMPTION - the data in the third spot of the tuple is the old value, it is multiplied by a weight of .99 as I expect
				// the stored to be only 99% accurate. If this guess is wrong, change the .99 to allow for more
				// or less error. If you want it to be even more accurate, we can brainstorm a struct to use but it 
				// would cause data bloat.

				//Weight stays stable - aka no *.5 or such above - make the .99 below change to be the rest of the weight - aka if 
				// mult new value by .5, the .99 would also become .5, if mult new value by .25 then the .99 would become .75
				long double newValue = ((std::get<2>(*it) * newWeight) + (value));
				std::get<2>(*it) = newValue;
			}
			//If it does not exist, place the tuple into the vector
			else
				positionOpt.push_back(std::make_tuple(IMU.IMUName, type, value));

			loopCount++;
		}

		loopCount = 0;

		//next data type/value pair
		token = strtok_s(NULL, " ", &context);
	}
	free(IMUDataCopy);

	//store the data by positionOpt-POSSTRING
	string dbIDString = "positionOpt";
	dbIDString += "-" + position;
	storeData(dbIDString);
}

IMUBuilder::internalMeasurementUnit CondenseData::accessData(int ID)
{
	IMUBuilder imuBuilder;
	IMUBuilder::internalMeasurementUnit IMU;
	sqlite3_stmt* stmt;
	string cmd = "SELECT * FROM IMUTable WHERE IMUID=?;";
	int rc = sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
	if(rc != 0)
		printf("Error preparing statement: %s\n", sqlite3_errmsg(imuBuilder.db));
	sqlite3_bind_int(stmt, 1, ID);
	//execute created stmt
	rc = sqlite3_step(stmt);
	//No more rows available
	if (rc == 101)
	{
		IMU.Weight = NULL;
	}
	else
	{
		//Create IMU from info in db
		IMU.IMUID = sqlite3_column_int(stmt, 0);
		IMU.IMUType = (const char*)sqlite3_column_text(stmt, 1);
		IMU.IMUName = (const char*)sqlite3_column_text(stmt, 2);
		IMU.Weight = sqlite3_column_double(stmt, 3);
		IMU.Data = (const char*)sqlite3_column_text(stmt, 4);
	}
	//clean up stmt
	sqlite3_finalize(stmt);
	return IMU;
}

void CondenseData::storeData(string objectName)
{
	IMUBuilder imuBuilder;

	if (objectName == "accuracyOpt")
	{
		string cmd = "UPDATE OptTable SET OptForAccuracy = ? WHERE ID = 0;";

		string accuracyString = "";

		//Iterate through accuracyOpt and create a string representation of the optimized data
		for (auto it = begin(accuracyOpt); it != end(accuracyOpt); ++it)
			accuracyString += it->first + ": " + to_string(it->second);

		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
		sqlite3_bind_text(stmt, 1, accuracyString.c_str(), -1, SQLITE_STATIC);
		//execute created stmt
		sqlite3_step(stmt);
		//clean up stmt
		sqlite3_finalize(stmt);
	}
	else
	{
		string cmd;
		if (objectName == "positionOpt-F")
			cmd = "UPDATE OptTable SET OptForPositionF = ? WHERE ID = 0;";

		if (objectName == "positionOpt-P")
			cmd = "UPDATE OptTable SET OptForPositionP = ? WHERE ID = 0;";

		if (objectName == "positionOpt-R")
			cmd = "UPDATE OptTable SET OptForPositionR = ? WHERE ID = 0;";

		if (objectName == "positionOpt-S")
			cmd = "UPDATE OptTable SET OptForPositionS = ? WHERE ID = 0;";

		string positionString = "";

		//Iterate through positionOpt and create a string representation of the optimized data
		for (auto it = begin(positionOpt); it != end(positionOpt); ++it)
			positionString += std::get<1>(*it) + ": " + to_string(std::get<2>(*it));

		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
		sqlite3_bind_text(stmt, 1, positionString.c_str(), -1, SQLITE_STATIC);
		//execute created stmt
		sqlite3_step(stmt);
		//clean up stmt
		sqlite3_finalize(stmt);
	}

	//NOTE: Debug print
	if (cdDebugMode > 0)
	{
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

void CondenseData::basicSortIMUs(IMUBuilder::internalMeasurementUnit IMU)
{
	//Get position (F, R, S, P) off of end of IMUName string
	string position = IMU.IMUName.substr(IMU.IMUName.size() - 1, IMU.IMUName.size());//TODO: something not getting decoded here -.-
	//Assemble pair and push onto list
	pair<IMUBuilder::internalMeasurementUnit, string> IMUandPos = std::make_pair(IMU, position);
	IMUList.push_back(IMUandPos);
	//Sort list by Weight, left with higher, right with lower
	IMUList.sort([](auto& left, auto& right) {return left.first.Weight > right.first.Weight; });
}

CondenseData::~CondenseData()
{
	
}