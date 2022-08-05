#include <iostream>

#include <opencv2/opencv.hpp>
using namespace  std;
using namespace  cv;

int main(int argc,char ** argv)
{
	string in;
	CommandLineParser parser(argc,argv,"{@input|corridor.jpg|input image}{help h||show help message");
	if (parser.has("help"))
	{
		parser.printMessage();
		return 0;
	}
	//in=samples::findFile(parser.get<string>("@input"));

}