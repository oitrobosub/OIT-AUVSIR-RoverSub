#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <cstdint>
#include <vector>
#include <sqlite3.h>
#include <list>

using namespace std;

class IMUBuilder
{
	public:
		struct internalMeasurementUnit 
		{
			int IMUID;
			string IMUType;
			string IMUName;
			float Weight;
			string Data;
		};
		vector<internalMeasurementUnit> IMUVector;
		sqlite3* db;

		IMUBuilder();
		IMUBuilder(int debugMode);
		void IMUPopulator(string IMUs);
		void openDB();
		void clearTables();
		~IMUBuilder();

	private:
		int ibDebugMode = 0;

		internalMeasurementUnit getIMU(int IMUID);
		void setIMU(int IMUID, string IMUType, string IMUName, float Weight);
		string getIMUData(int IMUID);
		void setIMUData(int IMUID, string Data);
		int verifyData(string Data, int IMUID);
		
};

