#pragma once
#include "IMUBuilder.h"

class CondenseData
{
	public:
		string positionOpt;
		string accuracyOpt;

		CondenseData();
		~CondenseData();

	private:
		void optimizeOnPosition();
		void optimizeOverAll();
		string accessData();
		void storeData();
		void sortIMUs();
};

