#pragma once 

#include <string.h>
#include <vector>
#include <string>

using namespace std;

class AnalysisMessage
{
public:
	AnalysisMessage(char *m, const char *sp);

	void spliteMessage(vector<char *> &resoult);
private:
	char *message;
	const char *spchar;
};
