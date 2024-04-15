#ifndef GstWrapper_H
#define GstWrapper_H

#include <gst/gst.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>

using namespace std;


typedef struct{
    string name;
    vector<string> gst_sink_pipeline_strs;

    int init_width = 1920;
    int init_height = 1080;
}MultiSinksConfig;

/*!
* VideoStreamElement
* @param appsrc: GstElement*: main stream input appsrc
* @param tee: GstElement*: main stream branch beginning
* @param sink_elements: vector<GstElement*>: a bunch of filter elements string
*/
typedef struct{
    MultiSinksConfig config;
    
    GstElement* appsrc;
    GstElement* tee;

    vector<GstElement*> sink_elements;

    bool cap_set;
}MultiSinksElement;

/*!
* VideoStreamConfig
* @param name: name of the config
* @param gst_pipeline_str: gst elements pipeline
*/

typedef struct{
    string name;
    string gst_pipeline_str;
    // bool allow_duplicated_frame_read;

        
    // uint64_t state_checking_interval = 5000;
    // uint64_t state_checking_max_frame_arrival = 5000;
    // int state_checking_restart_after = 6;

    
    // MultiSinksConfig multi_sinks_config;
}VideoStreamConfig;


typedef struct{
    VideoStreamConfig config;

    GstElement* element;

    GstPad* self_src_pad;

    gulong probe_id;
    MultiSinksElement* main_stream_source_sinks;
    MultiSinksElement* stream_sinks;
    
    uint64_t last_buffer_received_at;
    GstBuffer* last_buffer;
    int last_buffer_width;
    int last_buffer_height;
    int last_buffer_framerate_numerator;
    int last_buffer_framerate_denominator;

    
    uint64_t state_checking_at;
    int state_checking_failure_cnt;

    std::mutex* buffer_mutex;
} VideoStreamElement;



typedef struct{
    MultiSinksConfig main_stream_config;
    vector<VideoStreamConfig> video_streams_config;
}WrapperConfig;



/* 
|                 ->   branch-steamer-1     |
|                 ->   branch-steamer-2     |
|   appsrc -> tee ->   branch-steamer-3     |
|   (main stream) ->   branch-steamer-4     |
|                 ->   branch-steamer-5     |
*/
class GstWrapper
{
    private:

        GstElement* main_pipeline;
        GstClock* gst_clock;


        MultiSinksElement* main_stream_elements;
        vector<VideoStreamElement*> multi_streams_elements;


        void _init();

        MultiSinksElement* _create_appsrc_tee_elements(MultiSinksConfig config);
        VideoStreamElement* _create_branch_stream_elements(VideoStreamConfig config);

    public:
        GstWrapper(WrapperConfig args);
        ~GstWrapper();
        void start();
      
};

#endif