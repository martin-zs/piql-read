package no.ntnu.bachelor2018.filmreader;

import android.content.SharedPreferences;

import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;
import org.opencv.imgproc.Imgproc;


import no.ntnu.bachelor2018.imageProcessing.FrameFinder;

/**
 * Created by hcon on 13.02.18.
 */

public class Reader {

    private final String TAG = this.getClass().getSimpleName();

    private FrameFinder finder;

    private int width, height;
    private Mat grayImg;

    public Reader(int width, int height, SharedPreferences prefs){
        //TODO: Håkon add camera config parameter constructor
        finder = new FrameFinder(width, height, prefs);
        this.width = width;
        this.height = height;
        grayImg = new Mat(height, width, CvType.CV_8UC1);

    }

     /**
     * Main loop process that processes the image
     *
     * @param inputImage camera image frame
     */
    public Mat processFrame(Mat inputImage){
        return finder.drawEdges(inputImage, new Scalar(255, 255, 0));

    }
}
