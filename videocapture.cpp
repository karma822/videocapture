
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
#include <windows.h>
#include <string.h> 
#include <commdlg.h>
#include <WinUser.h>
#include <vector>

using namespace cv;
using namespace std;

#define NUMBUF 30
string FILENAME;

string getDlgFileName()
{
	OPENFILENAME ofn;
	char szFile[260] = "\0";       // buffer for file name
	HWND hwnd = NULL;              // owner window

	GetWindow(hwnd, GW_OWNER);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All\0*.*\0mp4\0*.MP4\0avi\0*.AVI\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) != TRUE)
		return string();

	string str(szFile);
	int len = str.size();
	int index = len;
	int backslashindex = len;
	int dotindex = len;
	while (index > 0) {
		if (str[index - 1] == '\\') {
			backslashindex = index;
			break;
		}
		else if (str[index - 1] == '.' && dotindex == len ) {
			dotindex = index;
		}
		index--;
	}
	
	FILENAME = str.substr(backslashindex, dotindex - backslashindex - 1 );

	cout << FILENAME << endl;
	return ofn.lpstrFile;
}

int imagenumber = 0;
VideoCapture openFile()
{
	string file = getDlgFileName();
	if (file.empty())
		return -1;

	VideoCapture cap(file); // open the default camera
	if (!cap.isOpened())  // check if we succeeded
		return -1;
	imagenumber = 0;
	return cap;
}

int prevstartindex = 0;
int prevnum = 0;
int previndex = 0;

void setPrevFrameBuffer(vector<Mat> &prev, Mat frame) {
	prev[(prevnum + prevstartindex) % NUMBUF] = frame.clone();
	if (prevnum >= NUMBUF) {
		prevstartindex = (prevstartindex + 1) % NUMBUF;
	}
	else {
		prevnum++;
		previndex++;
	}
}

int storeFrame(Mat frame) {
	string filename = FILENAME + "-" + to_string(imagenumber++) + ".jpg";
	return imwrite(filename.c_str() , frame);
}

int main(int, char**)
{
	VideoCapture cap = openFile();
	namedWindow("frame", 1);
	resizeWindow("frame", 420, 240);

	Mat frame;
	cap >> frame; // get a new frame from camera
	imshow("frame", frame);

	bool playing = false;
	int input = waitKeyEx(0);

	vector<Mat> prev = vector<Mat>(NUMBUF);
	int frameindex = 1;
	double framecount = cap.get(CAP_PROP_FRAME_COUNT);
	//quit at q or Q
	//play and pause sapcebar
	//next frame n
	//previous frame p
	//capture c
	//open new file o
	while (input != 'q' && input != 'Q' && (( !playing && input >= 0) || playing )) {
		if (input == 32) {
			playing = !playing;
			cout << (playing ? "play" : "pause") << endl;
		}

		if (input == 'o') {
			cout << "open new file" << endl;
			cap.release();
			cap = openFile();
			cap >> frame;
			imshow("frame", frame);
			prevstartindex = 0;
			prevnum = 0;
			previndex = 0;
			frameindex = 1;
			input = waitKey(0);
			continue;
		}

		if (input == 'c') {
			cout << "capture frame: " + FILENAME + "-" + to_string(imagenumber) + ".jpg" << endl;
			storeFrame(frame);
			input = waitKey(0);
			continue;
		}

		if (playing) {
			if (previndex < prevnum) {
				frame = prev[(previndex++ + prevstartindex) % 30].clone();
				frameindex++;
			} {
				setPrevFrameBuffer(prev, frame);
				cap >> frame;
				frameindex++;
				while (frame.empty()) {
					if (frameindex < framecount) {
						frameindex++;
						cap >> frame;
						continue;
					}
					MessageBox(NULL, "End of file", "EOL", MB_OK);
					playing = false;
					input = waitKey(0);
					break;
				}
			}
			imshow("frame", frame);
			input = waitKeyEx(30);
		}
		else {
			if (input == 'n') {
				if (previndex < prevnum) {
					frame = prev[(previndex++ + prevstartindex) % 30].clone();
				}
				else {
					setPrevFrameBuffer(prev, frame);
					cap >> frame;
					frameindex++;
					while (frame.empty()) {
						if (frameindex >= framecount ) {
							MessageBox(NULL, "End of file", "EOL", MB_OK);
							playing = false;
							input = waitKey(0);
							break;
						}
						cap >> frame;
						frameindex++;
					}
				}
				imshow("frame", frame);
			}
			else if (input == 'p') {
				if (previndex > 0) {
					previndex--;
					frameindex--;
					frame = prev[(previndex + prevstartindex) % NUMBUF].clone();
				}
				else if (frameindex == 0) {
					MessageBox(NULL, "End of file", "EOL", MB_OK);
					input = waitKey(0);
					continue;
				}
				else {
					frameindex--;
					cap.set(CAP_PROP_POS_FRAMES, frameindex);
					cap >> frame;
					prevstartindex = 0;
					prevnum = 0;
					previndex = 0;
				}
				imshow("frame", frame);
			}
			cout << frameindex << endl;
			input = waitKeyEx(0);
		}
	}
	return 0;
}