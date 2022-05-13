// it's snippet cut from my stellaris module, friend pushed me to release this early so it's a bit unfinished
// you will need to do a bit of job yourself to get this up and running (sorting results not included as it uses few headers but it should be easy to do with macros or by C++ code itself)

double totalRunTime = 0.0f;

std::mutex triggerMutex;
std::map<std::string*, std::pair<int, double>> logMap;


int64 TriggerEvaluate_Detour(void* thisptr, void* uh)
{
	std::string* myString = reinterpret_cast<std::string*>(reinterpret_cast<unsigned long long>(thisptr) + 0x40ull);
//	logNames[myString] = *myString;

	auto t1 = std::chrono::high_resolution_clock::now();
	int64 returnVal = CDetourHandler_OriginalFunc(TriggerEvaluate_Detour)(thisptr, uh);
	auto t2 = std::chrono::high_resolution_clock::now();


	if (!myString->empty())
	{
		if (triggerMutex.try_lock())
		{
			std::chrono::duration<double, std::milli> ms_double = t2 - t1;
			
			logMap[myString].first += 1;
			logMap[myString].second += ms_double.count();
			totalRunTime += ms_double.count();

			triggerMutex.unlock();
		}
	}
	return returnVal;
}

void HandleDailyUpdate_Detour(void* thisptr, void* edxx)
{
	int m_CurrentMonthDay = *reinterpret_cast<int*>(reinterpret_cast<unsigned long long>(thisptr) + 0x12Cull);
	//	0x12C current day of month

	if (m_CurrentMonthDay == 26)
	{
		std::ofstream myfile;	// shouldn't probably open file each time 26th hits cause its inefficient but heck
		myfile.open("log_data.txt", std::ios_base::app);
		myfile << "--Monthly--\n";
		myfile << "--totalRunTime: ";
		myfile << std::to_string(totalRunTime);
		myfile << "\nAmount of calls:\n";
		for (auto it = logMap.begin(); it != logMap.end(); it++)
		{
			std::string report;
			report += std::to_string(it->second.first);
			report += " calls";
			report += " - time elapsed here ";
			report += std::to_string(it->second.second);
			report += " # ";
			report += *it->first;
			report += " # ";

			report += "\n";

			myfile << report;
		}
		myfile << "\nAmount of time:\n";
		for (auto it = logMap.begin(); it != logMap.end(); it++)
		{
			std::string report;
			report += std::to_string(it->second.second);
			report += " time elapsed here - ";
			report += std::to_string(it->second.first);
			report += " calls";
			report += " # ";
			report += *it->first;
			report += " # ";

			report += "\n";

			myfile << report;
		}
		myfile << "--Monthly--\n";
		myfile.close();

	}
	CDetourHandler_OriginalFunc(HandleDailyUpdate_Detour)(thisptr, edxx);
}

void Init()
{
	MessageBox(NULL, "Startin.!", "StellarisPerf", NULL);

	// v3.4 bytes
	void *sig_ptrDailyUpdate = CSignature::QuickEval("stellaris.exe", "48 89 4C 24 08 55 53 56 57 41 54 41 55 41 56 41 57 B8 68 15 00 00");
	if (sig_ptrDailyUpdate == NULL)
	{
		MessageBox(NULL, "Failed to locate signature.", "StellarisPerf", NULL);
		return;
	}

	void *sig_ptrTriggerEvaluate = CSignature::QuickEval("stellaris.exe", "48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 50 FF FF FF 48 81 EC B0 01 00 00 4C 8B E2");
	if (sig_ptrTriggerEvaluate == NULL)
	{
		MessageBox(NULL, "Failed to locate signature.", "StellarisPerf", NULL);
		return;
	}
	
	CDetourHandler::Make_Instance(sig_ptrDailyUpdate, HandleDailyUpdate_Detour);
	CDetourHandler::Make_Instance(sig_ptrTriggerEvaluate, TriggerEvaluate_Detour);
	return;
}