#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <string>
#include <cstdint>
#include <vector>
#include <sqlite3.h>

using namespace std;

class IMUBuilder
{
	public:
		int ibDebugMode = 0; //TODO: this
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
		~IMUBuilder();

	private:
		internalMeasurementUnit getIMU(int IMUID);
		void setIMU(int IMUID, string IMUType, string IMUName, float Weight);
		string getIMUData(int IMUID);
		void setIMUData(int IMUID, string Data);
		int verifyData(string Data);
		
};

