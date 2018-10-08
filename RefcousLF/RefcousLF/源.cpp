//#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <cstdio>

using namespace std;
using namespace cv;

const int viwenum = 5;
const char filename[128] = "./mv/img-%02d.png";
vector<vector<Mat>> loadMV(const char *filename);
Mat MakeColorFocused(vector<vector<Mat>> LF, double disparity);
Mat MakeFocalImage(vector<Mat> LF, double disparity);
void kasankirihari(Mat src, int src_x, int src_y, Mat dst, int dst_x, int dst_y, int smallarea_x, int smallarea_y);
void ShiftandAddImage_double(Mat src, Mat dst, double shift_x, double shift_y);
void ShiftandAddImage(Mat src, Mat dst, int shift_x, int shift_y);


int main() {
	cout << "Refocused Light Field Images" << endl;
	//double disparity = 0.0;

	double disparity, disparity_past = -9999;

	vector<vector<Mat>> LF = loadMV(filename);

	int track_HALF = (int)(50);
	int trackval = track_HALF;
	char focusedwindowname[] = "focused_image";
	namedWindow(focusedwindowname);

	moveWindow(focusedwindowname, 300, 300);

	createTrackbar("DISPARITY", focusedwindowname, &trackval, 100);
	//Mat result = MakeColorFocused(LF, disparity);
	bool flag_manualdisparity = false;
	while (true) {
		if (flag_manualdisparity != true) {
			disparity = (double)(trackval - track_HALF) / ((double)track_HALF);
		}
		if (disparity != disparity_past) {
			cout << "disparity: " << disparity << endl;
			Mat result = MakeColorFocused(LF, disparity);
			cout << "calculate completed." << endl;
			imshow(focusedwindowname, result);
			waitKey(0);

		}
		disparity_past = disparity;
	}
	//imwrite("dd_color.png", result);
	return 0;
}

Mat MakeColorFocused(vector<vector<Mat>> LF, double disparity) {
	
	Mat res_r = MakeFocalImage(LF[0], disparity);
	Mat res_g = MakeFocalImage(LF[1], disparity);
	Mat res_b = MakeFocalImage(LF[2], disparity);


	vector<Mat> color_set;
	color_set.push_back(res_r);
	color_set.push_back(res_g);
	color_set.push_back(res_b);

	Mat result;
	merge(color_set, result);
	return result;
}
vector<vector<Mat>> loadMV(const char *filename) {
	vector<vector<Mat>> LF(3, vector<Mat> (viwenum*viwenum));


	for (int i = 0; i < viwenum; i++) {
		for (int j = 0; j < viwenum; j++) {
			char cur_fullpath[128];
			sprintf_s(cur_fullpath, filename, i*viwenum + j + 1);
			cout << cur_fullpath << endl;
			//Mat temp = imread(cur_fullpath, 0);
			Mat temp = imread(cur_fullpath);
			vector<Mat> rgb;
			split(temp, rgb);
			rgb[0].convertTo(rgb[0], CV_64FC1);
			rgb[1].convertTo(rgb[1], CV_64FC1);
			rgb[2].convertTo(rgb[2], CV_64FC1);

			LF[0][i*viwenum + j] = rgb[0];
			LF[1][i*viwenum + j] = rgb[1];
			LF[2][i*viwenum + j] = rgb[2];
		}
	}
	return LF;
}

Mat MakeFocalImage(vector<Mat> LF, double disparity) {
	//cout << LF[5].rows << LF[5].cols << endl;
	Mat dst = Mat::zeros(LF[0].rows, LF[0].cols, CV_64FC1);

	int viewnum = LF.size();
	double dividenum = 0;
	const int onednum = sqrt(viewnum);

	int *base = new int[onednum];
	int i = onednum / 2;


	for (int k = 0; k < onednum; k++) {
		base[k] = i;
		i--;
	}

	for (int num = 0; num < viewnum; num++) {
		int x = num % onednum;
		int y = num / onednum;
		double shift_x = base[x] * disparity;
		double shift_y = base[y] * disparity;

		ShiftandAddImage_double(LF[num], dst, shift_x, shift_y);
	}
	dst = dst / (double)viewnum;
	dst.convertTo(dst, CV_8UC1);
	return dst;
}
void ShiftandAddImage_double(Mat src, Mat dst, double shift_x, double shift_y) {
	int x = (int)shift_x;
	int y = (int)shift_y;

	double dx = shift_x - (double)x;
	double dy = shift_y - (double)y;

	double scale_x = abs(dx);
	double scale_y = abs(dy);

	double imscale_n = (1.0 - abs(dx)) * (1.0 - abs(dy));
	double imscale_x = abs(dx) * (1.0 - abs(dy));
	double imscale_y = (1.0 - abs(dx)) * abs(dy);
	double imscale_xy = abs(dx) * abs(dy);

	int add_x, add_y;
	if (dx > 0) {
		add_x = 1;
	}
	else if (dx == 0) {
		add_x = 0;
	}
	else {
		add_x = -1;
	}


	if (dy > 0) {
		add_y = 1;
	}
	else if (dy == 0) {
		add_y = 0;
	}
	else {
		add_y = -1;
	}

	ShiftandAddImage(src * imscale_n, dst, x, y);
	ShiftandAddImage(src * imscale_x, dst, x + add_x, y);
	ShiftandAddImage(src * imscale_y, dst, x, y + add_y);
	ShiftandAddImage(src * imscale_xy, dst, x + add_x, y + add_y);

}
void ShiftandAddImage(Mat src, Mat dst, int shift_x, int shift_y) {

	int src_x = 0, src_y = 0, dst_x = 0, dst_y = 0;

	if (shift_x > 0)dst_x = shift_x;
	if (shift_x < 0)src_x = -shift_x;


	if (shift_y > 0)dst_y = shift_y;
	if (shift_y < 0)src_y = -shift_y;

	int W = src.cols - abs(shift_x);
	int H = src.rows - abs(shift_y);

	kasankirihari(src, src_x, src_y, dst, dst_x, dst_y, W, H);
}

void kasankirihari(Mat src, int src_x, int src_y, Mat dst, int dst_x, int dst_y, int smallarea_x, int smallarea_y) {

	Rect src_roi = Rect(src_x, src_y, smallarea_x, smallarea_y);
	Rect dst_roi = Rect(dst_x, dst_y, smallarea_x, smallarea_y);
	Mat img_src = src(src_roi);
	Mat img_dst = dst(dst_roi);
	img_dst = img_src + img_dst;
}