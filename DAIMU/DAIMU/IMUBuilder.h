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
		void IMUPopulator(string IMUs);
		~IMUBuilder();

	private:
		internalMeasurementUnit getIMU(int IMUID);
		void setIMU(int IMUID, string IMUType, string IMUName, float Weight);
		string getIMUData(int IMUID);
		void setIMUData(int IMUID, string Data);
		int verifyData(string Data);
		void openDB();
};

