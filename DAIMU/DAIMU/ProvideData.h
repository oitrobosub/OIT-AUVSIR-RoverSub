#pragma once
#include "CondenseData.h"
class ProvideData
{
	public:
		string getOptimizedData(string dataType);

		ProvideData();
		~ProvideData();

	private:
		string accessData(string colName);
};

