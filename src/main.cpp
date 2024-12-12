#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include <iostream>
#include <wrapper.cpp>
using namespace cv;
using namespace std;



// timestamp
// [Sombra] -> https://stackoverflow.com/questions/47793422/how-to-use-use-the-gst-app-src-push-buffer-manually-with-custom-event

int main()
{
    /*
    gst-launch-1.0 -v v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! queue ! fpsdisplaysink video-sink=glimagesink
    gst-launch-1.0 -v filesrc location=../1.mp4 ! decodebin name=dec ! videoconvert  ! capsfilter caps="video/x-raw, format=BGR"  ! queue ! fpsdisplaysink video-sink=glimagesink
    */
    WrapperConfig config = {
        main_stream_config: {
            name: "main_stream",
            gst_sink_pipeline_strs: {
                "queue ! videoconvert ! fpsdisplaysink sync=false"
            }
        },
        video_streams_config: {
            VideoStreamConfig {
                name: "video-1",
                gst_pipeline_str: "v4l2src device=/dev/video0 ! videoconvert ! video/x-raw,format=BGR,width=640,height=480,framerate=30/1"
            },
            // VideoStreamConfig {
            //     name: "video-2",
            //     gst_pipeline_str: "filesrc location=1.mp4 ! decodebin ! videoconvert ! capsfilter caps=\"video/x-raw, format=BGR\" ! identity sync=true"
            // },
        }
    };


    GstWrapper gst_wrapper(config);
    spdlog::info("Exiting");


    // GstElement *pipeline;
    // GstBus *bus;
    // GstMessage *msg;

    // /* Initialize GStreamer */
    // gst_init (&argc, &argv);

    // std::string pipe = "rtspsrc location=rtsp://root:root@192.168.0.95/onvif-media/media.amp?profile=profile_1_h264 ! \
    // rtph264depay ! h264parse ! nvh264dec ! videoconvert ! appsink sync=false";

    // std::string pipe1 = "rtspsrc location=rtsp://root:root@192.168.0.95/onvif-media/media.amp?profile=profile_1_h264 ! \
    // rtph264depay ! h264parse ! nvh264dec ! videoconvert ! fpsdisplaysink sync=false";

    // /* Build the pipeline */
    // pipeline =
    //     gst_parse_launch
    //     (pipe1.c_str(), NULL);

    // /* Start playing */
    // gst_element_set_state (pipeline, GST_STATE_PLAYING);

    // /* Wait until error or EOS */
    // bus = gst_element_get_bus (pipeline);
    // msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);
    
    // if (msg != NULL) {
    //     gst_message_unref(msg);
    //     gst_object_unref(bus);
    //     gst_element_set_state(pipeline, GST_STATE_NULL);
    //     gst_object_unref(pipeline);
    // }
    // printf("finish");

    // VideoCapture cap(pipe, CAP_GSTREAMER);

    // if (!cap.isOpened()) {
    //     cerr <<"VideoCapture not opened"<<endl;
    //     exit(-1);
    // }

    // while (true) {
    //     Mat frame;

    //     cap.read(frame);

    //     imwrite("receiver.png", frame);

    //     getchar();
    // }

    return 0;
}