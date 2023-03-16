#pragma once
#include <utility>
#include <list>
#include <tuple>
#include "IMUBuilder.h"

class CondenseData
{
	public:
		//IMUName, DataType, data
		vector<std::tuple<string, string, long double>> positionOpt;
		//DataType, data
		vector<std::pair<string, long double>> accuracyOpt;

		std::vector <vector<std::pair<string, long double>>> accuracyOptList;
		std::list <std::pair<IMUBuilder::internalMeasurementUnit, string>> IMUList;

		int optCount = 0;

		CondenseData();
		CondenseData(int debugMode);
		void condenser();
		~CondenseData();

	private:
		int cdDebugMode = 0;

		void adjustOpt(int optCount);
		void optimizeOnPosition(IMUBuilder::internalMeasurementUnit IMU, string position);//TODO: Takes normal weight and if opposite side of desired div by 4, if not desired side but not opp. side, div by 2. Then opt
		void optimizeOverAll(IMUBuilder::internalMeasurementUnit IMU);
		IMUBuilder::internalMeasurementUnit accessData(int ID);
		void storeData(string objectName);
		void basicSortIMUs(IMUBuilder::internalMeasurementUnit IMU);
};