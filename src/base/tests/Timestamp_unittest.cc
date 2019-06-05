/*
 * @Author: py.wang 
 * @Date: 2019-06-05 16:18:50 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-05 16:46:10
 */

#include "src/base/Timestamp.h"

#include <stdio.h>
#include <vector>

using slack::Timestamp;
using std::vector;

void passByConstReference(const Timestamp &x)
{
    printf("%s-%s\n", x.toString().c_str(), x.toFormattedString().c_str());

}

void passByValue(Timestamp x)
{
    printf("%s-%s\n", x.toString().c_str(), x.toFormattedString().c_str());
}

// 基准测试
void benchmark()
{
    const int kNumber = 1000 * 1000;
    vector<Timestamp> stamps;
    // 预先分配空间,size依旧为0，capatity=kNumber(因为已知大小)
    stamps.reserve(kNumber);
    for (int i = 0; i < kNumber; ++i)
    {
        stamps.push_back(Timestamp::now());
    }
    printf("%s-%s\n", stamps.front().toString().c_str(), stamps.front().toFormattedString().c_str());
    printf("%s-%s\n", stamps.back().toString().c_str(), stamps.back().toFormattedString().c_str());
    printf("%f\n", timeDifference(stamps.back(), stamps.front()));

    int increments[100] = {0};
    int64_t start = stamps.front().microSecondsSinceEpoch();
    for (int i = 1; i < kNumber; ++i)
    {
        int64_t next = stamps[i].microSecondsSinceEpoch();
        int64_t inc = next - start;
        start = next;
        if (inc < 0)
        {
            printf("reverse\n");
        }
        else if (inc < 100)
        {
            ++increments[inc];
        }
        else 
        {
            printf("big gap %d\n", static_cast<int>(inc));
        }
    }

    for (int i = 0; i < 100; ++i)
    {
        printf("%2d: %d\n", i, increments[i]);
    }
}

int main()
{
    Timestamp now(Timestamp::now());
    printf("%s-%s\n", now.toString().c_str(), now.toFormattedString().c_str());
    passByConstReference(now);
    passByValue(now);
    benchmark();
}