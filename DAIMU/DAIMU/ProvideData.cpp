#include "ProvideData.h"

ProvideData::ProvideData()
{
}

string ProvideData::getOptimizedData(string dataType)
{
	string colName = "OptForAccuracy";
	if (dataType == "F")
		colName = "OptForPositionF";
	else if (dataType == "R")
		colName = "OptForPositionR";
	else if (dataType == "S")
		colName = "OptForPositionS";
	else if (dataType == "P")
		colName = "OptForPositionP";
	string desiredData = accessData(colName);
	return desiredData;
}

string ProvideData::accessData(string colName)
{
	IMUBuilder imuBuilder;

	sqlite3_stmt* stmt;
	//how do I use this to get the value in the X column, first row from OptTable
	string cmd = "SELECT " + colName + " FROM OptTable LIMIT 1;";
	int rc = sqlite3_prepare_v2(imuBuilder.db, cmd.c_str(), -1, &stmt, NULL);
	if (rc != 0)
		printf("Error preparing statement: %s\n", sqlite3_errmsg(imuBuilder.db));
	rc = sqlite3_step(stmt);
	string dataString = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
	sqlite3_finalize(stmt);
	return dataString;
}

ProvideData::~ProvideData()
{
}