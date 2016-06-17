#include <iostream>
#include <thread>
#include <chrono>
#ifdef USE_THREADPOOL
	#include "NdThreadPool.h"
#endif // USE_THREADPOOL
#include "NdHttpClient.h"
#include "NdTask.h"



using namespace std;


template<class F, class... Args>
void FuncWrapper(int count, F&& f, Args&&... args)
{
	float avg_time = 0.0f;
	for (int i = 0; i < count; i++)
	{
		std::cout << "**********************************************************\n";
		std::cout << "NetDragon Auto Auto Count  " << i <<  " Testing....\n";
		auto begin = std::chrono::high_resolution_clock::now();
		auto task  = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		task();
		auto end   = std::chrono::high_resolution_clock::now();
		auto tmp   = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
		avg_time   += tmp * 1.0f;
		std::cout << "Single Execution Time  " << tmp << " ms.\n";
	}
	std::printf("Average Execution Time :%0.2f ms\n", avg_time * 1.0f / count * 1.0f);
	NdHttpClient::GlobalUnInit();
}

#ifdef USE_THREADPOOL

int main(int argc, char* argv [])
{
	NdHttpClient::GlobalInit();
	FuncWrapper(1,
		[](int pool_size) {
			std::vector<NdUserInfo> userArray;
			bool result = NdTask::InitUsers("/home/orangepi/Apps/config.json", &userArray);
			if (!result || userArray.size() == 0)
			{
				std::cout << "Failed To Get Users Infoes" << std::endl;
				return;
			}
			NdThreadPool pool(pool_size);
			std::vector<std::future<bool> > results;
	
			for (size_t i = 0; i < userArray.size(); i++)
			{
				results.emplace_back(
					pool.enqueue(NdTask(), std::ref(userArray[i]))
				);
			}
			
			for (auto && result : results)
			{
				result.get();
			}
		}, 8);
	NdHttpClient::GlobalUnInit();
	return 0;
}

#else 
int main(int argc, char* argv [])
{
	std::cout << "No Implement Using Other Method" << std::endl;
	return 0;
}


#endif // USE_THREADPOOL