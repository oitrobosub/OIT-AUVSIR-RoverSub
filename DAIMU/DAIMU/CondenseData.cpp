#include "CondenseData.h"

CondenseData::CondenseData()
{
	int ID = 0;
	IMUBuilder::internalMeasurementUnit IMU = accessData(ID);
	//While IMU struct is populated
	while (IMU.Weight != NULL)
	{
		//Form and sort list by weight
		basicSortIMUs(IMU);
		//access next IMU
		ID++;
		IMU = accessData(ID);
	}
	//optimize the IMUs in the list
	for (auto it = IMUList.begin(); it != IMUList.end(); ++it)
	{
		IMU = it->first;
		string position = it->second;
		optimizeOverAll(IMU);
		optimizeOnPosition(IMU, position);
	}
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
		
		char* type = strtok_s(tokenCopy, ": ", &context2);
		long double value = stold(strtok_s(NULL, ": ", &context2));
		//Weight value accordingly
		value = value * IMU.Weight;
		
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
			
			//value was weighted above not necessacary in numerator
			long double newValue = (it->second * .99) + (value) / (.99 + IMU.Weight);
			it->second = newValue;
		}
		//If it does not exist, place the pair into the vector
		else
			accuracyOpt.push_back({ type, value });
		
		//next data type/value pair
		token = strtok_s(NULL, " ", &context);
	}
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

		char* type = strtok_s(tokenCopy, ": ", &context2);
		long double value = stold(strtok_s(NULL, ": ", &context2));
		//Weight value according to basic weight
		value = value * IMU.Weight;

		//Run once for each possible position
		while (loopCount < 4)
		{
			//Weight value by position
			if (position == "F")//Front
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * 1;
						break;
					case 1://Calculating port opt
						value = value * .5;
						break;
					case 2://Calculating rear opt
						value = value * .25;
						break;
					case 3://Calculating starboard opt
						value = value * .5;
						break;
				}
			}
			else if (position == "P")//Port (left)
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * .5;
						break;
					case 1://Calculating port opt
						value = value * 1;
						break;
					case 2://Calculating rear opt
						value = value * .5;
						break;
					case 3://Calculating starboard opt
						value = value * .25;
						break;
				}
			}
			else if (position == "R")//Rear
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * .25;
						break;
					case 1://Calculating port opt
						value = value * .5;
						break;
					case 2://Calculating rear opt
						value = value * 1;
						break;
					case 3://Calculating starboard opt
						value = value * .5;
						break;
				}
			}
			else if (position == "S")//Starboard (right)
			{
				switch (loopCount)
				{
					case 0://Calculating front opt
						value = value * .5;
						break;
					case 1://Calculating port opt
						value = value * .25;
						break;
					case 2://Calculating rear opt
						value = value * .5;
						break;
					case 3://Calculating starboard opt
						value = value * 1;
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

				//value was weighted above not necessacary in numerator
				long double newValue = (std::get<2>(*it) * .99) + (value) / (.99 + IMU.Weight);
				std::get<2>(*it) = newValue;
			}
			//If it does not exist, place the tuple into the vector
			else
				positionOpt.push_back(std::make_tuple(IMU.IMUName, type, value));

			//store the data by positionOpt-POSSTRING
			string dbIDString = "positionOpt";
			dbIDString += "-" + position;
			storeData(dbIDString);
			loopCount++;
		}

		loopCount = 0;

		//next data type/value pair
		token = strtok_s(NULL, " ", &context);
	}
	free(IMUDataCopy);
}

IMUBuilder::internalMeasurementUnit CondenseData::accessData(int ID)
{
	IMUBuilder imuBuilder;
	sqlite3_stmt* stmt;
	string cmd = "SELECT FROM IMUTable WHERE IMUID=?;";
	sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
	sqlite3_bind_int(stmt, 0, ID);
	//execute created stmt
	sqlite3_step(stmt);
	//Create IMU from info in db
	IMUBuilder::internalMeasurementUnit IMU;
	IMU.IMUID = sqlite3_column_int(stmt, 0);
	IMU.IMUType = (const char*)sqlite3_column_text(stmt, 1);
	IMU.IMUName = (const char*)sqlite3_column_text(stmt, 2);
	IMU.Weight = sqlite3_column_double(stmt, 3);
	IMU.Data = (const char*)sqlite3_column_text(stmt, 4);
	//clean up stmt
	sqlite3_finalize(stmt);
	return IMU;
}

void CondenseData::storeData(string objectName)
{
	IMUBuilder imuBuilder;
	//rc = return code -> this is common sqlite3 naming convention
	int rc = 1;
	while (rc != 0)
	{
		string tableCreation = "CREATE TABLE IF NOT EXISTS OptTable (OptForAccuracy TEXT, OptForPositionF TEXT, OptForPositionR TEXT, OptForPositionS TEXT, OptForPositionP TEXT);";
		rc = sqlite3_exec(imuBuilder.db, tableCreation.c_str(), NULL, NULL, NULL);
	}
	//TODO: switch case to determine which each is optmized for?
	if (objectName == "accuracyOpt")
	{
		string cmd = "UPDATE OptTable SET OptForAccuracy = ?;";

		string accuracyString = "";

		//Iterate through accuracyOpt and create a string representation of the optimized data
		for (auto it = begin(accuracyOpt); it != end(accuracyOpt); ++it)
			accuracyString += it->first + ": " + to_string(it->second) + ", ";

		sqlite3_stmt* stmt;
		sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
		sqlite3_bind_text(stmt, 1, accuracyString, -1, SQLITE_STATIC);
		//execute created stmt
		sqlite3_step(stmt);
		//clean up stmt
		sqlite3_finalize(stmt);

		return;
	}
	else
	{
		if (objectName == "positionOpt-F")

		if (objectName == "positionOpt-P")

		if (objectName == "positionOpt-R")

		if (objectName == "positionOpt-S")

	}
}

void CondenseData::basicSortIMUs(IMUBuilder::internalMeasurementUnit IMU)
{
	//Get position (F, R, S, P) off of end of IMUName string
	string position = IMU.IMUName.substr(0, IMU.IMUName.size() - 1);
	printf("position: %s", position);
	//Assemble pair and push onto list
	pair<IMUBuilder::internalMeasurementUnit, string> IMUandPos = std::make_pair(IMU, position);
	IMUList.push_back(IMUandPos);
	//Sort list by Weight, left with higher, right with lower
	IMUList.sort([](auto& left, auto& right) {return left.first.Weight > right.first.Weight});
}

CondenseData::~CondenseData()
{
	//TODO: This
}