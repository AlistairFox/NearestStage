#include <iostream>
#include "resource.h"
#include "header.h"
#include <string>

int main()
{
	main_class().arg4();
}

void main_class::starter()
{
	starter_c = "start ..\\_dedicated_\\xrEngine.exe -save_old_logs -skip_cdb_cache_crc32_check - fucknikitakalinovsky5785412386westdfa23 - i -fsltx ..\\fsgame_dedicated.ltx " + in_start_6;

	system(starter_c.c_str());
}

void main_class::arg4()
{
	
	std::cout << "Enter the level:( 1 nearestmap, 2 nearest_test)";
	std::cin >> start3;
	if (start3 == 1)
	{
		start4 = " server(nearestmap";
	}
	else if (start3 == 2)
		start4 = " server(nearest_test";


	in_start_6 = "-start" + start4 + dr;
	starter();
}
