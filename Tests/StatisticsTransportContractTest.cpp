#include "Transport/HorizonStatisticsTransportContract.h"
#include <cassert>
#include <iostream>
#include <limits>

int main()
{
	using namespace HorizonTransportContract;
	int64_t Parsed = 0;
	assert(ParseStatisticsInteger("9007199254740991", Parsed) && Parsed == StatisticsMaxValue);
	assert(ParseStatisticsInteger("-9007199254740991", Parsed) && Parsed == -StatisticsMaxValue);
	for (const auto* Invalid : {"9007199254740991.1", "1.0000000000000001", "9007199254740992", "01", "1e3", "1.0", "", "-", "+1", "1\n"}) assert(!ParseStatisticsInteger(Invalid, Parsed));
	assert(IsStatisticsOperationId("1792000000000:01234567-89ab-4cde-8123-0123456789ab"));
	assert(!IsStatisticsOperationId("1792000000000:01234567-89ab-4cde-8123-0123456789ab\n"));
	assert(!IsStatisticsOperationId("1792000000000:01234567-89ab-4cde-8123-0123456789ag"));
	assert(!IsStatisticsOperationId("01234567-89ab-4cde-8123-0123456789ab"));
	assert(IsStatisticKey("kills") && IsStatisticKey("best-score.v2") && IsStatisticKey(std::string(24, 'a')));
	assert(!IsStatisticKey("_kills") && !IsStatisticKey("Kills") && !IsStatisticKey("kills\n") && !IsStatisticKey(std::string(25, 'a')));
	assert(IsStatisticsNumber(9007199254740991.0) && IsStatisticsNumber(-9007199254740991.0));
	assert(!IsStatisticsNumber(9007199254740992.0) && !IsStatisticsNumber(.5));
	assert(!IsStatisticsNumber(std::numeric_limits<double>::infinity()) && !IsStatisticsNumber(std::numeric_limits<double>::quiet_NaN()));
	assert(IsStatisticsBatch({{"kills", StatisticsMaxValue}, {"best-score", -StatisticsMaxValue}}));
	assert(!IsStatisticsBatch({}) && !IsStatisticsBatch({{"kills", 1}, {"kills", 2}}));
	assert(!IsStatisticsBatch({{"kills", StatisticsMaxValue + 1}}));
	std::vector<std::pair<std::string,int64_t>> Batch;
	for (int Index = 0; Index < 201; ++Index) Batch.emplace_back("k" + std::to_string(Index), Index);
	assert(IsStatisticsBatch(Batch)); Batch.emplace_back("k200", 1); assert(!IsStatisticsBatch(Batch));
	std::cout << "Unreal statistics safe integer, key and explicit operation contract passed\n";
}
