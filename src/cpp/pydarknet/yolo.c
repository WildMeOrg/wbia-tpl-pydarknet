#include "network.h"
#include "detection_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"

#ifdef OPENCV
#include "opencv2/highgui/highgui_c.h"
#endif

// char *voc_names[] = {"aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow", "diningtable", "dog", "horse", "motorbike", "person", "pot$
// char *voc_names[] = {"elephant savanna", "giraffe reticulated", "giraffe masai", "zebra grevys", "zebra plains"};
// image voc_labels[5];

// void train_yolo(char *cfgfile, char *weightfile)
// {
//     char *train_images = "/media/hdd/jason/yolo/VOC/train.txt";
//     char *backup_directory = "/media/hdd/jason/yolo/backup/";
//     srand(time(0));
//     data_seed = time(0);
//     char *base = basecfg(cfgfile);
//     printf("%s\n", base);
//     float avg_loss = -1;
//     network net = parse_network_cfg(cfgfile, 1);
//     if(weightfile){
//         load_weights(&net, weightfile);
//     }
//     printf("Learning Rate: %g, Momentum: %g, Decay: %g\n", net.learning_rate, net.momentum, net.decay);
//     int imgs = net.batch*net.subdivisions;
//     int i = *net.seen/imgs;
//     data train, buffer;

//     layer l = net.layers[net.n - 1];

//     int side = l.side;
//     int classes = l.classes;
//     float jitter = l.jitter;

//     list *plist = get_paths(train_images);
//     //int N = plist->size;
//     char **paths = (char **)list_to_array(plist);

//     load_args args = {0};
//     args.w = net.w;
//     args.h = net.h;
//     args.paths = paths;
//     args.n = imgs;
//     args.m = plist->size;
//     args.classes = classes;
//     args.jitter = jitter;
//     args.num_boxes = side;
//     args.d = &buffer;
//     args.type = REGION_DATA;

//     pthread_t load_thread = load_data_in_thread(args);
//     clock_t time;
//     //while(i*imgs < N*120){
//     while(get_current_batch(net) < net.max_batches){
//         i += 1;
//         time=clock();
//         pthread_join(load_thread, 0);
//         train = buffer;
//         load_thread = load_data_in_thread(args);

//         printf("Loaded: %lf seconds\n", sec(clock()-time));

//         time=clock();
//         float loss = train_network(net, train);
//         if (avg_loss < 0) avg_loss = loss;
//         avg_loss = avg_loss*.9 + loss*.1;

//         printf("%d: %f, %f avg, %f rate, %lf seconds, %d images\n", i, loss, avg_loss, get_current_rate(net), sec(clock()-time), i*imgs);
//         if(i%1000==0 || i == 600){
//             char buff[256];
//             sprintf(buff, "%s/%s_%d.weights", backup_directory, base, i);
//             save_weights(net, buff);
//         }
//         free_data(train);
//     }
//     char buff[256];
//     sprintf(buff, "%s/%s_final.weights", backup_directory, base);
//     save_weights(net, buff);
// }


void train_yolo_custom(network *net, char *train_images, char *backup_directory, char *weight_filepath, int verbose, int quiet)
{
    #ifndef GPU
        printf("[pydarknet c train] Cannot train the network using CPU\n");
        return;
    #endif

    srand(time(0));
    data_seed = time(0);
    float avg_loss = -1;
    printf("Learning Rate: %g, Momentum: %g, Decay: %g\n", net->learning_rate, net->momentum, net->decay);
    int imgs = net->batch*net->subdivisions;
    int i = *net->seen/imgs;
    data train, buffer;

    layer l = net->layers[net->n - 1];

    int side = l.side;
    int classes = l.classes;
    float jitter = l.jitter;

    list *plist = get_paths(train_images);
    //int N = plist->size;
    char **paths = (char **)list_to_array(plist);

    load_args args = {0};
    args.w = net->w;
    args.h = net->h;
    args.paths = paths;
    args.n = imgs;
    args.m = plist->size;
    args.classes = classes;
    args.jitter = jitter;
    args.num_boxes = side;
    args.d = &buffer;
    args.type = REGION_DATA;

    pthread_t load_thread = load_data_in_thread(args);
    clock_t time;
    //while(i*imgs < N*120){
    while(get_current_batch(*net) < net->max_batches){
        i += 1;
        time=clock();
        pthread_join(load_thread, 0);
        train = buffer;
        load_thread = load_data_in_thread(args);

        printf("Loaded: %lf seconds\n", sec(clock()-time));

        time=clock();
        float loss = train_network(*net, train);
        if (avg_loss < 0) avg_loss = loss;
        avg_loss = avg_loss*.9 + loss*.1;


        printf("%d: %f, %f avg, %f rate, %lf seconds, %d images\n", i, loss, avg_loss, get_current_rate(*net), sec(clock()-time), i*imgs);
        if(i%1000==0 || i == 600){
            char buff[256];
            sprintf(buff, "%s/detect.yolo.%d.%d.weights", backup_directory, l.classes, i);
            save_weights(*net, buff);
        }
        free_data(train);
    }
    sprintf(weight_filepath, "%s/detect.yolo.%d.weights", backup_directory, l.classes);
    save_weights(*net, weight_filepath);
}

void convert_yolo_detections(float *predictions, int classes, int num, int square, int side, int w, int h, float thresh, float **probs, box *boxes, int only_objectness, int offset, float dx, float dy, float ds)
{
    int i,j,n;
    int index_offset = side*side*num*offset;
    //int per_cell = 5*num+classes;
    for (i = 0; i < side*side; ++i){
        int row = i / side;
        int col = i % side;
        for(n = 0; n < num; ++n){
            int index = i*num + n + index_offset;
            int p_index = side*side*classes + i*num + n;
            float scale = predictions[p_index];
            int box_index = side*side*(classes + num) + (i*num + n)*4;
            boxes[index].x = (predictions[box_index + 0] + col) / side * w;
            boxes[index].x = boxes[index].x * ds + dx;
            boxes[index].y = (predictions[box_index + 1] + row) / side * h;
            boxes[index].y = boxes[index].y * ds + dy;
            boxes[index].w = pow(predictions[box_index + 2], (square?2:1)) * w;
            boxes[index].w = boxes[index].w * ds;
            boxes[index].h = pow(predictions[box_index + 3], (square?2:1)) * h;
            boxes[index].h = boxes[index].h * ds;
            for(j = 0; j < classes; ++j){
                int class_index = i*classes;
                float prob = scale*predictions[class_index+j];
                probs[index][j] = (prob > thresh) ? prob : 0;
            }
            if(only_objectness){
                probs[index][0] = scale;
            }
        }
    }
}

// void print_yolo_detections(FILE **fps, char *id, box *boxes, float **probs, int total, int classes, int w, int h)
// {
//     int i, j;
//     for(i = 0; i < total; ++i){
//         float xmin = boxes[i].x - boxes[i].w/2.;
//         float xmax = boxes[i].x + boxes[i].w/2.;
//         float ymin = boxes[i].y - boxes[i].h/2.;
//         float ymax = boxes[i].y + boxes[i].h/2.;

//         if (xmin < 0) xmin = 0;
//         if (ymin < 0) ymin = 0;
//         if (xmax > w) xmax = w;
//         if (ymax > h) ymax = h;

//         for(j = 0; j < classes; ++j){
//             if (probs[i][j]) fprintf(fps[j], "%s %f %f %f %f %f\n", id, probs[i][j],
//                     xmin, ymin, xmax, ymax);
//         }
//     }
// }

// void validate_yolo(char *cfgfile, char *weightfile)
// {
//     network net = parse_network_cfg(cfgfile, 1);
//     if(weightfile){
//         load_weights(&net, weightfile);
//     }
//     set_batch_network(&net, 1);
//     fprintf(stderr, "Learning Rate: %g, Momentum: %g, Decay: %g\n", net.learning_rate, net.momentum, net.decay);
//     srand(time(0));

//     char *base = "results/comp4_det_test_";
//     list *plist = get_paths("data/voc.2007.test");
//     //list *plist = get_paths("data/voc.2012.test");
//     char **paths = (char **)list_to_array(plist);

//     layer l = net.layers[net.n-1];
//     int classes = l.classes;
//     int square = l.sqrt;
//     int side = l.side;

//     int j;
//     FILE **fps = calloc(classes, sizeof(FILE *));
//     for(j = 0; j < classes; ++j){
//         char buff[1024];
//         snprintf(buff, 1024, "%s%s.txt", base, voc_names[j]);
//         fps[j] = fopen(buff, "w");
//     }
//     box *boxes = calloc(side*side*l.n, sizeof(box));
//     float **probs = calloc(side*side*l.n, sizeof(float *));
//     for(j = 0; j < side*side*l.n; ++j) probs[j] = calloc(classes, sizeof(float *));

//     int m = plist->size;
//     int i=0;
//     int t;

//     float thresh = .001;
//     int nms = 1;
//     float iou_thresh = .5;

//     int nthreads = 2;
//     image *val = calloc(nthreads, sizeof(image));
//     image *val_resized = calloc(nthreads, sizeof(image));
//     image *buf = calloc(nthreads, sizeof(image));
//     image *buf_resized = calloc(nthreads, sizeof(image));
//     pthread_t *thr = calloc(nthreads, sizeof(pthread_t));

//     load_args args = {0};
//     args.w = net.w;
//     args.h = net.h;
//     args.type = IMAGE_DATA;

//     for(t = 0; t < nthreads; ++t){
//         args.path = paths[i+t];
//         args.im = &buf[t];
//         args.resized = &buf_resized[t];
//         thr[t] = load_data_in_thread(args);
//     }
//     time_t start = time(0);
//     for(i = nthreads; i < m+nthreads; i += nthreads){
//         fprintf(stderr, "%d\n", i);
//         for(t = 0; t < nthreads && i+t-nthreads < m; ++t){
//             pthread_join(thr[t], 0);
//             val[t] = buf[t];
//             val_resized[t] = buf_resized[t];
//         }
//         for(t = 0; t < nthreads && i+t < m; ++t){
//             args.path = paths[i+t];
//             args.im = &buf[t];
//             args.resized = &buf_resized[t];
//             thr[t] = load_data_in_thread(args);
//         }
//         for(t = 0; t < nthreads && i+t-nthreads < m; ++t){
//             char *path = paths[i+t-nthreads];
//             char *id = basecfg(path);
//             float *X = val_resized[t].data;
//             float *predictions = network_predict(net, X);
//             int w = val[t].w;
//             int h = val[t].h;
//             convert_yolo_detections(predictions, classes, l.n, square, side, w, h, thresh, probs, boxes, 0, 0, 0, 0, 1.0);
//             if (nms) do_nms_sort(boxes, probs, side*side*l.n, classes, iou_thresh);
//             print_yolo_detections(fps, id, boxes, probs, side*side*l.n, classes, w, h);
//             free(id);
//             free_image(val[t]);
//             free_image(val_resized[t]);
//         }
//     }
//     fprintf(stderr, "Total Detection Time: %f Seconds\n", (double)(time(0) - start));
// }

// void validate_yolo_recall(char *cfgfile, char *weightfile)
// {
//     network net = parse_network_cfg(cfgfile, 1);
//     if(weightfile){
//         load_weights(&net, weightfile);
//     }
//     set_batch_network(&net, 1);
//     fprintf(stderr, "Learning Rate: %g, Momentum: %g, Decay: %g\n", net.learning_rate, net.momentum, net.decay);
//     srand(time(0));

//     char *base = "results/comp4_det_test_";
//     list *plist = get_paths("data/voc.2007.test");
//     char **paths = (char **)list_to_array(plist);

//     layer l = net.layers[net.n-1];
//     int classes = l.classes;
//     int square = l.sqrt;
//     int side = l.side;

//     int j, k;
//     FILE **fps = calloc(classes, sizeof(FILE *));
//     for(j = 0; j < classes; ++j){
//         char buff[1024];
//         snprintf(buff, 1024, "%s%s.txt", base, voc_names[j]);
//         fps[j] = fopen(buff, "w");
//     }
//     box *boxes = calloc(side*side*l.n, sizeof(box));
//     float **probs = calloc(side*side*l.n, sizeof(float *));
//     for(j = 0; j < side*side*l.n; ++j) probs[j] = calloc(classes, sizeof(float *));

//     int m = plist->size;
//     int i=0;

//     float thresh = .001;
//     float iou_thresh = .5;
//     float nms = 0;

//     int total = 0;
//     int correct = 0;
//     int proposals = 0;
//     float avg_iou = 0;

//     for(i = 0; i < m; ++i){
//         char *path = paths[i];
//         image orig = load_image_color(path, 0, 0);
//         image sized = resize_image(orig, net.w, net.h);
//         char *id = basecfg(path);
//         float *predictions = network_predict(net, sized.data);
//         convert_yolo_detections(predictions, classes, l.n, square, side, 1, 1, thresh, probs, boxes, 1, 0, 0, 0, 1.0);
//         if (nms) do_nms(boxes, probs, side*side*l.n, 1, nms);

//         char *labelpath = find_replace(path, "images", "labels");
//         labelpath = find_replace(labelpath, "JPEGImages", "labels");
//         labelpath = find_replace(labelpath, ".jpg", ".txt");
//         labelpath = find_replace(labelpath, ".JPEG", ".txt");

//         int num_labels = 0;
//         box_label *truth = read_boxes(labelpath, &num_labels);
//         for(k = 0; k < side*side*l.n; ++k){
//             if(probs[k][0] > thresh){
//                 ++proposals;
//             }
//         }
//         for (j = 0; j < num_labels; ++j) {
//             ++total;
//             box t = {truth[j].x, truth[j].y, truth[j].w, truth[j].h};
//             float best_iou = 0;
//             for(k = 0; k < side*side*l.n; ++k){
//                 float iou = box_iou(boxes[k], t);
//                 if(probs[k][0] > thresh && iou > best_iou){
//                     best_iou = iou;
//                 }
//             }
//             avg_iou += best_iou;
//             if(best_iou > iou_thresh){
//                 ++correct;
//             }
//         }

//         fprintf(stderr, "%5d %5d %5d\tRPs/Img: %.2f\tIOU: %.2f%%\tRecall:%.2f%%\n", i, correct, total, (float)proposals/(i+1), avg_iou*100/total, 100.*correct/total);
//         free(id);
//         free_image(orig);
//         free_image(sized);
//     }
// }

// void test_yolo(char *cfgfile, char *weightfile, char *filename, float thresh)
// {
//     network net = parse_network_cfg(cfgfile, 1);
//     if(weightfile){
//         load_weights(&net, weightfile);
//     }
//     detection_layer l = net.layers[net.n-1];
//     set_batch_network(&net, 1);
//     srand(2222222);
//     clock_t time;
//     char buff[256];
//     char *input = buff;
//     int j;
//     float nms=.5;
//     box *boxes = calloc(l.side*l.side*l.n, sizeof(box));
//     float **probs = calloc(l.side*l.side*l.n, sizeof(float *));
//     for(j = 0; j < l.side*l.side*l.n; ++j) probs[j] = calloc(l.classes, sizeof(float *));
//     while(1){
//         if(filename){
//             strncpy(input, filename, 256);
//         } else {
//             printf("Enter Image Path: ");
//             fflush(stdout);
//             input = fgets(input, 256, stdin);
//             if(!input) return;
//             strtok(input, "\n");
//         }
//         image im = load_image_color(input,0,0);
//         image sized = resize_image(im, net.w, net.h);
//         float *X = sized.data;
//         time=clock();
//         float *predictions = network_predict(net, X);

//         printf("%s: Predicted in %f seconds.\n", input, sec(clock()-time));
//         convert_yolo_detections(predictions, l.classes, l.n, l.sqrt, l.side, 1, 1, thresh, probs, boxes, 0, 0, 0, 0, 1.0);
//         if (nms) do_nms_sort(boxes, probs, l.side*l.side*l.n, l.classes, nms);
//         draw_detections(im, l.side*l.side*l.n, thresh, boxes, probs, voc_names, voc_labels, l.classes);
//         show_image(im, "predictions");

//         show_image(sized, "resized");
//         free_image(im);
//         free_image(sized);
// #ifdef OPENCV
//         cvWaitKey(0);
//         cvDestroyAllWindows();
// #endif
//         if (filename) break;
//     }
// }

void test_yolo_results(network *net, char *filename, float sensitivity, int grid, float* results, int result_index, int verbose, int quiet)
{
    if(verbose)
    {
        printf("\n[pydarknet c] Preparing network");
    }

    detection_layer l = net->layers[net->n-1];
    set_batch_network(net, 1);
    srand(2222222);
    clock_t time;
    char buff[256];
    char *input = buff;
    int i, j, prob_offset, offset;
    float nms=.25;
    int num = l.side*l.side*l.n;
    if(grid)
    {
        num *= 10;
    }
    box *boxes = calloc(num, sizeof(box));
    float **probs = calloc(num, sizeof(float *));
    for(j = 0; j < num; ++j) probs[j] = calloc(l.classes, sizeof(float *));

    strncpy(input, filename, 256);

    if(verbose)
    {
        printf("\n[pydarknet c] Preparing image %s", input);
    }

    int super_w = (int) net->w * (5.0 / 3.0) + 1;
    int super_h = (int) net->h * (5.0 / 3.0) + 1;
    image im = load_image_color(input,0,0);
    image sized_super = resize_image(im, super_w, super_h);

    float *X;
    float *predictions;

    time=clock();

    image sized = resize_image(sized_super, net->w, net->h);
    X = sized.data;

    if(verbose)
    {
        printf("\n[pydarknet c] Predicting on %s", input);
    }
    predictions = network_predict(*net, X);
    offset = 0;
    convert_yolo_detections(predictions, l.classes, l.n, l.sqrt, l.side, 1, 1, sensitivity, probs, boxes, 0, offset, 0, 0, 1.0);

    if(grid)
    {
        if(verbose)
        {
            printf("0: (0 0 %d %d)\n", sized_super.w, sized_super.h);
        }
        int dx, dy;
        for(i = 0; i < 3; ++i)
        {
            for(j = 0; j < 3; ++j)
            {
                offset = i * 3 + j + 1;
                dx = (int) (super_w + 1) * (i / 5.0);
                dy = (int) (super_h + 1) * (j / 5.0);
                sized = crop_image(sized_super, dx, dy, net->w, net->h);
                if(verbose)
                {
                    printf("%d: (%d %d %d %d)\n", offset, dx, dy, sized.w, sized.h);
                }
                X = sized.data;
                predictions = network_predict(*net, X);
                convert_yolo_detections(predictions, l.classes, l.n, l.sqrt, l.side, 1, 1, sensitivity, probs, boxes, 0, offset, i / 5.0, j / 5.0, 3.0 / 5.0);
            }
        }
    }

    if(verbose)
    {
        printf("[pydarknet c] %s: Predicted in %f seconds.\n", input, sec(clock()-time));
    }

    free_image(im);
    free_image(sized_super);
    free_image(sized);

    if(grid)
    {
        for(i = 0; i < num / 10; ++i)
        {
            for(j = 0; j < l.classes; ++j)
            {
                probs[i][j] *= 2.0;
            }
        }
    }

    if (nms) do_nms_sort(boxes, probs, num, l.classes, nms);

    if(grid)
    {
        for(i = 0; i < num / 10; ++i)
        {
            for(j = 0; j < l.classes; ++j)
            {
                probs[i][j] /= 2.0;
            }
        }
    }

    int result_length = num * (l.classes + 4);
    int result_offset = result_index * result_length;

    for(i = 0; i < num; ++i)
    {
        for(j = 0; j < l.classes; ++j)
        {
            results[result_offset + i * l.classes + j] = probs[i][j];
        }
    }

    float left, right, top, bot, width, height;
    box b;
    prob_offset = num * l.classes;
    for(i = 0; i < num; ++i)
    {
        offset = prob_offset + i * 4;

        b = boxes[i];
        left  = (b.x-b.w/2.)*im.w;
        right = (b.x+b.w/2.)*im.w;
        top   = (b.y-b.h/2.)*im.h;
        bot   = (b.y+b.h/2.)*im.h;

        if(left < 0) left = 0;
        if(right > im.w-1) right = im.w-1;
        if(top < 0) top = 0;
        if(bot > im.h-1) bot = im.h-1;

        width = right - left;
        height = bot - top;

        if(width < 1)
        {
            width = 1;
            if(left + width > im.w-1) left = im.w-1-width;
        }
        if(height < 1)
        {
            height = 1;
            if(top + height > im.h-1) top = im.h-1-height;
        }

        results[result_offset + offset + 0] = left;
        results[result_offset + offset + 1] = top;
        results[result_offset + offset + 2] = width;
        results[result_offset + offset + 3] = height;
    }

    free(boxes);
    for(j = 0; j < num; ++j) free(probs[j]);
    free(probs);
}

void export_yolo_to_numpy(char *cfgfile, char *weightfile)
{
    network net = parse_network_cfg(cfgfile, 1);
    if(weightfile){
        load_weights(&net, weightfile);
        save_weights_numpy(net, weightfile);
        visualize_network(net);
    }
}

/*
#ifdef OPENCV
image ipl_to_image(IplImage* src);
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"

void demo_swag(char *cfgfile, char *weightfile, float thresh)
{
network net = parse_network_cfg(cfgfile, 1);
if(weightfile){
load_weights(&net, weightfile);
}
detection_layer layer = net.layers[net.n-1];
CvCapture *capture = cvCreateCameraCapture(-1);
set_batch_network(&net, 1);
srand(2222222);
while(1){
IplImage* frame = cvQueryFrame(capture);
image im = ipl_to_image(frame);
cvReleaseImage(&frame);
rgbgr_image(im);

image sized = resize_image(im, net.w, net.h);
float *X = sized.data;
float *predictions = network_predict(net, X);
draw_swag(im, predictions, layer.side, layer.n, "predictions", thresh);
free_image(im);
free_image(sized);
cvWaitKey(10);
}
}
#else
void demo_swag(char *cfgfile, char *weightfile, float thresh){}
#endif
 */

// void demo_yolo(char *cfgfile, char *weightfile, float thresh, int cam_index);
// #ifndef GPU
// void demo_yolo(char *cfgfile, char *weightfile, float thresh, int cam_index)
// {
//     fprintf(stderr, "Darknet must be compiled with CUDA for YOLO demo.\n");
// }
// #endif

void run_yolo(int argc, char **argv)
{
    // int i;
    // for(i = 0; i < 5; ++i){
    //     char buff[256];
    //     sprintf(buff, "data/labels/%s.png", voc_names[i]);
    //     voc_labels[i] = load_image_color(buff, 0, 0);
    // }

    // float thresh = find_float_arg(argc, argv, "-thresh", .2);
    // int cam_index = find_int_arg(argc, argv, "-c", 0);
    if(argc < 4){
        fprintf(stderr, "usage: %s %s [train/test/valid] [cfg] [weights (optional)]\n", argv[0], argv[1]);
        return;
    }

    char *cfg = argv[3];
    char *weights = (argc > 4) ? argv[4] : 0;
    // char *filename = (argc > 5) ? argv[5]: 0;
    // if(0==strcmp(argv[2], "test")) test_yolo(cfg, weights, filename, thresh);
    // else if(0==strcmp(argv[2], "train")) train_yolo(cfg, weights);
    // else if(0==strcmp(argv[2], "valid")) validate_yolo(cfg, weights);
    // else if(0==strcmp(argv[2], "recall")) validate_yolo_recall(cfg, weights);
    if(0==strcmp(argv[2], "numpy")) export_yolo_to_numpy(cfg, weights);
    // else if(0==strcmp(argv[2], "demo")) demo_yolo(cfg, weights, thresh, cam_index);
}
