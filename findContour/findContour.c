/*
 A Simple OpenCV Tutorial
 by Jean-Marc Pelletier (c)2009
 
 This program identities edges and contours in the camera image. When the user clicks inside a closed path, the area of the contour immediately enclosing the mouse pointer is 
 written out.
 
 The user can start or stop the image by pressing the space bar and quit the application by pressing the "esc" key.
 
 This turorial is meant to show the basic structure of a simple OpenCV application. Furthermore, it shows how one can work with sequences and contours, which are important
 parts of OpenCV that could be better documented.
 
 */

//OpenCV includes:
#ifdef __APPLE__
	#include "OpenCV/cv.h"
	#include "OpenCV/highgui.h"
#else
	#include "cv.h"
	#include "highgui.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/videodev2.h>

//Useful values
#define MAX_CONTOUR_LEVELS 10 //This will be used when displaying contours

#define IMG_WIDTH 640  //Image width
#define IMG_HEIGHT 480 //Image height

#define DEFAULT_TRACKBAR_VAL 128 //For the trackbars

#define	MAIN_WINNAME	"FindContour"

//Function declarations
void onMouse(int event, int x, int y, int flags, void *params); //This will be called when the user performs a mouse action.
CvContour* contourFromPosition(CvContour *contours, int x, int y); //Returns a pointer to the inner-most contour surrounding the given point
char pointIsInsideContour(CvContour *contour, int x, int y); //Returns whether the given position is inside a contour

int camID=-1;
char inFile[1024], orgFile[1024];
int roi_x=157,roi_y=76;

static void errno_exit(const char *s)
{
	printf("+%s:%s error %d, %s\n", __func__, s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}


static void usage(FILE *fp, int argc, char **argv)
{
	fprintf(fp,
		 "Usage: %s [options]\n\n"
		 "Options:\n"
		 "-h | --help           Print this message\n"
		 "-d | --device         Video device id:/dev/video0,1,2,3\n"
		 "-i | --inputfile      binary contour file name\n"
		 "-o | --orgfile        original file name to extract contour\n"
		 "-r | --roi            \"157,76\" the start pos(x,y) of contour roi"
		 "", argv[0]);
}

static const char short_options[] = "d:hi:o:r:";
static const struct option
long_options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "device", required_argument, NULL, 'd' },
	{ "inputfile",required_argument, NULL, 'i' },
	{ "orgfile",required_argument, NULL, 'o' },
	{ "roi",required_argument, NULL, 'r' },
	{ 0, 0, 0, 0 }
};

static int option(int argc, char **argv)
{
	int r=0;
	printf("+%s:\n",__func__);

	for (;;) {
		int idx;
		int c;

		c = getopt_long(argc, argv,
				short_options, long_options, &idx);
		printf("c=%d\n",c);
		if (-1 == c)//no option is available
			break;

		switch (c) {
		case 0: /* getopt_long() flag */
			break;
		case 'd':
			errno = 0;
			camID = strtol(optarg, NULL, 0);
			if (errno)
				errno_exit(optarg);
			break;
		case 'o':
			errno = 0;
			strncpy(orgFile, optarg, 1024);
			printf("%s\n", orgFile);
			if (errno)
				errno_exit(optarg);
			break;
		case 'i':
			errno = 0;
			strncpy(inFile, optarg, 1024);
			printf("%s\n", inFile);
			if (errno)
				errno_exit(optarg);
			break;
		case 'r':
			errno = 0;
			sscanf(optarg, "%d,%d", &roi_x, &roi_y);
			printf("roi_x=%d,roi_y=%d\n",roi_x, roi_y);
			if (errno)
				errno_exit(optarg);
			break;
		case 'h':
			errno = 0;
			usage(stderr, argc, argv);
			r=-2;
			break;
		default:
			usage(stderr, argc, argv);
			r=-1;
			break;
		}
	}
	return r;
}

/* http://jmpelletier.com/a-simple-opencv-tutorial/
 * You can access other contours using the h_next and v_next members of CvSeq. 
 * If you specify CV_RETR_TREE to find contour, 
 * the pointer v_next points to the first contour that is inside the current contour. 
 * On the other hand, h_next points to the first contour that is outside the current contour. 
 * 
 * h_next points the outside contours
 * v_next points to the inside contours of the outside contour
 *
 */
//This boolean function returns whether the (x,y) point is inside the given contour.
CvContour * LargestContour(void *contours){
	//We know that a point is inside a contour if we can find one point in the contour that is immediately to the left,
	//one that is immediately on top, one that is immediately to the right and one that is immediately below the (x,y) point.
	//We will update the boolean variables below when these are found:
	char found_left=0, found_top=0, found_right=0, found_bottom=0;
	int count, i; //Variables used for iteration
	
	if(!contours)return 0; //Don't bother doing anything if there is no contour.
	
	CvContour *output=NULL, *contour = *(CvContour **)contours;
	float maxArea=0.0f;
	while(contour){ //We loop through the list of contours...
		float area=0.0;
		area = fabs(cvContourArea(contour, CV_WHOLE_SEQ,0));
		if(maxArea < area){
			maxArea = area;
			output = contour;
		}
		count = contour->total; //The total field holds the number of points in the contour.
		printf("outside contour : area=%f, points=%d\n",area, count);
		CvContour *inside=(CvContour *)contour->v_next;
		while(inside){ //Check to see if the point the user clicked on is inside the current contour, if yes...
			//output = contour; //Since the point is inside the contour, set the output accordingly.
			area = fabs(cvContourArea(inside, CV_WHOLE_SEQ,0));
			count = inside->total; //The total field holds the number of points in the contour.
			printf("	inside Area: area=%f, points=%d\n", area, count); //and print the result out.
			//We now need to check all the contours inside the one we are processing. We go down one level by following the v_next (vertical next) pointer in the 
			//CvSeq/CvContour structure. From then, we recurse into this function to go through all the contours.
			inside = (CvContour *)inside->v_next;
		}

		//Once we're done with this contour, we need to move to the following contour that's not included in the one we just processed.
		//This is pointed to by the h_next (horizontal next) pointer. 
		contour = (CvContour *)(contour->h_next);
	}
	printf("max contour %p, area=%f\n",output, maxArea);
	return output;
}

/*
 * -d 0,1,2,3,4 =>/dev/video0,video1,...
 * -f static file
 * 
 */
int main (int argc, char * argv[]) {
	//Two boolean variables
	char quit = 0; //Exit main program loop?
	char grab_frame = 1; //Do we grab a new frame from the camera?
	
	int thresh1=DEFAULT_TRACKBAR_VAL, thresh2=DEFAULT_TRACKBAR_VAL; //These two variables will hold trackbar positions.

	if(argc==1){
		camID=0;
		printf("set default camera 0\n");
	}else if(option(argc,argv) < 0 ){
		printf( "Wrong args!!!\n");
		exit(EXIT_FAILURE);
	}

	//These are pointers to IPL images, which will hold the result of our calculations
	IplImage *small_image = NULL; /*cvCreateImage(cvSize(IMG_WIDTH,IMG_HEIGHT),IPL_DEPTH_8U,3)*/; //size, depth, channels (RGB = 3)
	IplImage *small_grey_image = NULL;/*cvCreateImage(cvGetSize(small_image), IPL_DEPTH_8U, 1)*/; //1 channel for greyscale
	IplImage *edge_image = NULL; /*cvCreateImage(cvGetSize(small_image), IPL_DEPTH_8U, 1)*/; //We use cvGetSize to make sure the images are the same size. 
	
	//CvMemStorage and CvSeq are structures used for dynamic data collection. CvMemStorage contains pointers to the actual
	//allocated memory, but CvSeq is used to access this data. Here, it will hold the list of image contours.
	CvMemStorage *storage = cvCreateMemStorage(0);
	CvSeq *contours = 0;
	
	CvCapture *camera=NULL;
	if(camID>=0){
		camera = cvCreateCameraCapture(camID); //This function tries to connect to the first (0th) camera it finds and creates a structure to refer to it.
		if(!camera){ //cvCreateCameraCapture failed, most likely there is no camera connected.
			printf("Could not find a camera to capture from...\n"); //Notify the user...
			return -1; //And quit with an error.
		}
	}
	cvNamedWindow(MAIN_WINNAME, CV_WINDOW_AUTOSIZE); //Here we create a window and give it a name. The second argument tells the window to not automatically adjust its size.
	
	//We add two trackbars (sliders) to the window. These will be used to set the parameters for the Canny edge detection.
	cvCreateTrackbar("Thresh1", MAIN_WINNAME, &thresh1, 256, 0);
	cvCreateTrackbar("Thresh2", MAIN_WINNAME, &thresh2, 256, 0);
	
	//Set the trackbar position to the default value. 
	cvSetTrackbarPos("Thresh1", MAIN_WINNAME, DEFAULT_TRACKBAR_VAL); //Trackbar name, window name, position
	cvSetTrackbarPos("Thresh2", MAIN_WINNAME, DEFAULT_TRACKBAR_VAL);
	
	//Now set the mouse callback function. We need to pass the location of the contours so that it will be able to access this information.
	cvSetMouseCallback(MAIN_WINNAME, onMouse, &contours); //Window name, function pointer, user-defined parameters.
	
	IplImage *frame=NULL; //This will point to the IPL image we will retrieve from the camera.
	IplImage *frame_org=NULL;
	if(camID < 0){
		frame_org = cvLoadImage(orgFile, CV_LOAD_IMAGE_UNCHANGED);
		frame = cvLoadImage(inFile, CV_LOAD_IMAGE_UNCHANGED);
	}
	//This is the main program loop. We exit the loop when the user sets quit to true by pressing the "esc" key
	while(!quit){
		int c =0;
		if(camID >= 0){
			c=cvWaitKey(30); //Wait 30 ms for the user to press a key.
			
			//Respond to key pressed.
			switch(c){
				case 32: //Space
					grab_frame = !grab_frame; //Reverse the value of grab_frame. That way, the user can toggle by pressing the space bar.
					break;
				case 27: //Esc: quit application when user presses the 'esc' key.
					quit = 1; //Get out of loop
					break;
			};

			//If we don't have to grab a frame, we're done for this pass.
			if(!grab_frame)continue;	

			//Grab a frame from the camera.
			frame = cvQueryFrame(camera);
			
			if(!frame) continue; //Couldn't get an image, try again next time.
		}
		
		//In computer vision, it's always better to work with the smallest images possible, for faster performance.
		//cvResize will use inter-linear interpolation to fit frame into small_image.
		if(small_image==NULL){
			small_image = cvCreateImage(cvSize(frame->width,frame->height),IPL_DEPTH_8U,3); //size, depth, channels (RGB = 3)
			printf("w=%d, h=%d\n", frame->width,frame->height);
		}
		if(small_grey_image==NULL) small_grey_image=cvCreateImage(cvGetSize(small_image), IPL_DEPTH_8U, 1);
		if(edge_image==NULL) edge_image = cvCreateImage(cvGetSize(small_image), IPL_DEPTH_8U, 1);

		cvResize(frame, small_image, CV_INTER_LINEAR);
		
		//Many computer vision algorithms do not use colour information. Here, we convert from RGB to greyscale before processing further.
		cvCvtColor(small_image, small_grey_image, CV_RGB2GRAY);

		//We then detect edges in the image using the Canny algorithm. This will return a binary image, one where the pixel values will be 255 for 
		//pixels that are edges and 0 otherwise. This is unlike other edge detection algorithms like Sobel, which compute greyscale levels.
		cvCanny(small_grey_image, edge_image, (double)thresh1, (double)thresh2, 3); //We use the threshold values from the trackbars and set the window size to 3
		
		//The edges returned by the Canny algorithm might have small holes in them, which will cause some problems during contour detection.
		//The simplest way to solve this problem is to "dilate" the image. This is a morphological operator that will set any pixel in a binary image to 255 (on) 
		//if it has one neighbour that is not 0. The result of this is that edges grow fatter and small holes are filled in.
		//We re-use small_grey_image to store the results, as we won't need it anymore.
		cvDilate(edge_image, small_grey_image, 0, 1);
		
		//Once we have a binary image, we can look for contours in it. cvFindContours will scan through the image and store connected contours in "sorage".
		//"contours" will point to the first contour detected. CV_RETR_TREE means that the function will create a contour hierarchy. Each contour will contain 
		//a pointer to contours that are contained inside it (holes). CV_CHAIN_APPROX_NONE means that all the contours points will be stored. Finally, an offset
		//value can be specified, but we set it to (0,0).
		cvFindContours(small_grey_image, storage, &contours, sizeof(CvContour), CV_RETR_TREE, 
					   CV_CHAIN_APPROX_NONE, cvPoint(0,0));
		CvContour *max_contour = LargestContour(&contours);
		//This function will display contours on top of an image. We can specify different colours depending on whether the contour in a hole or not.
		cvDrawContours(small_image, contours, CV_RGB(255,0,0), CV_RGB(0,255,0), MAX_CONTOUR_LEVELS, 
					   1, CV_AA, cvPoint(0,0));
		
		//Finally, display the image in the window.
		cvShowImage(MAIN_WINNAME, small_image);
		if(camID< 0){
			//draw the max contour on the original
			cvDrawContours(frame_org, (CvSeq *)max_contour, CV_RGB(0, 255,0), CV_RGB(0,200,0), 
						0, 1, CV_AA, cvPoint(roi_x,roi_y));		
			cvShowImage(orgFile, frame_org);
			c=cvWaitKey(0); //Wait 30 ms for the user to press a key.
			//Respond to key pressed.
			if(c == 27) quit = 1; //Get out of loop
		}
	}
	
	//Clean up before quitting.
	cvDestroyAllWindows(); //This function releases all the windows created so far.
	cvReleaseCapture(&camera); //Release the camera capture structure.
	
	//release memory
	cvReleaseMemStorage(&storage);
	
	//Release images
	cvReleaseImage(&small_image); //We pass a pointer to a pointer, because cvReleaseImage will set the image pointer to 0 for us.
	cvReleaseImage(&small_grey_image);
	cvReleaseImage(&edge_image);
	if( (camID < 0) && frame ){
		cvReleaseImage(&frame);
	}
	return 0; //We're done.
}


//When the user clicks in the window...
void onMouse(int event, int x, int y, int flags, void *params){
	//params points to the contour sequence. Here we cast the pointer to CvContour instead of CvSeq.
	//CvContour is in fact an extension of CvSeq and is the structure used by cvFindContours, if we cast 
	//to CvSeq, we won't be able to access the fields specific to CvContour.
	CvContour *contours = *(CvContour **)params;
	CvContour *ctr; //This will point to the specific contour the user clicked in.
	double area; //This will hold the area of the contour the user clicked in.
	
	if(!contours)return; //If there are no contours, we don't bother doing anything.
	
	//"event" tells us what occured
	switch(event){
		case CV_EVENT_LBUTTONDOWN: //single click
		case CV_EVENT_LBUTTONDBLCLK: //double click
			printf("Click: %d %d\n",x,y);  //Write out where the user clicked.
			
			//Here we retrieve the inner-most contour the user clicked in. If there is no such contour,
			//the function returns a null pointer, which we need to check against.
			ctr = contourFromPosition(contours, x, y);
			if(ctr){ //The user did click inside something
				//Calculate the area of the contour using cvContourArea. CV_WHOLE_SEQ means we want the area of the whole contour. 
				//Important: we need to calculate the absolute value of the result using fabs because cvContourArea can return negative values.
				area = fabs(cvContourArea(ctr, CV_WHOLE_SEQ,0));
				printf("Area: %f\n",area); //and print the result out.
			}
			
			break;
		case CV_EVENT_LBUTTONUP: //single click up
		case 10: //double-click up? (not documented)
			break;
	}
}

CvContour* contourFromPosition(CvContour *contours, int x, int y){
	CvContour *contour; //This will point to a single contour.
	CvContour *tmpContour=0, *output=0; //Two more pointers, first a temporary contour and the final output.
	
	if(!contours)return 0; //Don't bother doing anything if there are no contours.
	
	contour = contours; //Now contour points to the first contour in the list
	
	while(contour){ //We loop through the list of contours...
		if(pointIsInsideContour(contour, x, y)){ //Check to see if the point the user clicked on is inside the current contour, if yes...
			output = contour; //Since the point is inside the contour, set the output accordingly.
			
			//We now need to check all the contours inside the one we are processing. We go down one level by following the v_next (vertical next) pointer in the 
			//CvSeq/CvContour structure. From then, we recurse into this function to go through all the contours.
			if(contour->v_next){
				//It's possible that this will return a null pointer, so we store the results in a temporary pointer...
				tmpContour = contourFromPosition((CvContour *)contour->v_next,x,y);
				//and only update the output if the pointer is valid.
				if(tmpContour) output = tmpContour;
				
				//At this point, because the sequence is organized as a tree, we know that output points to the inner-most contour, which is what we want.
			}
		}
		
		//Once we're done with this contour, we need to move to the following contour that's not included in the one we just processed.
		//This is pointed to by the h_next (horizontal next) pointer. 
		contour = (CvContour *)(contour->h_next);
	}
	
	return output;
}

//This boolean function returns whether the (x,y) point is inside the given contour.
char pointIsInsideContour(CvContour *contour, int x, int y){
	//We know that a point is inside a contour if we can find one point in the contour that is immediately to the left,
	//one that is immediately on top, one that is immediately to the right and one that is immediately below the (x,y) point.
	//We will update the boolean variables below when these are found:
	char found_left=0, found_top=0, found_right=0, found_bottom=0;
	int count, i; //Variables used for iteration
	CvPoint *contourPoint; //A pointer to a single contour poit.
	
	if(!contour)return 0; //Don't bother doing anything if there is no contour.
	
	count = contour->total; //The total field holds the number of points in the contour.

	for(i=0;i<count;i++){ //So, for every point in the contour...
		//We retrieve a pointer to the point at index i using the useful macro CV_GET_SEQ_ELEM
		contourPoint = (CvPoint *)CV_GET_SEQ_ELEM(CvPoint,contour,i);
		
		if(contourPoint->x == x){ //If the point is on the same vertical plane as (x,y)...
			if(contourPoint->y < y)found_top = 1; //and is above (x,y), we found the top.
			else found_bottom = 1; //Otherwise, it's the bottom.
		}
		if(contourPoint->y == y){ //Do the same thing for the horizontal axis...
			if(contourPoint->x < x)found_left = 1;
			else found_right = 1;
		}
	}
	
	return found_left && found_top && found_right && found_bottom; //Did we find all four points?
}