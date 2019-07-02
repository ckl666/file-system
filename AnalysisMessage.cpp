#include "AnalysisMessage.h"

#define MAX_LENGTH 10

AnalysisMessage::AnalysisMessage(char *m, const char *sp)
	:message(m),
	spchar(sp)
{}

//split message
void AnalysisMessage::spliteMessage(vector<char *> &resoult)
{
	char *save = nullptr;
	char *str = strtok_r(message, spchar, &save);
	int i = 0;
	while(str != nullptr)
	{
		resoult.push_back(str);
		str = strtok_r(NULL, spchar, &save);
	}
}
