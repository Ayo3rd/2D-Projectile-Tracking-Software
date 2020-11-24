#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#include <math.h>


using namespace cv;
using namespace std;

//CONSTANTS
//Window sizing
//const int W = 1000;
//const int L = 600;

//const int W = 1280;
//const int L = 774;

const int PADDLE_X = 80;
const int BOUND = 400;
const double DELTA_T = 0.0003;

//HSV Values
const int H_HIGH = 179;
const int S_HIGH = 255;
const int V_HIGH = 58;

//Math
const double PI = 3.141592653589793238463;


//Initilize Variables



void MyLine(Mat img, Point start, Point end)
{
    int thickness = 2;
    int lineType = LINE_8;
    line(img,
        start,
        end,
        Scalar(0, 0, 0),
        thickness,
        lineType);
}

// 
double getY(double oldY, double L)
{
    return L - oldY;
}

//
void paddleMaker(double yPos, Mat image) 
{
    //Create point of Paddle
    Point p2(PADDLE_X, yPos);

    //Draw paddle
    circle(image, p2, 30, Scalar(300, 100, 50), -1);
    circle(image, p2, 31, Scalar(128, 0, 0), 1, 8);
    circle(image, p2, 13, Scalar(128, 0, 0), -1);
}



double prediction(double x1, double y1, double x2, double y2, double deltaT, double tableLengthX, double tableHeightY, double radius)
{
    //double maxAngle = 1; // update as necessary 
    //double maxSpeed = 500000; //update as necessary 

    double theta = atan((y2 - y1) / (x2 - x1));
    double slope = (y2 - y1) / (x2 - x1);
    double intercept = y2 - slope * x2;
    double speed = sqrt(pow((x1 - x2), 2) + pow((y1 - y2), 2)) / deltaT;

    //if (theta > maxAngle) {
    //    return -1;
    //}

    //if (speed > maxSpeed) {
    //    return -2;
    //}

  
    
    double hypotheticalY = slope * PADDLE_X + intercept; //where the puck would travel to if there were no walls 

    slope = slope * -1; // new slope of puck movement after it bounces 



    if (hypotheticalY < 0)//bounce off the bottom of the table 
    { 
        double newx1 = (radius - hypotheticalY) / ((y2 - y1) / (x2 - x1));
        double newy1 = radius;

        double newx2 = newx1 + (x2 - x1); //could be plus or minus 
        double newy2 = slope * (newx2 - newx1) + newy1;

        prediction(newx1, newy1, newx2, newy2, deltaT, tableLengthX, tableHeightY, radius);
    }
    else if (tableHeightY < hypotheticalY)//bounce off the top of the table
    {  
        double newx1 = (tableHeightY - y2) / slope + x2 - radius / tan(theta);
        double newy1 = tableHeightY - radius;

        double newx2 = newx1 + (x2 - x1); //could be plus or minus 
        double newy2 = slope * (newx2 - newx1) + newy1;;

        prediction(newx1, newy1, newx2, newy2, deltaT, tableLengthX, tableHeightY, radius);
    }
    else 
    {
        return hypotheticalY;
    }
}








int main(int argc, char** argv)
{
    //VideoCapture cap("final_tester.mp4"); 
    VideoCapture cap("Final_Master_Cropped.mp4");//capture the video from web cam

    

    if (!cap.isOpened())  // if not success, exit program
    {
        cout << "Cannot open the video" << endl;
        return -1;
    }

    //Controls for adjusting coloring filter
    /*
   
    namedWindow("Control", WINDOW_NORMAL); //create a window called "Control"

    //Tried and worked (0,179...106,255,....0,70)
    //Tried and worked (0,179...0,255,....0,58)  GREAT

    int iLowH = 0;
    int iHighH = 179;

    int iLowS = 0;
    int iHighS = 255;

    int iLowV = 0;
    int iHighV = 255;

    //Create trackbars in "Control" window
    createTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
    createTrackbar("HighH", "Control", &iHighH, 179);

    createTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
    createTrackbar("HighS", "Control", &iHighS, 255);

    createTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
    createTrackbar("HighV", "Control", &iHighV, 255);

    */

    //Capture a temporary image from the camera
    Mat imgTmp;
    cap.read(imgTmp);

    //Set Width and height of image
    const int W = 1280;
    const int L = 774;

    
    //Create a black image with the size as the camera output
    Mat imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);

    //Initialize last coordinates for line tracker and Initial Predicted Paddle position
    double iLastX = -1;
    double iLastY = -1;
    double predY = L/2;

    

    while (true)
    {
        Mat imgOriginal;

        bool bSuccess = cap.read(imgOriginal); // read a new frame from video

        if (!bSuccess) //if not success, break loop
        {
            cout << "Cannot read a frame from video stream" << endl;
            break;
        }

        Mat imgHSV;

        cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
        blur(imgHSV, imgHSV, Size(3,3));

        Mat imgThresholded;

        //inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgThresholded); //Threshold the image

        inRange(imgHSV, Scalar(0, 0, 0), Scalar(H_HIGH, S_HIGH, V_HIGH), imgThresholded); //Threshold the image

        //morphological opening (remove small objects from the foreground)
        erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

        //morphological closing (fill small holes in the foreground)
        dilate(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
        erode(imgThresholded, imgThresholded, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

        //Crop out Goals from image
        //Mat croppedThresholded = imgThresholded(Rect(0, 0, W-40, L-40));

        //Calculate the moments of the thresholded image
        Moments oMoments = moments(imgThresholded,true);

        double dM01 = oMoments.m01;
        double dM10 = oMoments.m10;
        double dArea = oMoments.m00;
                     

         //if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 

        //calculate the position of the ball
        double posX = dM10 / dArea;
        double posY = dM01 / dArea;
        Point p(posX, posY);

        //Get radius of Puck
        double radius = sqrt(dArea/ PI) + 1;

        //Apply prediction function when puck moves left 
        if(iLastX > posX)
        {
           
            cout << "Current: " << posX << " , " << getY(posY, L) << endl;
            cout << "Last: " << iLastX << " , " << getY(iLastY, L) << endl;


          predY =  prediction(iLastX, getY(iLastY,L), posX, getY(posY,L), DELTA_T, W, L, radius);

           
           cout << "Predicted Y:" << predY << endl;
           
           //Adjust Y back 
           predY = L-predY;

           //cout << predY << endl;
           paddleMaker(predY, imgThresholded);
           
        }
        else
        {
            //Paddle in center 
            double tempY = L / 2;
            paddleMaker(tempY, imgThresholded);
        }



        ////Draw dotted line to show artificial bound
        //LineIterator it(imgOriginal, Point(BOUND, 0), Point(BOUND, L), 8);
        //for (int i = 0; i < it.count; i++, it++)
        //{           
        //    if (i % 5 != 0)
        //    {
        //        (*it)[0] = 0; // Blue
        //        (*it)[1] = 0; // Green
        //        (*it)[2] = 0; // Red
        //    }
        //}
        
        //Red Tracking Line
        if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0 && abs(posX - iLastX) < 200)
        {
            //Draw a red line from the previous point to the current point
            line(imgLines, Point(posX, posY), Point(iLastX, iLastY), Scalar(0, 0, 255), 2);
        }

        //Remove previous redlines after clip is reset
        if(abs(posX - iLastX) > 200)
        {
            imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);
        }
            
        //Reset last coordinate
        iLastX = posX;
        iLastY = posY;

        //Debug Coordinate printer
        //cout << posX << "  " << posY << endl;


        //Display Thresholded Image
        namedWindow("Thresholded Image", WINDOW_NORMAL);
        resizeWindow("Thresholded Image", W, L);
        imshow("Thresholded Image", imgThresholded); //show the thresholded image


    
        //Display Original Image with tracker line
        imgOriginal = imgOriginal + imgLines;
        namedWindow("Original Image", WINDOW_NORMAL);
        resizeWindow("Original Image", W, L);
        imshow("Original Image", imgOriginal); //show the original image


        if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
        {
            cout << "esc key is pressed by user" << endl;
            break;
        }
    }

    return 0;

}

