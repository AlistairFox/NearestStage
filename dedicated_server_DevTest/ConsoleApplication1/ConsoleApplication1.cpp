#include <iostream>
#include "resource.h"
#include "header.h"
#include <string>

void main()
{
	main_class().arg1();
}

void main_class::starter()
{
	std::cout << "input password";
	std::cin >> incode;
	if (incode == 1)
	{
		starter_c = start + start2 + start5 + start6;

		system(starter_c.c_str());
	}
	else
	{
		std::cout << "poshel nahui";
		std::cin >> incode;
	}
	std::cout << starter_c;
}

void main_class::arg1()
{
		start = "start ..\\_dedicated_\\xrEngine.exe -save_old_logs -skip_cdb_cache_crc32_check ";
		arg2();
}

void main_class::arg2()
{
		arg3();
}

void main_class::arg3()
{
		start5 = "-fsltx ..\\fsgame_dedicated.ltx ";
		arg4();
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
	start6 = in_start_6;
	starter();
}
