#include <cnet.h>
#include <cnetsupport.h>
#include <string.h>
#include <time.h>

#define	WALKING_SPEED		3.0
#define	PAUSE_TIME		20

#define	TX_NEXT			(5000000 + rand()%5000000)

typedef struct {
    int			dest;
    int			src;
    // CnetPosition	prevpos;	// position of previous node
    int			length;		// length of payload
    int         IsBeacon;  // 1 if beacon, 0 if not
    int         IsAck;  // 1 if ack, 0 if not
    int         IsRequest;  // 1 if request, 0 if not
} WLAN_HEADER;

typedef struct {
    WLAN_HEADER		header;
    char		payload[2304];
} WLAN_FRAME;

// static	int		*stats		= NULL;
// static	CnetPosition	*positions	= NULL;

static	bool		verbose		= true;

// frame table for anchor
WLAN_FRAME anchor_frame_table[20];

// 2 string for mobiles and access points addresses
int mobiles[5];
int anchors;

/* ----------------------------------------------------------------------- */

static EVENT_HANDLER(transmit)
{
    WLAN_FRAME	frame;
    int		link	= 1;

    //  POPULATE A NEW FRAME
    do {
    int rand_index = rand() % 5;  // generate a random index between 0 and 4
    frame.header.dest = mobiles[rand_index];
    // frame.header.dest	= rand() % NNODES;
    } while(frame.header.dest == nodeinfo.nodenumber);
    frame.header.src		= nodeinfo.nodenumber;
    frame.header.IsBeacon = 0;
    frame.header.IsAck = 0;
    frame.header.IsRequest = 0;
    // frame.header.prevpos	= positions[nodeinfo.nodenumber];	// me!

    sprintf(frame.payload, "hello from %d", nodeinfo.nodenumber);
    frame.header.length	= strlen(frame.payload) + 1;	// send nul-byte too

    //  TRANSMIT THE FRAME
    size_t len	= sizeof(WLAN_HEADER) + frame.header.length;
    CHECK(CNET_write_physical_reliable(link, &frame, &len));
    ++stats[0];

    if(verbose) {
    // DISPLAY OUR CURRENT POSITION
    CnetPosition pos, max;
    CNET_get_position(&pos, &max);
    printf("mobile [%d](x=%f, y=%f): pkt (transmitted) (src=%d, dest=%d)\n", nodeinfo.address, pos.x, pos.y, frame.header.src, frame.header.dest);
    fprintf(stdout, "\nmobile [%d](x=%f, y=%f): pkt (transmitted) (src=%d, dest=%d)\n", nodeinfo.address, pos.x, pos.y, frame.header.src, frame.header.dest);
    }

    //  SCHEDULE OUR NEXT TRANSMISSION
    CNET_start_timer(EV_TIMER1, TX_NEXT, 0);
    
}

// //  DETERMINE 2D-DISTANCE BETWEEN TWO POSITIONS
// static double distance(CnetPosition p0, CnetPosition p1)
// {
//     int	dx	= p1.x - p0.x;
//     int	dy	= p1.y - p0.y;

//     return sqrt(dx*dx + dy*dy);
// }

static EVENT_HANDLER(receive)
{
    WLAN_FRAME	frame;
    size_t	len;
    int		link;

    //  READ THE ARRIVING FRAME FROM OUR PHYSICAL LINK
    len	= sizeof(frame);
    CHECK(CNET_read_physical(&link, &frame, &len));
    if(verbose) {
    double	rx_signal;
    CHECK(CNET_wlan_arrival(link, &rx_signal, NULL));
    }

    // if the node is mobile
    if (nodeinfo.nodetype == NT_MOBILE){
        // if it is not a beacon message, then it is a normal message
        if (frame.header.IsBeacon == 0 && frame.header.IsAck == 0){
            //  IS THIS FRAME FOR ME?
            if(frame.header.dest == nodeinfo.nodenumber) {
            ++stats[1];
            if(verbose)
                printf("mobile [%d]: pkt (received) (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                fprintf(stdout, "\nmobile [%d]: pkt (received) (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);       

            // send an ack frame to the sender 
            // the sender might be the anchor or mobiles
            // ignore the ack if the sender is mobile
            // delete the first frame in the frame table if the sender is anchor
            WLAN_FRAME	ackframe;
            ackframe.header.dest = frame.header.src;
            ackframe.header.src = nodeinfo.nodenumber;
            ackframe.header.IsBeacon = 0;
            ackframe.header.IsAck = 1;
            ackframe.header.IsRequest = 0;
            ackframe.header.length = 0;
            size_t len	= sizeof(WLAN_HEADER) + ackframe.header.length;
            CHECK(CNET_write_physical_reliable(link, &ackframe, &len));
            }
            //  NO; RETRANSMIT FRAME through another link
            else {
            for (int i = 1; i <= nodeinfo.nlinks; i++){
                if (i != link){
                size_t len	= sizeof(WLAN_HEADER) + frame.header.length;
                CHECK(CNET_write_physical_reliable(i, &frame, &len));
                ++stats[0];
                printf("mobile [%d]: pkt (relayed) (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                fprintf(stdout, "\nmobile [%d]: pkt (relayed) (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                }
                break;
            }
            }
        }
        else if (frame.header.IsBeacon == 0 && frame.header.IsAck == 1){
            // if the node is an anchor
            // if it is an ack frame, then it is from mobile
            // check if there're any dest in the msg is the same as the node's address
            // if so, delete the first frame in the frame table
            if (nodeinfo.nodetype == NT_ACCESSPOINT)
            {
                int recvdest = frame.header.src;
                for (int i = 0; i < 20; i++){
                    if (anchor_frame_table[i].header.dest == recvdest){
                        anchor_frame_table[i].header.dest = -1;
                    }
                    break;
                }
            }
        }
        else if (frame.header.IsBeacon == 1 && frame.header.IsAck == 0){
            // if it is a beacon message, then it is from anchor
            // check if there're any dest in the msg is the same as the node's address
            // if so, send a request frame to the anchor
            // int posx, posy; // position of the anchor
            int dests[20];  // destination addresses
            char *token;
            int i = 0;
            const char *delim = ",";

            // parse the position from the payload string
            token = strtok(frame.payload, delim);
            // posx = atoi(token);
            token = strtok(NULL, delim);
            // posy = atoi(token);

            // parse the destination addresses from the payload string
            token = strtok(NULL, delim);
            while (token != NULL) {
                dests[i] = atoi(token);
                token = strtok(NULL, delim);
                i++;
            }
            // check if the node is in the destination list
            for (int j = 0; j < i; j++){
                if (dests[j] == nodeinfo.nodenumber){
                    // send a request frame to the anchor
                    WLAN_FRAME	reqframe;
                    reqframe.header.dest = frame.header.src;
                    reqframe.header.src = nodeinfo.nodenumber;
                    reqframe.header.IsBeacon = 0;
                    reqframe.header.IsAck = 0;
                    reqframe.header.IsRequest = 1;
                    reqframe.header.length = 0;
                    size_t len	= sizeof(WLAN_HEADER) + reqframe.header.length;
                    CHECK(CNET_write_physical_reliable(link, &reqframe, &len));
                    break;
                }
            }
        }
    }
    else if (nodeinfo.nodetype == NT_ACCESSPOINT){
        // if it is not a request frame, then it is a normal message, store it to the table
        
        if (frame.header.IsRequest == 0){
            int table_is_full = 1;
            for (int i = 0; i < 20; i++){
                if (anchor_frame_table[i].header.dest == -1){
                    anchor_frame_table[i] = frame;
                    printf("anchor [%d]: pkt received and stored (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                    fprintf(stdout, "\nanchor [%d]: pkt received and stored (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                    table_is_full = 0;
                    break;
                }
            }
            if (table_is_full == 1){
                // retranmit the frame to the next mobile
                for (int i = 1; i <= nodeinfo.nlinks; i++){
                    size_t len	= sizeof(WLAN_HEADER) + frame.header.length;
                    CHECK(CNET_write_physical_reliable(i, &frame, &len));
                    ++stats[0];
                    printf("anchor [%d]: pkt (relayed) (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                    fprintf(stdout, "\nanchor [%d]: pkt (relayed) (src=%d, dest=%d)\n", nodeinfo.address, frame.header.src, frame.header.dest);
                    break;
                }
            }
        }
        // if it is a request frame, then it is from mobile
        // find the first frame in the frame table that has the same dest as the node's address and send it and clean the table
        else if (frame.header.IsRequest == 1){
            printf("anchor [%d]: download request (dest=%d)\n", nodeinfo.address, frame.header.src);
            fprintf(stdout, "\nanchor [%d]: download request (dest=%d)\n", nodeinfo.address, frame.header.src);
            for (int i = 0; i < 20; i++){
                if (anchor_frame_table[i].header.dest == frame.header.src){
                    size_t len	= sizeof(WLAN_HEADER) + anchor_frame_table[i].header.length;
                    CHECK(CNET_write_physical_reliable(link, &anchor_frame_table[i], &len));
                    // clean the table
                    anchor_frame_table[i].header.dest = -1;
                    break;
                }
            }
        }

    }
}

// //  THIS FUNCTION IS CALLED ONCE, ON TERMINATION, TO REPORT OUR STATISTICS
// static EVENT_HANDLER(finished)
// {
//     fprintf(stdout, "messages generated:\t%d\n", stats[0]);
//     fprintf(stdout, "messages received:\t%d\n", stats[1]);
//     if(stats[0] > 0)
// 	fprintf(stdout, "delivery ratio:\t\t%.1f%%\n", 100.0*stats[1]/stats[0]);
// }

//  this function is called periodically to send beacon messages from anchor
static EVENT_HANDLER(send_beacon_msg){
    WLAN_FRAME	frame;
    CnetPosition pos, max;
    CNET_get_position(&pos, &max);

    // create the message string
    char msg[32];  // assume a maximum of 32 characters
    snprintf(msg, sizeof(msg), "%f,%f", pos.x, pos.y);

    // concatenate the destination addresses to the message string
    for (int i = 0; i < 20; i++) {
        if (anchor_frame_table[i].header.dest != -1) {
            char dest_str[16];  // assume a maximum of 16 characters for each destination address
            snprintf(dest_str, sizeof(dest_str), "%d", anchor_frame_table[i].header.dest);
            strcat(msg, ",");
            strcat(msg, dest_str);
        }
    }

    // now the msg looks like this: "x,y,dest1,dest2,dest3,..."

    frame.header.IsBeacon = 1;
    frame.header.IsAck = 0;
    frame.header.IsRequest = 0;
    frame.header.src		= nodeinfo.nodenumber;
    frame.header.dest		= 0;	// broadcast

    strcpy(frame.payload, msg);
    frame.header.length	= strlen(frame.payload) + 1;	// send nul-byte too

    for (int i = 1; i <= nodeinfo.nlinks; i++){
        //  TRANSMIT THE FRAME
        size_t len	= sizeof(WLAN_HEADER) + frame.header.length;
        CHECK(CNET_write_physical_reliable(i, &frame, &len));
    }

    //  resend the beacon message after 5 seconds
    CNET_start_timer (EV_TIMER2, 5000000, 0);
}

EVENT_HANDLER(reboot_node)
{
    extern void init_mobility(double walkspeed_m_per_sec, int pausetime_secs);

    

    //  get the address of the mobiles and access points
    //-------------------------------------------------
    // retrieve values from topology file
    char *anchors_str = CNET_getvar("anchors");
    char *mobiles_str = CNET_getvar("mobiles");
    char *token;
    int i;

    anchors = atoi(anchors_str);

    // tokenize and convert mobiles string to int array
    i = 0;
    token = strtok(mobiles_str, ",");
    while (token != NULL && i < 5) {
        mobiles[i] = atoi(token);
        token = strtok(NULL, ",");
        i++;
    }
    //-------------------------------------------------

    //  ENSURE THAT WE'RE USING THE CORRECT VERSION OF cnet
    // CNET_check_version(CNET_VERSION);
    // srand(time(NULL) + nodeinfo.nodenumber);

    //  INITIALIZE MOBILITY PARAMETERS
    if (nodeinfo.nodetype == NT_MOBILE)
    {    
        // init_mobility(WALKING_SPEED, PAUSE_TIME);
        CHECK(CNET_set_handler(EV_TIMER1, transmit, 0));
        CNET_start_timer(EV_TIMER1, TX_NEXT, 0);
    }
    if (nodeinfo.nodetype == NT_ACCESSPOINT)
    {
        // set table all the dest to -1
        for (int i = 0; i < 20; i++) {
            anchor_frame_table[i].header.dest = -1;
        }
        CNET_set_handler(EV_TIMER2, send_beacon_msg, 0);
        // Request EV_TIMER2 in 2 sec, ignore return value
        CNET_start_timer (EV_TIMER2, 2000000, 0);
    }

    //  ALLOCATE MEMORY FOR SHARED MEMORY SEGMENTS
    // stats	= CNET_shmem2("s", 2*sizeof(int));
    // positions	= CNET_shmem2("p", NNODES*sizeof(CnetPosition));


//  SET HANDLERS FOR EVENTS FROM THE PHYSICAL LAYER
    CHECK(CNET_set_handler(EV_PHYSICALREADY,  receive, 0));

//  WHEN THE SIMULATION TERMINATES, NODE-0 WILL REPORT THE STATISTICS
    // if(nodeinfo.nodenumber == 0)
	// CHECK(CNET_set_handler(EV_SHUTDOWN,  finished, 0));
}
