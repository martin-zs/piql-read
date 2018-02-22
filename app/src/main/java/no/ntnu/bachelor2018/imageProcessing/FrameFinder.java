package no.ntnu.bachelor2018.imageProcessing;

import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfInt;
import org.opencv.core.MatOfPoint;
import org.opencv.core.MatOfPoint2f;
import org.opencv.core.Point;
import org.opencv.imgproc.Imgproc;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

/**
 * Created by Håkon Heggholmen on 13.02.2018.
 * Class is used to find the outer frame of the image
 */

public class FrameFinder {

    private final String TAG = this.getClass().getSimpleName();

    private int width, height;
    private Mat contourImage, hierarchy;
    private List<MatOfPoint> contours;
    private MatOfPoint2f contour2f;
    private MatOfInt hull;
    private List<Point> retPoints;

    public FrameFinder(int width, int height){
        this.width = width;
        this.height = height;
        hierarchy = new Mat(height, width, CvType.CV_8UC1);
        contours = new ArrayList<>();
        contour2f = new MatOfPoint2f();
        hull = new MatOfInt();
        retPoints = new ArrayList<>();

    }

    /**
     * Finds corners of film frame(black boarder edge corners)
     * @param image
     * @return
     */
    public List<Point> cornerFinder(Mat image){
        //Init variables
        Point points[];
        boolean done = false;
        contours.clear();
        retPoints.clear();

        //Find outer contour
        Imgproc.findContours(image, contours,hierarchy, Imgproc.RETR_EXTERNAL, Imgproc.CHAIN_APPROX_SIMPLE);

        //Loop through all contours
        for(MatOfPoint conto: contours){
            //Filter out small contour with area less then
            if(Imgproc.contourArea(conto)>Math.pow(height/3,2) && !done){

                //Approximate polygon line to contour
                conto.convertTo(contour2f,CvType.CV_32FC2);
                Imgproc.approxPolyDP(contour2f,contour2f,10,true);
                contour2f.convertTo(conto,CvType.CV_32S);

                points = conto.toArray();

                //Find corner points
                Imgproc.convexHull(conto,hull,true);
                //add corner points to return vector, only quads are accepted
                if(hull.size().area() == 4){
                    for(int hullId:hull.toArray()){
                        retPoints.add(points[hullId]);

                    }
                    done = true;
                }

            }
        }
        return retPoints;
    }
}
